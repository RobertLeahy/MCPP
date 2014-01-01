#include <chat/chat.hpp>
#include <command/command.hpp>
#include <permissions/permissions.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <utility>


using namespace MCPP;


static const String name("Shutdown/Restart Command");
static const Word priority=1;
static const String shutdown_identifier("shutdown");
static const String restart_identifier("restart");
static const String shutdown_summary("Shuts down the server.");
static const String restart_summary("Restarts the server.");
static const String shutdown_desc("shutdown");
static const String restart_desc("restart");
static const String shutdown_desc_initial_cap("Shutdown");
static const String restart_desc_initial_cap("Restart");
static const String help(
	"Syntax: /{0} [cancel] [quiet] [<time>]\n"
	"Instructs the server to {1}.\n"
	"If a time (in seconds) is specified the server will delay the {1} for that amount of time after the command is executed.\n"
	"Will broadcast through chat unless \"quiet\" is specified.\n"
	"If \"cancel\" is specified, any shutdown or restart currently pending will be cancelled.\n"
	"Upon cancellation a broadcast will be sent unless \"quiet\" is specified."
);
static const String cancel_arg("cancel");
static const String quiet_arg("quiet");
static const String no_pending("There is no pending shutdown or restart.");
static const String pending("Another shutdown or restart is pending.");
static const String cancelled("Pending {0} cancelled");
static const String by(" by {0}");
static const String requested("{0} requested");
static const String in(" in {0}");
static const String announcement("Server {0} in {1}");


//	Time templates
static const String time_separator(", ");
static const String time_template("{0} {1}");
static const String day("day");
static const String hour("hour");
static const String minute("minute");
static const String second("second");
static const String days("days");
static const String hours("hours");
static const String minutes("minutes");
static const String seconds("seconds");
static const Word mps=1000;
static const Word mpm=60*mps;
static const Word mph=60*mpm;
static const Word mpd=24*mph;


static inline String get_worker (Word & milliseconds, Word per, const String & singular, const String & plural) {

	String retr;
	
	if (milliseconds>=per) {
	
		Word num=milliseconds/per;
		
		retr=String::Format(
			time_template,
			num,
			(num==1) ? singular : plural
		);
		
		if ((milliseconds%per)!=0) retr << time_separator;
		
		milliseconds-=num*per;
	
	}
	
	return retr;

}


static inline String get_days (Word & milliseconds) {

	return get_worker(
		milliseconds,
		mpd,
		day,
		days
	);

}


static inline String get_hours (Word & milliseconds) {

	return get_worker(
		milliseconds,
		mph,
		hour,
		hours
	);

}


static inline String get_minutes (Word & milliseconds) {

	return get_worker(
		milliseconds,
		mpm,
		minute,
		minutes
	);

}


static inline String get_seconds (Word & milliseconds) {

	return get_worker(
		milliseconds,
		mps,
		second,
		seconds
	);

}


static inline String get_time (Word milliseconds) {

	String retr(get_days(milliseconds));
	retr << get_hours(milliseconds);
	retr << get_minutes(milliseconds);
	retr << get_seconds(milliseconds);
	return retr;

}


static inline Word get_next_impl (Word curr, Word modulo) noexcept {

	return ((curr%modulo)==0) ? (curr-modulo) : ((curr/modulo)*modulo);

}


static inline Word get_next (Word curr) noexcept {

	//	15 seconds or less, send an announcement
	//	every second
	if (curr<=(15*mps)) return get_next_impl(curr,mps);
	
	//	A minute or less, send an announcement
	//	every 15 seconds
	if (curr<=mpm) return get_next_impl(curr,15*mps);
	
	//	Fifteen minutes or less, send an
	//	announcement every minute
	if (curr<=(15*mpm)) return get_next_impl(curr,mpm);
	
	//	An hour or less, send an announcement
	//	every 15 minutes
	if (curr<=mph) return get_next_impl(curr,15*mpm);
	
	//	Over an hour, send an announcement every
	//	hour
	return get_next_impl(curr,mph);

}


class ShutdownInfo {


	public:
	
	
		//	Whether or not this shutdown is
		//	quiet, i.e. whether or not it will
		//	broadcast a countdown through chat
		bool Quiet;
		//	How many seconds should be delayed
		//	before the shutdown/restart
		Word When;
		//	Whether or not this is a restart
		//	request
		bool Restart;
		//	Who requested this shutdown or
		//	restart
		Nullable<String> RequestedBy;
		//	Whether this is a cancellation request
		//	or not
		bool Cancel;
		
		
		ShutdownInfo () noexcept : Quiet(false), When(0), Cancel(false) {	}


};


class Shutdown : public Module, public Command {


	private:
	
	
		//	Current shutdown/restart event
		Mutex lock;
		SmartPointer<ShutdownInfo> info;
	
	
		static bool is_shutdown (const String & identifier) {
		
			return identifier==shutdown_identifier;
		
		}
		
		
		SmartPointer<ShutdownInfo> parse (const CommandEvent & event) {
		
			auto retr=SmartPointer<ShutdownInfo>::Make();
			
			//	Parse arguments
			bool time_specified=false;
			for (const auto & arg : event.Arguments) {
			
				if (arg==cancel_arg) {
				
					//	Don't specify the same flag multiple
					//	times
					if (retr->Cancel) goto syntax_error;
					
					retr->Cancel=true;
					
				} else if (arg==quiet_arg) {
				
					//	Don't specify the same flag multiple
					//	times
					if (retr->Quiet) goto syntax_error;
					
					retr->Quiet=true;
				
				} else if (arg.ToInteger(&(retr->When))) {
				
					//	Don't specify the delay multiple
					//	times
					if (time_specified) goto syntax_error;
					
					time_specified=true;
				
				} else {
				
					//	Couldn't parse
					goto syntax_error;
					
				}
			
			}
			
			//	You can't delay a cancellation
			if (retr->Cancel && time_specified) goto syntax_error;
			
			//	Shutdown or restart?
			retr->Restart=!is_shutdown(event.Identifier);
			//	Who requested this action?
			if (!event.Issuer.IsNull()) retr->RequestedBy.Construct(event.Issuer->GetUsername());
			
			return retr;
			
			//	Control jumps here when there's a
			//	syntax error and null should be
			//	returned
			syntax_error:
			retr=SmartPointer<ShutdownInfo>();
			return retr;
		
		}
		
		
		void error (ChatMessage & message, String text) {
		
			message	<<	ChatStyle::Bold
					<<	ChatStyle::Red
					<<	std::move(text);
		
		}
		
		
		void success (ChatMessage & message, String text) {
		
			message <<	ChatStyle::Bold
					<<	ChatStyle::BrightGreen
					<<	std::move(text);
		
		}
		
		
		void warning (ChatMessage & message, String text) {
		
			message	<<	ChatStyle::Bold
					<<	ChatStyle::Yellow
					<<	std::move(text);
		
		}
		
		
		void cancel (CommandEvent event, const ShutdownInfo & info, CommandResult & retr) {
		
			//	Get the pending restart/shutdown
			auto cancelled=lock.Execute([&] () mutable {	return std::move(this->info);	});
			
			//	If there was no pending restart/shutdown,
			//	this command was erroneous
			if (cancelled.IsNull()) {
			
				error(retr.Message,no_pending);
			
			//	Otherwise we cancelled it
			} else {
			
				//	Prepare a message
				String log(
					String::Format(
						::cancelled,
						cancelled->Restart ? restart_desc : shutdown_desc
					)
				);
				
				//	If this is quiet, we just send this
				//	back to the issuer
				if (info.Quiet) success(retr.Message,log);
				
				//	Before we write to the log, we get the
				//	person (if any) who cancelled this shutdown
				if (!info.RequestedBy.IsNull()) log << String::Format(
					by,
					*info.RequestedBy
				);
				
				//	If this isn't quiet, we send this to everyone
				//	now
				if (!info.Quiet) {
				
					ChatMessage broadcast;
					success(broadcast,log);
					Chat::Get().Send(broadcast);
				
				}
				
				//	Write out to the log
				Server::Get().WriteLog(
					log,
					Service::LogType::Information
				);
			
			}
		
		}
		
		
		void worker (SmartPointer<ShutdownInfo> info, Word remaining) {
			
			//	Ensure that this shutdown/restart
			//	event is still enqueued
			if (lock.Execute([&] () {	return static_cast<const ShutdownInfo *>(this->info);	})!=static_cast<ShutdownInfo *>(info)) return;
			
			auto & server=Server::Get();
			
			//	Actually shutdown/restart if applicable
			if (remaining==0) {
				
				if (info->Restart) server.Restart();
				else server.Stop();
				
				return;
			
			}
			
			server.PanicOnThrow([&] () mutable {
			
				Word next;
				//	If the shutdown/restart is quiet, there's no
				//	reason for this task to run again until it
				//	comes time to actually restart/shutdown
				if (info->Quiet) {
				
					next=0;
				
				} else {
				
					next=get_next(remaining);
				
					//	Send announcement
					ChatMessage message;
					warning(
						message,
						String::Format(
							announcement,
							info->Restart ? restart_desc : shutdown_desc,
							get_time(remaining)
						)
					);
					
					Chat::Get().Send(message);
				
				}
				
				//	Enqueue next iteration				
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wpedantic"
				server.Pool().Enqueue(
					remaining-next,
					[this,info=std::move(info),next] () mutable {	worker(std::move(info),next);	}
				);
				#pragma GCC diagnostic pop
			
			});
		
		}
		
		
		void execute (CommandEvent event, SmartPointer<ShutdownInfo> info, CommandResult & retr) {
		
			//	We test to see if there's already a pending
			//	restart/shutdown.  If there is we do not
			//	proceed, otherwise we install the new
			//	information block
			if (lock.Execute([&] () mutable {
			
				if (!this->info.IsNull()) return false;
				
				this->info=info;
				
				return true;
			
			})) try {
			
				//	Log/return message to issuer
				String log(
					String::Format(
						requested,
						info->Restart ? restart_desc_initial_cap : shutdown_desc_initial_cap
					)
				);
				if (info->When!=0) log << String::Format(
					in,
					get_time(info->When)
				);
				
				success(retr.Message,log);
				
				if (!info->RequestedBy.IsNull()) log << String::Format(
					by,
					*info->RequestedBy
				);
				
				auto & server=Server::Get();
				
				server.WriteLog(
					log,
					Service::LogType::Information
				);
				
				//	Enqueue task
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wpedantic"				
				server.Pool().Enqueue(
					[this,info=std::move(info)] () mutable {
					
						Word remaining=info->When;
					
						worker(
							std::move(info),
							remaining
						);
					
					}
				);
				#pragma GCC diagnostic pop
				
			//	On error, dequeue shutdown/restart
			} catch (...) {
			
				lock.Execute([&] () mutable {	this->info=SmartPointer<ShutdownInfo>();	});
				
				throw;
			
			}
			//	Another shutdown/restart pending
			else error(retr.Message,pending);
		
		}


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			auto & commands=Commands::Get();
			commands.Add(
				shutdown_identifier,
				this
			);
			commands.Add(
				restart_identifier,
				this
			);
		
		}
		
		
		virtual void Summary (const String & identifier, ChatMessage & message) override {
		
			message << (is_shutdown(identifier) ? shutdown_summary : restart_summary);
		
		}
		
		
		virtual void Help (const String & identifier, ChatMessage & message) override {
		
			message << String::Format(
				help,
				identifier,
				is_shutdown(identifier) ? shutdown_desc : restart_desc
			);
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			return event.Issuer.IsNull() ? true : Permissions::Get().GetUser(event.Issuer).Check(event.Identifier);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
		
			//	Parse
			auto info=parse(event);
			
			//	Abort immediately on syntax errors
			if (!info) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Convert seconds to milliseconds
			try {
			
				info->When=static_cast<Word>(SafeWord(info->When)*SafeWord(1000));
			
			} catch (...) {
			
				//	We report this overflow as a syntax error
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Notwithstanding syntax errors, the only
			//	other possibility is "success".
			//
			//	Success may still "fail" -- i.e. may not
			//	restart the server or enqueue a restart, but
			//	it will appear to be a success from the
			//	point-of-view of the command interpreter
			retr.Status=CommandStatus::Success;
			
			//	Handle cancellations
			if (info->Cancel) cancel(
				std::move(event),
				*info,
				retr
			);
			//	Handle actual shutdown/restart
			else execute(
				std::move(event),
				std::move(info),
				retr
			);
			
			return retr;
		
		}


};


INSTALL_MODULE(Shutdown)

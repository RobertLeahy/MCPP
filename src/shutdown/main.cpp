#include <chat/chat.hpp>
#include <command/command.hpp>
#include <op/op.hpp>
#include <server.hpp>
#include <memory>
#include <utility>


namespace MCPP {


	static const String name("Shutdown/Restart Command");
	static const String identifier("shutdown");
	static const Word priority=1;
	static const String summary("Shuts down or restarts the server.");
	static const String help(
		"Syntax: /shutdown [restart|cancel] [quiet] [<time>]\n"
		"Instructs the server to shutdown and—if \"restart\" is specified—restart.\n"
		"If a time (in seconds) is specified the server will wait that long before restarting.\n"
		"Will broadcast countdown through chat unless \"quiet\" is specified.\n"
		"If \"cancel\" is specified, any restart or shutdown currently pending will be cancelled.\n"
		"Upon cancellation a broadcast will be sent unless \"quiet\" is specified."
	);
	
	
	//	Parsing
	static const Regex is_restart(
		"(?:^|\\s)restart(?:$|\\s)",
		RegexOptions().SetIgnoreCase()
	);
	static const Regex is_cancel(
		"(?:^|\\s)cancel(?:$|\\s)",
		RegexOptions().SetIgnoreCase()
	);
	static const Regex is_quiet(
		"(?:^|\\s)quiet(?:$|\\s)",
		RegexOptions().SetIgnoreCase()
	);
	static const Regex extract_time(
		"(?:^|\\s)(\\d+)(?:$|\\s)"
	);
	static const Regex last_word(
		"[^\\s]*$"
	);
	static const String word_regex(
		"(?:^|\\s){0}"
	);
	
	
	//	Arguments
	static const String restart("restart");
	static const String cancel("cancel");
	static const String quiet("quiet");
	
	
	//	Logging
	static const String shutdown_label("Shutdown");
	static const String restart_label("Restart");
	static const String cancelled("{0} cancelled");
	static const String announcement("{0} in {1}");
	static const String log("{0} requested by {1}");
	static const String shutdown_log("Shutting down");
	static const String restart_log("Restarting");
	static const String separator(" ");
	
	
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
	
	
	//	Contains information about a
	//	shutdown/restart request
	class ShutdownInfo {
	
		
		public:
		
		
			//	Whether the shutdown is
			//	quiet or not
			bool Quiet;
			//	How long until shutdown
			//	when the task will wake
			//	up again
			Word Next;
			//	How long until shutdown
			//	when the task last woke
			//	up
			Word Last;
			//	How long since the task
			//	last woke up
			Timer Since;
			//	Whether or not this is
			//	a restart request
			bool Restart;
			//	Who requested this shutdown
			//	or restart
			Nullable<String> RequestedBy;
			//	Must be held to modify/read
			//	this structure (except the
			//	worker function, which doesn't
			//	need to lock to read, as it's
			//	the only thread allowed to
			//	modify the structure)
			Mutex Lock;
		
	
	};


	class Shutdown : public Module, public Command {
	
	
		private:
		
		
			Mutex lock;
			SmartPointer<ShutdownInfo> info;
			
			
			inline void do_shutdown (SmartPointer<ShutdownInfo> info) const {
			
				auto & server=Server::Get();
			
				server.WriteLog(
					info->Restart ? restart_log : shutdown_log,
					Service::LogType::Information
				);
				
				if (info->Restart) server.Restart();
				else server.Stop();
			
			}
			
			
			inline bool do_maintenance (const SmartPointer<ShutdownInfo> & info) {
			
				lock.Acquire();
				if (this->info!=info) {
				
					//	Cancelled
					
					lock.Release();
					
					return false;
				
				}
				//	If this action is to be
				//	performed at once, remove
				//	the pending action
				if (info->Next==0) this->info=SmartPointer<ShutdownInfo>();
				lock.Release();
				
				return true;
			
			}
			
			
			inline void do_periodic (SmartPointer<ShutdownInfo> info) const {
			
				if (info->Quiet) {
				
					info->Lock.Execute([&] () mutable {
					
						info->Next=0;
						info->Since=Timer::CreateAndStart();
					
					});
					
				} else {
			
					//	Send announcement
					ChatMessage message;
					message	<<	ChatStyle::Bold
							<<	ChatStyle::Yellow
							<<	String::Format(
									announcement,
									info->Restart ? restart_label : shutdown_label,
									get_time(info->Next)
								);
					Chat::Get().Send(message);
				
					//	When will the next iteration be?
					Word next=get_next(info->Next);
					
					//	Update structure
					info->Lock.Execute([&] () mutable {
					
						info->Last=info->Next;
						info->Next=next;
						info->Since=Timer::CreateAndStart();
					
					});
					
				}
			
				//	Enqueue next iteration
				
				//	Calculate next iteration time here
				//	to avoid undefined behaviour (using
				//	info to calculate the time and
				//	moving it into the lambda)
				Word when=info->Last-info->Next;
				
				//	C++14 explicitly supports
				//	generalized lambda capture,
				//	and GCC has supported this
				//	particular syntax since 4.5,
				//	so I see now reason not to
				//	take advantage of it
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wpedantic"
				Server::Get().Pool().Enqueue(
					when,
					[=,info=std::move(info)] () mutable {	worker_func(std::move(info));	}
				);
				#pragma GCC diagnostic pop
			
			}
			
			
			void worker_func (SmartPointer<ShutdownInfo> info) {
			
				Server::PanicOnThrow([&] () mutable {
				
					//	Make sure our shutdown/restart
					//	request wasn't cancelled
					if (!do_maintenance(info)) return;
					
					//	Are we to perform the action
					//	at once?
					if (info->Next==0) do_shutdown(std::move(info));
					//	Otherwise...
					else do_periodic(std::move(info));
				
				});
			
			}
	
	
		public:
		
		
			virtual const String & Name () const noexcept override {
			
				return name;
			
			}
			
			
			virtual Word Priority () const noexcept override {
			
				return priority;
			
			}
			
			
			virtual void Install () override {
			
				Commands::Get().Add(this);
			
			}
			
			
			virtual const String & Identifier () const noexcept override {
			
				return identifier;
			
			}
			
			
			virtual const String & Summary () const noexcept override {
			
				return summary;
			
			}
			
			
			virtual const String & Help () const noexcept override {
			
				return help;
			
			}
			
			
			virtual bool Check (SmartPointer<Client> client) const noexcept override {
			
				return Ops::Get().IsOp(client->GetUsername());
			
			}
			
			
			virtual Vector<String> AutoComplete (const String & args) const {
			
				//	Attempt to extract the last word
				auto match=last_word.Match(args);
				
				Vector<String> retr;
				
				Regex matcher(String::Format(
					word_regex,
					Regex::Escape(
						match.Value()
					)
				));
				
				if (
					matcher.IsMatch(restart) &&
					!is_restart.IsMatch(args)
				) retr.Add(restart);
				
				if (
					matcher.IsMatch(quiet) &&
					!is_quiet.IsMatch(args)
				) retr.Add(quiet);
				
				if (
					matcher.IsMatch(cancel) &&
					!is_cancel.IsMatch(args)
				) retr.Add(cancel);
				
				return retr;
			
			}
			
			
			virtual bool Execute (SmartPointer<Client> client, const String & args, ChatMessage & message) {
			
				//	Are we cancelling a restart/shutdown
				//	request?
				if (is_cancel.IsMatch(args)) {
				
					//	Null the shutdown request (cancelling
					//	it), and load it (so we can determine
					//	whether anything was actually cancelled
					//	at all)
					lock.Acquire();
					auto info=std::move(this->info);
					lock.Release();
					
					if (!info.IsNull()) {
					
						//	We cancelled a shutdown
						
						//	Broadcast if not quiet, otherwise
						//	just return the cancelled message
						//	to the requester
						Nullable<ChatMessage> broadcast;
						bool quiet=is_quiet.IsMatch(args);
						if (!quiet) broadcast.Construct();
						ChatMessage & m=quiet ? message : *broadcast;
						
						m	<<	ChatStyle::BrightGreen
							<<	ChatStyle::Bold
							<<	String::Format(
									cancelled,
									info->Restart ? restart_label : shutdown_label
								);
								
						//	Send if applicable
						if (!quiet) Chat::Get().Send(*broadcast);
					
					}
					
					//	Done
					return true;
				
				}
				
				//	Create a shutdown info structure
				//	to hold information about this
				//	shutdown event
				auto info=SmartPointer<ShutdownInfo>::Make();
				
				//	Parse the request
				
				//	Quiet?
				info->Quiet=is_quiet.IsMatch(args);
				
				//	Restart?
				info->Restart=is_restart.IsMatch(args);
				
				//	Time?
				auto match=extract_time.Match(args);
				if (match.Success()) {
				
					Word seconds;
					if (!match[1].Value().ToInteger(&seconds)) return false;
				
					try {
					
						info->Next=Word(SafeWord(seconds)*SafeWord(1000));
					
					} catch (...) {
					
						//	Number of seconds specified was
						//	too large
						
						return false;
					
					}
				
				}
				
				//	Initialize remaining fields
				info->Last=info->Next;
				if (!client.IsNull()) info->RequestedBy.Construct(client->GetUsername());
				
				auto & server=Server::Get();
				
				//	If the shutdown/restart is not
				//	immediate, there's logging et cetera
				//	to do
				if (info->Next!=0) {
				
					String str(String::Format(
						announcement,
						info->Restart ? restart_label : shutdown_label,
						get_time(info->Next)
					));
					
					//	If this shutdown/restart is quiet,
					//	return the announcement to the
					//	requester
					if (info->Quiet) message << ChatStyle::Yellow << ChatStyle::Bold << str;
					
					//	Log
					
					if (!client.IsNull()) str=String::Format(
						log,
						str,
						client->GetUsername()
					);
					
					server.WriteLog(
						str,
						Service::LogType::Information
					);
				
				}
				
				lock.Execute([&] () mutable {
				
					//	Enqueue worker
					Server::Get().Pool().Enqueue([=] () mutable {	worker_func(std::move(info));	});
					
					this->info=std::move(info);
					
				});
				
				return true;
			
			}
	
	
	};


}


static Nullable<Shutdown> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

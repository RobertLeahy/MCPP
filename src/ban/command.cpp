#include <ban/ban.hpp>
#include <chat/chat.hpp>
#include <command/command.hpp>
#include <permissions/permissions.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <utility>


using namespace MCPP;


static const String & name("Ban/Unban Commands");
static const Word priority=1;
static const String ban_identifier("ban");
static const String unban_identifier("unban");
static const String ban_summary("Bans a user.");
static const String unban_summary("Unbans a user.");
static const String ban_help(
	"Syntax: /ban [-q] <player name> [<reason>]\n"
	"Bans the player specified by \"player name\".\n"
	"If \"-q\" is specified, does not broadcast a notification that \"player name\" has been banned.\n"
);
static const String unban_help(
	"Syntax: /unban [-q] <player name>\n"
	"Unbans the player specified by \"player name\".\n"
	"If \"-q\" is specified, does not broadcast a notification that \"player name\" has been unbanned.\n"
);
static const Regex remove_group("^\\s*\\S+\\s+(?=\\S)");
static const String quiet_arg("-q");
static const String already_banned(" is already banned");
static const String banned(" was banned");
static const String unbanned(" was unbanned");
static const String not_banned(" was not banned");
static const String by(" by ");
static const String reason []={
	" with reason \"",
	"\""
};
static const String regex("^{0}");


class BanCommand : public Module, public Command {


	private:
	
	
		class ParseResult {
		
		
			public:
			
			
				//	True if this is a ban, false
				//	otherwise
				bool IsBan;
				//	Information about the ban to
				//	enact (or repeal)
				BanInfo Info;
				//	Quiet?
				bool Quiet;
				
				
				ParseResult () noexcept : Quiet(false) {	}
		
		};
	
	
		static bool is_ban (const String & identifier) {
		
			return identifier==ban_identifier;
		
		}
		
		
		static Nullable<ParseResult> parse (CommandEvent event) {
		
			Nullable<ParseResult> retr;
			
			ParseResult result;
			result.IsBan=is_ban(event.Identifier);
			
			//	At least one argument (player's name)
			//	is required for both unbanning and
			//	banning
			if (event.Arguments.Count()==0) return retr;
			
			//	The argument we're currently
			//	examining
			Word loc=0;
			
			//	Is this quiet?
			if (event.Arguments[loc]==quiet_arg) {
			
				++loc;
				
				result.Quiet=true;
			
			}
			
			//	Did we run out of arguments?
			if (loc==event.Arguments.Count()) return retr;
			
			//	Get player's name
			result.Info.Username=std::move(event.Arguments[loc]);
			++loc;
			
			//	Did we run out of arguments?
			if (loc!=event.Arguments.Count()) {
			
				//	A reason can only be supplied when
				//	banning, not when unbanning
				if (!result.IsBan) return retr;
				
				//	Remove the groups we've already
				//	parsed from the raw arguments
				for (Word i=0;i<loc;++i) event.RawArguments=remove_group.Replace(
					event.RawArguments,
					String()
				);
				
				//	Now what's left is the reason
				result.Info.Reason.Construct(std::move(event.RawArguments));
			
			}
			
			//	Get the name of the person
			//	issuing this command (if
			//	applicable)
			if (!event.Issuer.IsNull()) result.Info.By.Construct(event.Issuer->GetUsername());
			
			retr.Construct(std::move(result));
			return retr;
		
		}
		
		
		static void ban (ParseResult args, ChatMessage & message) {
		
			auto & info=args.Info;
		
			if (Bans::Get().Ban(info)) {
			
				//	User was banned
				
				message	<<	ChatStyle::Yellow
						<<	ChatStyle::Bold
						<<	info.Username
						<<	ChatFormat::Pop
						<<	banned;
						
				//	If not quiet, we add extra information
				//	if applicable, and then broadcast
				if (!args.Quiet) {
				
					if (!info.By.IsNull()) message	<<	by
													<<	ChatStyle::Bold
													<<	*info.By
													<<	ChatFormat::Pop;
													
					if (!info.Reason.IsNull()) message	<<	reason[0]
														<<	ChatStyle::Bold
														<<	*info.Reason
														<<	ChatFormat::Pop
														<<	reason[1];
				
					Chat::Get().Send(message);
					//	Blank message
					message=ChatMessage();
				
				}
			
			} else {
			
				//	User was already banned and therefore
				//	this command invocation did nothing
				
				message	<<	ChatStyle::Red
						<<	ChatStyle::Bold
						<<	info.Username
						<<	ChatFormat::Pop
						<<	already_banned;
			
			}
		
		}
		
		
		static void unban (ParseResult args, ChatMessage & message) {
		
			auto & info=args.Info;
			
			if (Bans::Get().Unban(
				info.Username,
				info.By
			)) {
			
				//	User was unbanned
				
				message	<<	ChatStyle::BrightGreen
						<<	ChatStyle::Bold
						<<	info.Username
						<<	ChatFormat::Pop
						<<	unbanned;
						
				//	If not quiet, we add extra information
				//	if applicable, and then broadcast
				if (!args.Quiet) {
				
					if (!info.By.IsNull()) message	<<	by
													<<	ChatStyle::Bold
													<<	*info.By;
													
					Chat::Get().Send(message);
					//	Blank message
					message=ChatMessage();
				
				}
			
			} else {
			
				//	User was not banned and therefore this
				//	command did nothing
				
				message	<<	ChatStyle::Red
						<<	ChatStyle::Bold
						<<	info.Username
						<<	ChatFormat::Pop
						<<	not_banned;
			
			}
		
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
				ban_identifier,
				this
			);
			commands.Add(
				unban_identifier,
				this
			);
		
		}
		
		
		virtual void Summary (const String & identifier, ChatMessage & message) override {
		
			message << (is_ban(identifier) ? ban_summary : unban_summary);
		
		}
		
		
		virtual void Help (const String & identifier, ChatMessage & message) override {
		
			message << (is_ban(identifier) ? ban_help : unban_help);
		
		}
		
		
		virtual Vector<String> AutoComplete (const CommandEvent & event) override {
		
			Vector<String> retr;
			
			if (
				//	If there's more than two arguments,
				//	the two arguments we could make suggestions
				//	based on are out the window, so just return
				//	no suggestions
				(event.Arguments.Count()>2) ||
				//	If there's exactly two arguments, and the
				//	first argument isn't the quiet arg, the
				//	first argument is the username, so we still
				//	can't make any suggestions
				(
					(event.Arguments.Count()==2) &&
					(event.Arguments[0]!=quiet_arg)
				)
			) return retr;
			
			//	Can we suggest the quiet arg?
			if (
				//	If there are no arguments, we
				//	definitely can
				(event.Arguments.Count()==0) ||
				//	Otherwise the first argument has
				//	to match the quiet arg in some way
				(
					(event.Arguments.Count()==1) &&
					Regex(
						String::Format(
							regex,
							Regex::Escape(event.Arguments[0])
						)
					).IsMatch(quiet_arg)
				)
			) retr.Add(quiet_arg);
			
			//	Loop over all clients that we can
			//	suggest
			for (auto & client : Server::Get().Clients.Get(
				event.Arguments[event.Arguments.Count()-1],
				ClientSearch::Begin
			)) if (client->GetState()==ProtocolState::Play) retr.Add(client->GetUsername());
			
			return retr;
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			return event.Issuer.IsNull() ? true : Permissions::Get().GetUser(event.Issuer).Check(event.Identifier);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
			
			//	Parse arguments
			auto args=parse(std::move(event));
			
			//	Report syntax errors
			if (args.IsNull()) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Command will succeed now
			retr.Status=CommandStatus::Success;
			
			//	Execute command
			if (args->IsBan) ban(
				std::move(*args),
				retr.Message
			);
			else unban(
				std::move(*args),
				retr.Message
			);
			
			return retr;
		
		}


};


INSTALL_MODULE(BanCommand)

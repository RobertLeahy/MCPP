#include <chat/chat.hpp>
#include <command/command.hpp>
#include <permissions/permissions.hpp>
#include <whitelist/whitelist.hpp>
#include <mod.hpp>


using namespace MCPP;


static const String name("Whitelist Command");
static const Word priority=1;
static const String identifier("whitelist");
static const String summary("Manipulates the whitelist system.");
static const String help(
	"Syntax: /whitelist [-q] [enable|disable|(add|remove <player name>)]\n"
	"If \"enable\" is specified, enables the whitelist.\n"
	"If \"disable\" is specified, disables the whitelist.\n"
	"If \"add\" or \"remove\" is specified, adds or removes (respectively) \"player name\" from the whitelist.\n"
	"Unless \"-q\" is specified, the action will be broadcast."
);
static const String quiet_arg("-q");
static const String add_arg("add");
static const String remove_arg("remove");
static const String enable_arg("enable");
static const String disable_arg("disable");
static const String already_enabled("Whitelist already enabled");
static const String already_disabled("Whitelist already disabled");
static const String already_added("{0} is already whitelisted");
static const String already_removed("{0} is not on the whitelist");
static const String by(" by {0}");
static const String enabled("Whitelist enabled");
static const String disabled("Whitelist disabled");
static const String added("{0} added to whitelist");
static const String removed("{0} removed from whitelist");


class WhitelistCommand : public Module, public Command {


	private:
	
	
		enum class Type {
		
			Enable,
			Disable,
			Add,
			Remove
		
		};
	
	
		class ParseResult {
		
		
			public:
			
			
				bool Quiet;
				Type Action;
				String Username;
				
				
				ParseResult () noexcept : Quiet(false) {	}
		
		
		};
		
		
		static Nullable<Type> get_type (const String & str) {
		
			if (str==add_arg) return Type::Add;
			if (str==remove_arg) return Type::Remove;
			if (str==disable_arg) return Type::Disable;
			if (str==enable_arg) return Type::Enable;
			
			return Nullable<Type>();
		
		}
		
		
		static Nullable<ParseResult> parse (CommandEvent event) {
		
			Nullable<ParseResult> retr;
			
			//	If there are more than three arguments,
			//	or zero arguments, it's always invalid
			//	syntax
			if (
				(event.Arguments.Count()==0) ||
				(event.Arguments.Count()>3)
			) return retr;
			
			ParseResult result;
			
			Word loc=0;
			
			//	Check to see if the first argument is
			//	the quiet arg
			if (event.Arguments[0]==quiet_arg) {
			
				result.Quiet=true;
				
				++loc;
			
			}
			
			//	Make sure we haven't run out of
			//	arguments
			if (event.Arguments.Count()==loc) return retr;
			
			//	Check for an action
			auto type=get_type(event.Arguments[loc++]);
			if (type.IsNull()) return retr;
			result.Action=*type;
			
			//	Check argument count
			if (
				(result.Action==Type::Add) ||
				(result.Action==Type::Remove)
			) {
			
				if (event.Arguments.Count()!=(loc+1)) return retr;
				
				result.Username=std::move(
					event.Arguments[event.Arguments.Count()-1]
				);
			
			} else if (event.Arguments.Count()!=loc) return retr;
			
			return result;
		
		}
		
		
		static void success (ChatMessage & message, String str) {
		
			message <<	ChatStyle::Bold
					<<	ChatStyle::BrightGreen
					<<	std::move(str);
		
		}
		
		
		static void error (ChatMessage & message, String str) {
		
			message	<<	ChatStyle::Bold
					<<	ChatStyle::Red
					<<	std::move(str);
		
		}
		
		
		static void add_by (SmartPointer<Client> client, ChatMessage & message) {
		
			if (!client.IsNull()) message << String::Format(
				by,
				client->GetUsername()
			);
		
		}
		
		
		static bool enable (SmartPointer<Client> client, bool quiet, ChatMessage & message) {
		
			if (Whitelist::Get().Enable()) {
			
				success(
					message,
					enabled
				);
				
				if (!quiet) add_by(std::move(client),message);
				
				return true;
			
			}
			
			error(
				message,
				already_enabled
			);
			
			return false;
		
		}
		
		
		static bool disable (SmartPointer<Client> client, bool quiet, ChatMessage & message) {
		
			if (Whitelist::Get().Disable()) {
			
				success(
					message,
					disabled
				);
				
				if (!quiet) add_by(std::move(client),message);
				
				return true;
			
			}
			
			error(
				message,
				already_disabled
			);
			
			return false;
		
		}
		
		
		static bool add (SmartPointer<Client> client, String username, bool quiet, ChatMessage & message) {
		
			if (Whitelist::Get().Add(username)) {
			
				success(
					message,
					String::Format(
						added,
						username
					)
				);
				
				if (!quiet) add_by(std::move(client),message);
				
				return true;
			
			}
			
			error(
				message,
				String::Format(
					already_added,
					username
				)
			);
			
			return false;
		
		}
		
		
		static bool remove (SmartPointer<Client> client, String username, bool quiet, ChatMessage & message) {
		
			if (Whitelist::Get().Remove(username)) {
			
				success(
					message,
					String::Format(
						removed,
						username
					)
				);
				
				if (!quiet) add_by(std::move(client),message);
				
				return true;
			
			}
			
			error(
				message,
				String::Format(
					already_removed,
					username
				)
			);
			
			return false;
		
		}


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			Commands::Get().Add(
				identifier,
				this
			);
		
		}
		
		
		virtual void Summary (const String &, ChatMessage & message) override {
		
			message << summary;
		
		}
		
		
		virtual void Help (const String &, ChatMessage & message) override {
		
			message << help;
		
		}
		
		
		virtual Vector<String> AutoComplete (const CommandEvent & event) override {
		
			//	TODO: Implement
			return Vector<String>();
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			return event.Issuer.IsNull() ? true : Permissions::Get().GetUser(event.Issuer).Check(identifier);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
			
			//	Save issuer
			auto issuer=event.Issuer;
			
			//	Parse arguments
			auto result=parse(std::move(event));
			if (result.IsNull()) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Success is guaranteed now
			retr.Status=CommandStatus::Success;
			
			bool success;
			switch (result->Action) {
			
				case Type::Enable:
					success=enable(
						std::move(issuer),
						result->Quiet,
						retr.Message
					);
					break;
				case Type::Disable:
					success=disable(
						std::move(issuer),
						result->Quiet,
						retr.Message
					);
					break;
				case Type::Add:
					success=add(
						std::move(issuer),
						std::move(result->Username),
						result->Quiet,
						retr.Message
					);
					break;
				case Type::Remove:
					success=remove(
						std::move(issuer),
						std::move(result->Username),
						result->Quiet,
						retr.Message
					);
					break;
				default:
					//	This shouldn't happen
					retr.Status=CommandStatus::SyntaxError;
					return retr;
			
			}
			
			//	Broadcast if applicable
			if (success && !result->Quiet) {
			
				Chat::Get().Send(retr.Message);
				
				//	Clear message so nothing is
				//	returned to the issuer
				retr.Message=ChatMessage();
			
			}
			
			return retr;
		
		}


};


INSTALL_MODULE(WhitelistCommand)

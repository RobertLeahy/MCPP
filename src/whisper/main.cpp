#include <rleahylib/rleahylib.hpp>
#include <command/command.hpp>
#include <chat/chat.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <utility>


using namespace MCPP;


static const Word priority=1;
static const String name("Whisper Support");
static const String identifier("w");
static const String summary("Sends a private message to another player.");
static const String help(
	"Syntax: /w <player name> <message>\n"
	"Sends a private message to <player name>.\n"
	"Note that this message will still be logged."
);
static const Regex extract_message("^\\s*\\S+\\s+(\\S.*)$");
static const String dne("User \"{0}\" does not exist");


class Whisper : public Module, public Command {


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
		
			Vector<String> retr;
			
			//	If there's more than one argument,
			//	there's no autocompletions that can
			//	be made
			if (event.Arguments.Count()>1) return retr;
			
			auto & clients=Server::Get().Clients;
			
			//	If there's no arguments, the autocompletions
			//	is the name of every logged in player
			if (event.Arguments.Count()==0) {
			
				for (auto & client : clients) if (
					(client!=event.Issuer) &&
					(client->GetState()==ProtocolState::Play)
				) retr.Add(client->GetUsername());
				
				return retr;
			
			}
			
			//	If there's just the one argument, every
			//	player whose name matches is an autocompletion
			for (auto & client : clients.Get(
				event.Arguments[0],
				ClientSearch::Begin
			)) retr.Add(client->GetUsername());
			
			return retr;
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
		
			//	There needs to be more than two arguments
			if (event.Arguments.Count()<=1) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Extract the message
			auto match=extract_message.Match(event.RawArguments);
			//	If a message could not be extracted,
			//	the syntax of the command was incorrect
			if (!match.Success()) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Command will always succeed now
			retr.Status=CommandStatus::Success;
			
			//	Prepare a whisper
			ChatMessage whisper(
				event.Issuer,
				event.Arguments[0],
				match[1].Value()
			);
			
			auto & chat=Chat::Get();
			
			//	Attempt to send
			if (chat.Send(whisper).Count()==0) {
			
				//	Delivery success -- could not be
				//	delivered to zero recipients (meaning
				//	intended recipient exists)
				
				//	Send back to original sender, unless
				//	the original sender is the server, in
				//	which case it'll show up in the chat
				//	log
				if (!event.Issuer.IsNull()) {
				
					whisper.Echo=true;
					
					chat.Send(whisper);
				
				}
			
			} else {
			
				//	Delivery failure -- could not be delivered
				//	to at least one recipient (meaning intended
				//	recipient does not exist).
				
				retr.Message	<<	ChatStyle::Bold
								<<	ChatStyle::Red
								<<	String::Format(
										dne,
										event.Arguments[0]
									);
			
			}
			
			return retr;
		
		}


};


INSTALL_MODULE(Whisper)

#include <command/command.hpp>
#include <chat/chat.hpp>


static const Word priority=1;
static const String name("Whisper Support");
static const String identifier("w");
static const String summary("Sends a private message to another player.");
static const String help(
	"Syntax: /w <player name> <message>\n"
	"Sends a private message to <player name>.\n"
	"Note that this message will still be logged."
);
static const Regex whitespace("\\s");
static const Regex parse("^([^\\s]+)\\s+(.+)$");


class Whisper : public Module, public Command {


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
		
		
		virtual const String & Summary () const noexcept override {
		
			return summary;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual Vector<String> AutoComplete (const String & args) const override {
		
			Vector<String> retr;
		
			//	Is there whitespace in the args
			//	listing?
			//
			//	If so a username has been specified
			//	and there's nothing we can do
			if (whitespace.IsMatch(args)) return retr;
			
			//	The list of completions is the
			//	list of users whose usernames
			//	begin with the specified string
			auto matches=Server::Get().Clients.Get(
				args,
				ClientSearch::Begin
			);
			
			//	Extract usernames
			for (auto & client : matches) retr.Add(client->GetUsername());
			
			return retr;
		
		}
		
		
		virtual bool Execute (SmartPointer<Client> client, const String & args, ChatMessage & message) override {
		
			//	Attempt to parse args
			auto match=parse.Match(args);
			
			if (!match.Success()) return false;
			
			//	Prepare a message
			ChatMessage whisper(
				client,
				match[1].Value(),
				match[2].Value()
			);
			
			//	Attempt to send
			if (Chat::Get().Send(whisper).Count()==0) {
			
				//	Delivery success -- could not be
				//	delivered to zero recipients (meaning
				//	intended recipient exists)
				
				//	Send it back to original sender
				
				//	If the original sender is the server,
				//	it'll show up in the chat log, so don't
				//	echo
				if (!client.IsNull()) {
				
					whisper.Echo=true;
					
					message=std::move(whisper);
					
				}
			
			} else {
			
				//	Delivery failure -- could not be
				//	delivered to at least one recipient
				//	(meaning intended recipient does not
				//	exist).
				
				//	Transmit error to caller
				message	<< ChatStyle::Red
						<< ChatStyle::Bold
						<< "User \""
						<< match[1].Value()
						<< "\" does not exist";
			
			}
			
			return true;
		
		}


};


INSTALL_MODULE(Whisper)

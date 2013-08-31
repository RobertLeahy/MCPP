#include <command/command.hpp>
#include <chat/chat.hpp>


static const Word priority=1;
static const String name("Whisper Support");
static const String identifier("w");
static const String summary("Sends a private message to another player.");
static const String help(
	"Syntax: /w <player name> <message>\n"
	"Sends a private message to <player name>.\n"
	"Note that this message will still be logged, so don't send anything you wouldn't want server administrators to read."
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
		
			Commands->Add(this);
		
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
		
		
		virtual bool Check (SmartPointer<Client> client) const override {
		
			//	Everyone can send whispers
			return true;
		
		}
		
		
		virtual Vector<String> AutoComplete (const String & args) const override {
		
			Vector<String> retr;
		
			//	Is there whitespace in the args
			//	listing?
			//
			//	If so a username has been specified
			//	and there's nothing we can do
			if (whitespace.IsMatch(args)) return retr;
			
			//	Create a regex to match only users
			//	whose username begins with the
			//	string we were passed
			Regex regex(
				String::Format(
					"^{0}",
					Regex::Escape(
						args
					)
				),
				//	Usernames are not case sensitive
				RegexOptions().SetIgnoreCase()
			);
			
			RunningServer->Clients.Scan([&] (const SmartPointer<Client> & client) {
			
				//	Only authenticated users are valid
				//	chat targets
				if (
					(client->GetState()==ClientState::Authenticated) &&
					regex.IsMatch(client->GetUsername())
				) {
				
					retr.Add(
						//	Normalize to lower case
						client->GetUsername().ToLower()
					);
				
				}
			
			});
			
			return retr;
		
		}
		
		
		virtual bool Execute (SmartPointer<Client> client, const String & args, ChatMessage & message) override {
		
			//	Attempt to parse args
			auto match=parse.Match(args);
			
			if (!match.Success()) return false;
			
			//	Prepare a message
			ChatMessage whisper;
			whisper.To.Add(match[1].Value());
			whisper.From=std::move(client);
			whisper << match[2].Value();
			
			//	Attempt to send
			if (Chat->Send(whisper).Count()==0) {
			
				//	Delivery success -- could not be
				//	delivered to zero recipients (meaning
				//	intended recipient exists)
				
				//	Send it back to original sender
				whisper.Echo=true;
				
				message=std::move(whisper);
			
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


static Nullable<Whisper> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

#include <command/command.hpp>
#include <op/op.hpp>
#include <chat/chat.hpp>


static const String name("Kick Command");
static const Word priority=1;
static const String identifier("kick");
static const String summary("Kicks a player.");
static const String help(
	"Syntax: /kick <player name>\n"
	"Kicks <player name>, disconnecting them from the server."
);


class Kick : public Module, public Command {


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
		
		
		virtual bool Check (SmartPointer<Client> client) const override {
		
			//	Only ops can kick
			return Ops::Get().IsOp(client->GetUsername());
		
		}
		
		
		virtual Vector<String> AutoComplete (const String & args) const override {
		
			Vector<String> retr;
			
			//	Create a regex to filter users
			//	so we only select users that have
			//	a username which begins with the
			//	passed string
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
			
			Server::Get().Clients.Scan([&] (const SmartPointer<Client> & client) {
			
				//	Only authenticated users are
				//	valid chat targets
				if (client->GetState()==ClientState::Authenticated) {
				
					//	Get client's username
					String username(client->GetUsername());
					
					//	Only autocomplete if the username
					//	matches the regex
					if (regex.IsMatch(username)) {
					
						//	Normalize to lower case
						username.ToLower();
						
						//	Add
						retr.Add(std::move(username));
					
					}
				
				}
			
			});
			
			return retr;
		
		}
		
		
		virtual bool Execute (SmartPointer<Client> client, const String & args, ChatMessage & message) override {
		
			//	No player name, syntax error
			if (args.Size()==0) return false;
		
			//	Normalize usernames to lowercase
			String target(args.ToLower());
			Nullable<String> caller;
			if (!client.IsNull()) caller.Construct(client->GetUsername().ToLower());
			
			//	Generate kick message
			String reason("Kicked");
			if (!caller.IsNull()) reason << " by " << *caller;
			//	Generate kick packet
			typedef PacketTypeMap<0xFF> pt;
			Packet packet;
			packet.SetType<pt>();
			packet.Retrieve<pt,0>()=reason;
			
			bool found=false;
			
			//	Scan
			Server::Get().Clients.Scan([&] (SmartPointer<Client> & client) {
			
				if (
					(client->GetState()==ClientState::Authenticated) &&
					(client->GetUsername().ToLower()==target)
				) {
				
					found=true;
					
					client->Send(packet)->AddCallback([=] (SendState) mutable {
					
						client->Disconnect(std::move(reason));
					
					});
				
				}
			
			});
			
			if (!(found || client.IsNull())) {

				message	<< ChatStyle::Red
						<< ChatStyle::Bold
						<< "Player \""
						<< target
						<< "\" could not be found";
			
			}
			
			return true;
		
		}


};


static Nullable<Kick> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

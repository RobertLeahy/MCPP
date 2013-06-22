#include <common.hpp>
#include <chat/chat.hpp>
#include <op/op.hpp>
#include <utility>


static const Word priority=2;
static const String name("Kick Support");
static const Regex kick_regex(
	"^\\/kick\\s+(.+)$",
	RegexOptions().SetIgnoreCase().SetSingleline()
);
static const String kick_reason_template("Kicked by {0}");


class Kick : public Module {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	Extract current chat handler
			ChatHandler prev(std::move(Chat->Chat));
			
			//	Install our handler
			Chat->Chat=[=] (SmartPointer<Client> client, const String & message) {
			
				//	Check for match
				auto match=kick_regex.Match(message);
				auto username=client->GetUsername();
				
				//	Proceed only if it's a match
				//	and the user is an op
				if (
					match.Success() &&
					Ops->IsOp(username)
				) {
				
					auto target=match[1].Value().ToLower();
				
					//	Scan user list and attempt
					//	to find connected user with
					//	a matching username
					bool found=false;
					RunningServer->Clients.Scan([&] (SmartPointer<Client> & u) {
					
						//	Only clients who are authenticated
						//	are considered to have a username
						if (
							(u->GetState()==ClientState::Authenticated) &&
							(u->GetUsername().ToLower()==target)
						) {
						
							//	Kick
							u->Disconnect(
								String::Format(
									kick_reason_template,
									username
								)
							);
							
							found=true;
						
						}
					
					});
					
					if (!found) {
					
						ChatMessage message;
						message.To.Add(std::move(username));
						message	<<	ChatColour::Red
								<<	ChatStyle::Bold
								<<	ChatFormat::Label
								<<	ChatFormat::LabelSeparator
								<<	ChatFormat::PopStyle
								<<	" player "
								<<	ChatStyle::Bold
								<<	target
								<<	ChatFormat::PopStyle
								<<	" could not be found";
								
						Chat->Send(message);
					
					}
					
					//	Do not carry on
					return;
				
				}
				
				//	Chain
				if (prev) prev(
					std::move(client),
					message
				);
			
			};
		
		}


};


static Nullable<Kick> kick;


extern "C" {


	Module * Load () {
	
		if (kick.IsNull()) kick.Construct();
		
		return &(*kick);
	
	}
	
	
	void Unload () {
	
		kick.Destroy();
	
	}


}

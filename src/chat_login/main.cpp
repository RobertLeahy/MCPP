#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>
#include <unordered_map>


namespace MCPP {


	static const String name("Chat Login/Logout Broadcast Support");
	static const Word priority=1;
	static const String online(" has come online");
	static const String offline(" has gone offline");
	static const String start_reason(" (");
	static const String end_reason(")");
	
	
	class LoginLogout : public Module {
	
	
		private:
		
		
			std::unordered_map<const Connection *,SmartPointer<Client>> map;
			Mutex map_lock;
	
	
		public:
		
		
			virtual const String & Name () const noexcept override {
			
				return name;
			
			}
			
			
			virtual Word Priority () const noexcept override {
			
				return priority;
			
			}
			
			
			virtual void Install () override {
			
				//	Subscribe to the onlogin
				//	event within the server
				RunningServer->OnLogin.Add([] (SmartPointer<Client> client) {
				
					ChatMessage message;
					message.Message.EmplaceBack(ChatStyle::Bold);
					message.Message.EmplaceBack(ChatColour::BrightGreen);
					message.Message.EmplaceBack(client->GetUsername());
					message.Message.EmplaceBack(ChatFormat::PopStyle);
					message.Message.EmplaceBack(online);
					
					Chat->Send(message);
				
				});
				
				//	Subscribe to the ondisconnect
				//	event within the server
				RunningServer->OnDisconnect.Add([] (SmartPointer<Client> client, const String & reason) {
				
					//	Only proceed if the user was
					//	authenticated
					if (client->GetState()==ClientState::Authenticated) {
					
						ChatMessage message;
						message.Message.EmplaceBack(ChatStyle::Bold);
						message.Message.EmplaceBack(ChatColour::Red);
						message.Message.EmplaceBack(client->GetUsername());
						message.Message.EmplaceBack(ChatFormat::PopStyle);
						message.Message.EmplaceBack(offline);
						
						//	Add reason if there was
						//	a reason associated with
						//	the disconnect
						if (reason.Size()!=0) {
						
							message.Message.EmplaceBack(start_reason);
							message.Message.EmplaceBack(ChatStyle::Bold);
							message.Message.EmplaceBack(reason);
							message.Message.EmplaceBack(ChatFormat::PopStyle);
							message.Message.EmplaceBack(end_reason);
						
						}
						
						Chat->Send(message);
					
					}
				
				});
			
			}
	
	
	};


}


static Nullable<LoginLogout> login_logout;


extern "C" {


	Module * Load () {
	
		try {
		
			if (login_logout.IsNull()) login_logout.Construct();
			
			return &(*login_logout);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		login_logout.Destroy();
	
	}


}

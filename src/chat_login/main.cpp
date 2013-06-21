#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>
#include <unordered_map>


namespace MCPP {


	static const String name("Chat Login/Logout Broadcast Support");
	static const Word priority=1;
	static const String login_template("§a§l{0}§r§a has come online");
	static const String logout_template(
		"§c§l"	//	Bold and red
		"{0}"	//	Player's username
		"§r§c"	//	Not-bold and red
		" has gone offline"
	);
	static const String logout_reason_template(
		"§c§l"	//	Bold and red
		"{{0}}"	//	Player's username (but not during first substitution)
		"§r§c"	//	Not-bold and red
		" has gone offline ("
		"§c§l"	//	Bold and red
		"{0}"	//	reason
		"§r§c"	//	Not-bold and red
		")"
	);
	
	
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
				
					//	Broadcast
					Chat->Broadcast(
						ChatModule::Sanitize(
							String::Format(
								login_template,
								client->GetUsername()
							),
							false
						)
					);
				
				});
				
				//	Subscribe to the ondisconnect
				//	event within the server
				RunningServer->OnDisconnect.Add([] (SmartPointer<Client> client, const String & reason) {
				
					//	Only proceed if the user was
					//	authenticated
					if (client->GetState()==ClientState::Authenticated) {
					
						//	Generate message
						String broadcast;
						if (reason.Size()==0) {
						
							//	No reason
							broadcast=String::Format(
								logout_template,
								client->GetUsername()
							);
						
						} else {
						
							//	Reason
							broadcast=String::Format(
								String::Format(
									logout_reason_template,
									reason
								),
								client->GetUsername()
							);
						
						}
						
						//	Broadcast
						Chat->Broadcast(
							ChatModule::Sanitize(
								std::move(broadcast),
								false
							)
						);
					
					}
				
				});
			
			}
	
	
	};


}


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		if (mod_ptr==nullptr) try {
		
			mod_ptr=new LoginLogout();
		
		} catch (...) {	}
		
		return mod_ptr;
	
	}
	
	
	void Unload () {
	
		if (mod_ptr!=nullptr) {
		
			delete mod_ptr;
			
			mod_ptr=nullptr;
		
		}
	
	}


}

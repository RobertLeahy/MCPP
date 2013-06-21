#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>


namespace MCPP {


	static const String name("Basic Chat Support");
	static const Word priority=1;
	static const String chat_template("§l{0}:§r {1}");
	static const String chat_log_template("[{0}] says: {1}");
	
	
	class BasicChat : public Module {
	
	
		public:
		
		
			virtual const String & Name () const noexcept override {
			
				return name;
			
			}
			
			
			virtual Word Priority () const noexcept override {
			
				return priority;
			
			}
			
			
			virtual void Install () override {
				
				//	Install our chat handler
				Chat->Chat=[] (SmartPointer<Client> client, const String & message) {
				
					//	Grab the user's username
					String username(client->GetUsername());
					
					//	Log the message
					RunningServer->WriteLog(
						String::Format(
							chat_log_template,
							username,
							message
						),
						Service::LogType::Information
					);
					
					//	Send the message
					Chat->Broadcast(
						ChatModule::Sanitize(
							String::Format(
								chat_template,
								std::move(username),
								message
							),
							false
						)
					);
				
				};
			
			}
	
	
	};


}


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		if (mod_ptr==nullptr) try {
		
			mod_ptr=new BasicChat();
		
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

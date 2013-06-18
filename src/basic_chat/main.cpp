#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>


namespace MCPP {


	static const String name("Basic Chat Support");
	static const Word priority=1;
	static const String chat_template("<{0}> {1}");
	
	
	class BasicChat : public Module {
	
	
		public:
		
		
			virtual const String & Name () const noexcept override {
			
				return name;
			
			}
			
			
			virtual Word Priority () const noexcept override {
			
				return priority;
			
			}
			
			
			virtual void Install () override {
			
				//	Extract previous chat handler
				ChatHandler prev(
					std::move(
						Chat->Chat
					)
				);
				
				//	Install our chat handler
				Chat->Chat=[=] (SmartPointer<Client> client, const String & message) {
				
					RunningServer->WriteLog(
						message,
						Service::LogType::Information
					);
				
					String outbound(
						String::Format(
							chat_template,
							client->GetUsername(),
							message
						)
					);
					
					Chat->Broadcast(
						ChatModule::Sanitize(
							std::move(outbound),
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

#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>


namespace MCPP {


	static const String name("Basic Chat Support");
	static const Word priority=1;
	
	
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
				
					//	Send
					Chat->Send(
						ChatMessage(
							std::move(client),
							message
						)
					);
				
				};
			
			}
	
	
	};


}


static Nullable<BasicChat> basic_chat;


extern "C" {


	Module * Load () {
	
		try {
		
			if (basic_chat.IsNull()) basic_chat.Construct();
			
			return &(*basic_chat);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		basic_chat.Destroy();
	
	}


}

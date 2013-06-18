#include <common.hpp>
#include <utility>


namespace MCPP {


	static const String name("Disconnect (0xFF) Support");
	static const Word priority=1;
	
	
	class Disconnect : public Module {
	
	
		public:
		
		
			virtual const String & Name () const noexcept override {
			
				return name;
			
			}
			
			
			virtual Word Priority () const noexcept override {
			
				return priority;
			
			}
			
			
			virtual void Install () override {
			
				//	Old packet handler
				PacketHandler prev(
					std::move(
						RunningServer->Router[0xFF]
					)
				);
				
				//	New handler
				RunningServer->Router[0xFF]=[=] (SmartPointer<Client> client, Packet packet) {
				
					typedef PacketTypeMap<0xFF> pt;
				
					//	If there's nothing to chain to
					//	we actually don't need to
					//	avoid mutating the packet, so
					//	we can make the call to
					//	disconnect slightly more
					//	efficient by moving the
					//	string rather than copying
					//	it
					if (prev) {
					
						//	COPY STRING AND CHAIN
					
						client->Disconnect(packet.Retrieve<pt,0>());
						
						prev(
							std::move(client),
							std::move(packet)
						);
					
					} else {
					
						//	MOVE STRING AND DO NOT CHAIN
						
						client->Disconnect(
							std::move(
								packet.Retrieve<pt,0>()
							)
						);
					
					}
				
				};
			
			}
	
	
	};


}


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		if (mod_ptr==nullptr) try {
		
			mod_ptr=new Disconnect();
		
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

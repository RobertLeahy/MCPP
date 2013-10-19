#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <server.hpp>


using namespace MCPP;


static const String name("Handshake Handler");
static const Word priority=1;


class Handshake : public Module {


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			Server::Get().Router(0x00,ProtocolState::Handshaking)=[] (ReceiveEvent event) {
			
				auto & packet=event.Data.Get<Packets::Handshaking::Serverbound::Handshake>();
				
				event.From->SetState(packet.CurrentState);
				
				Server::Get().WriteLog(
					"Handshake",
					Service::LogType::Information
				);
			
			};
		
		}


};


INSTALL_MODULE(Handshake)

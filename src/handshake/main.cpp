#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <server.hpp>


using namespace MCPP;


static const String name("Handshake Handler");
static const Word priority=1;


class Handshake : public Module {


	private:
	
	
		typedef Packets::Handshaking::Serverbound::Handshake handshake;


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			Server::Get().Router(
				handshake::PacketID,
				ProtocolState::Handshaking
			)=[] (PacketEvent event) {
			
				auto & packet=event.Data.Get<handshake>();
				
				event.From->SetState(packet.CurrentState);
			
			};
		
		}


};


INSTALL_MODULE(Handshake)

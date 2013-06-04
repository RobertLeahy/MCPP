#include <packet_router.hpp>


namespace MCPP {


	static const String packet_dne="Packet 0x{0} has no recognized handler";
	
	
	inline void PacketRouter::destroy () noexcept {
	
		for (Word i=0;i<std::numeric_limits<Byte>::max();++i) {
		
			routes[i].~PacketHandler();
		
		}
	
	}
	
	
	inline void PacketRouter::init () noexcept {
	
		for (Word i=0;i<std::numeric_limits<Byte>::max();++i) {
		
			new (&routes[i]) PacketHandler ();
		
		}
	
	}


	PacketRouter::PacketRouter (bool ignore_dne) noexcept : ignore_dne(ignore_dne) {
	
		init();
	
	}
	
	
	PacketRouter::~PacketRouter () noexcept {
	
		destroy();
	
	}
	
	
	PacketHandler & PacketRouter::operator [] (Byte type) noexcept {
	
		return routes[type];
	
	}
	
	
	void PacketRouter::operator () (SmartPointer<Client> client, Packet && packet) const {
	
		//	Check that a valid route exists
		if (routes[packet.Type()]) {
		
			//	Dispatch
			routes[packet.Type()](
				std::move(client),
				std::move(packet)
			);
		
		//	No valid route and we're
		//	supposed to kick offenders
		} else if (!ignore_dne) {
		
			String reason;
			try {
			
				reason=String::Format(
					packet_dne,
					String(packet.Type(),16)
				);
			
			} catch (...) {
			
				//	We need to make sure this gets
				//	done...
				client->Disconnect(reason);
				
				throw;
			
			}
			
			client->Disconnect(reason);
		
		}
	
	}
	
	
	void PacketRouter::Clear () noexcept {
	
		destroy();
		
		init();
	
	}


}

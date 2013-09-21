#include <common.hpp>
#include <utility>


static const String protocol_error("Protocol error");
static const String name("Ping Support");
static const Word priority=1;
static const String ping_template("{0}:{1} pinged");


class ServerListPing : public Module {


	public:
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			PacketHandler prev(std::move(Server::Get().Router[0xFE]));
		
			Server::Get().Router[0xFE]=[=] (SmartPointer<Client> client, Packet packet) {
			
				//	Is this client in the right state?
				if (client->GetState()!=ClientState::Connected) {
				
					//	Not for us, chain through if possible
					if (prev) prev(
						std::move(client),
						std::move(packet)
					);
					//	Nothing to chain through to, kill the
					//	client
					else client->Disconnect(protocol_error);
				
					return;
				
				}
				
				//	Log the fact that the client
				//	pinged
				Server::Get().WriteLog(
					String::Format(
						ping_template,
						client->IP(),
						client->Port()
					),
					Service::LogType::Information
				);
				
				//	Make a version number string
				String ver_num(MinecraftMajorVersion);
				ver_num << "." << String(MinecraftMinorVersion);
				if (MinecraftSubminorVersion!=0) ver_num << "." << String(MinecraftSubminorVersion);
			
				//	Prepare the packet
				GraphemeCluster null_char('\0');
				Packet reply;
				reply.SetType<PacketTypeMap<0xFF>>();
				reply.Retrieve<String>(0)	<< "§1"
											<<	null_char
											<<	String(ProtocolVersion)
											<<	null_char
											<<	ver_num
											<<	null_char
											<<	Server::Get().GetMessageOfTheDay()
											<<	null_char
											<<	String(Server::Get().Clients.AuthenticatedCount())
											<<	null_char
											<< 	String(
													//	Minecraft client probably doesn't understand
													//	that 0 max players = unlimited, so we
													//	try and transform that into something the
													//	Minecraft client can understand
													(Server::Get().MaximumPlayers==0)
															//	Which probably isn't this but at least
															//	we tried...
														?	std::numeric_limits<Word>::max()
														:	Server::Get().MaximumPlayers
												);
				
				//	Send and attach
				//	callback to disconnect
				//	client after
				//	the packet is on
				//	the wire
				client->Send(reply)->AddCallback([=] (SendState) mutable {	client->Disconnect();	});
		
				//	Chain to previous callback
				//	if it exists
				if (prev) prev(
					std::move(client),
					std::move(packet)
				);
			
			};
		
		}


};


static Module * mod_ptr=nullptr;


extern "C" {
	
	
	Module * Load () {

		try {
		
			if (mod_ptr==nullptr) mod_ptr=new ServerListPing();
		
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

#include <common.hpp>
#include <utility>


static const String motd_setting("motd");
static const String protocol_error("Protocol error");
static const String name("0xFE Server List Ping Protocol Support");
static const Word priority=1;


class ServerListPing : public Module {


	public:
	
	
		ServerListPing () noexcept {	}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			PacketHandler prev(std::move(RunningServer->Router[0xFE]));
		
			RunningServer->Router[0xFE]=[=] (SmartPointer<Client> client, Packet packet) {
			
				try {
				
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
					
					//	Make a version number string
					String ver_num(MinecraftMajorVersion);
					ver_num << "." << MinecraftMinorVersion;
					if (MinecraftSubminorVersion!=0) ver_num << "." << MinecraftSubminorVersion;
				
					//	Prepare a string
					GraphemeCluster null_char('\0');
					String ping_str("§1");
					ping_str	<<	null_char
								<<	ProtocolVersion
								<<	null_char
								<<	ver_num
								<<	null_char
								<<	RunningServer->GetMessageOfTheDay()
								<<	null_char
								<<	RunningServer->Clients.AuthenticatedCount()
								<<	null_char
								<< 	(
										//	Minecraft client probably doesn't understand
										//	that 0 max players = unlimited, so we
										//	try and transform that into something the
										//	Minecraft client can understand
										(RunningServer->MaximumPlayers==0)
												//	Which probably isn't this but at least
												//	we tried...
											?	std::numeric_limits<Word>::max()
											:	RunningServer->MaximumPlayers
									);
			
					//	Create packet
					Packet reply;
					reply.SetType<PacketTypeMap<0xFF>>();
					reply.Retrieve<String>(0)=std::move(ping_str);
					
					//	Send
					auto handle=client->Send(reply.ToBytes());
					
					//	Queue up disconnect
					handle->AddCallback([=] (SendState) mutable {	client->Disconnect();	});
			
					//	Chain to previous callback
					if (prev) prev(
						std::move(client),
						std::move(packet)
					);
					
				} catch (...) {
				
					RunningServer->Panic();
				
				}
			
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

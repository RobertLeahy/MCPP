#include <common.hpp>
#include <utility>
#include <limits>


static const String motd_setting("motd");
static const String protocol_error("Protocol error");


class ServerListPing : public Module {


	public:
	
	
		ServerListPing () noexcept {	}
		
		
		virtual Word Priority () const noexcept override {
		
			return 1;
		
		}
		
		
		virtual void Install () override {
		
			PacketHandler prev(std::move(RunningServer->Router[0xFE]));
		
			RunningServer->Router[0xFE]=[=] (SmartPointer<Client> && client, Packet && packet) {
			
				try {
				
					//	Is this client in the right state?
					if (client->GetState()!=ClientState::Connected) {
					
						client->Disconnect(protocol_error);
					
						return;
					
					}
				
					//	Get MotD
					Nullable<String> motd=RunningServer->Data().GetSetting(motd_setting);
					
					//	Make a version number string
					String ver_num(MinecraftMajorVersion);
					ver_num << "." << MinecraftMinorVersion;
					if (MinecraftSubminorVersion!=0) ver_num << "." << MinecraftSubminorVersion;
				
					//	Prepare a string
					GraphemeCluster null_char('\0');
					String ping_str("§1");
					ping_str	<< null_char
								<< ProtocolVersion
								<< null_char
								<< ver_num
								<< null_char
								<< (motd.IsNull() ? String("") : *motd)
								<< null_char
								<< RunningServer->MaximumPlayers==0 ? std::numeric_limits<Word>::max() : RunningServer->MaximumPlayers;
			
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



static const char * module_name="0xFE Server List Ping Protocol Support";
static ServerListPing * mod_ptr=nullptr;


extern "C" {


	const char * ModuleName () {
	
		return module_name;
	
	}
	
	
	Module * Load () {

		try {
		
			if (mod_ptr!=nullptr) mod_ptr=new ServerListPing();
		
		} catch (...) {
		
			return nullptr;
		
		}
		
		return mod_ptr;
	
	}
	
	
	void Unload () {
	
		if (mod_ptr!=nullptr) {
		
			delete mod_ptr;
		
			mod_ptr=nullptr;
		
		}
	
	}
	
	
}

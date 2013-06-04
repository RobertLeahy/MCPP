#include <common.hpp>
#include <utility>
#include <limits>


static const String motd_setting("motd");
static const String max_players_setting("max_players");
static const String protocol_error("Protocol error");


class ServerListPing : public Module {


	public:
	
	
		ServerListPing () noexcept {	}
		
		
		virtual Word Priority () const noexcept override {
		
			return 1;
		
		}
		
		
		virtual void Install () override {
		
			PackeHandler prev(std::move(RunningServer->Router[0xFE]));
		
			RunningServer->Router[0xFE]=[=] (SmartPointer<Client> && client, Packet && packet) {
			
				try {
				
					//	Is this client in the right state?
					if (client->GetState()!=ClientState::Connected) {
					
						client->Disconnect(protocol_error);
					
						return;
					
					}
				
					//	Get MotD
					Nullable<String> motd=RunningServer->Data().GetSetting(motd_setting);
					//	Get max players
					Nullable<String> max_players=RunningServer->Data().GetSetting(max_players_setting);
					
					Word max_players_num;
					if (
						max_players.IsNull() ||
						!max_players->ToInteger(&max_players_num)
					) max_players_num=std::numeric_limits<Word>::max();
					
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
								<< (motd.IsNull() ? String("") : motd)
								<< null_char
								<< max_players_num;
					ping_str << GraphemeCluster('\0')
			
					//	Create packet
					Packet reply;
					reply.SetType<PacketTypeMap<0xFF>>();
					reply.Retrieve(0)=std::move(ping_str);
					
					//	Send
					auto handle=client->Send(reply.ToBytes());
					
					//	Queue up disconnect
					handle.AddCallback([=] (ClientState) {	client->Disconnect();	});
			
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
		
		
		virtual void Destroy () noexcept override {	}


};


extern "C" {


	const char * ModuleName="0xFE Server List Ping Protocol Support";
	
	
	Module * Load () {

		return new ServerListPing();
	
	}
	
	
}

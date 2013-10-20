#include <rleahylib/rleahylib.hpp>
#include <base_64.hpp>
#include <json.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <utility>


using namespace MCPP;


static const String protocol_error("Protocol error");
static const String name("Ping Support");
static const Word priority=1;
static const String ping_template("{0}:{1} pinged");
static const String favicon_key("favicon");
static const String favicon_prefix("data:image/png;base64,");


class ServerListPing : public Module {


	public:
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	Get a reference to the packet router
			auto & router=Server::Get().Router;
			
			//	Install handler for the status
			//	request packet
			router(0,ProtocolState::Status)=[] (ReceiveEvent event) {
			
				auto & server=Server::Get();
			
				//	Get list of players currently on-line
				//	and number of players currently on-line
				//	for the "players" JSON object
				
				JSON::Array arr;
				Word player_count=0;
				
				for (auto & client : server.Clients) {
				
					if (client->GetState()==ProtocolState::Play) {
					
						JSON::Object obj;		
						obj.Add(
							String("name"),client->GetUsername(),
							String("id"),String()
						);
						
						arr.Values.Add(std::move(obj));
						
						++player_count;
						
					}
					
				}
				
				//	Determine maximum number of players
				Word max_players=server.MaximumPlayers;
				Double json_max_players=(max_players==0) ? std::numeric_limits<Double>::max() : Double(max_players);
				
				//	Create "players" object
				JSON::Object players;
				players.Add(
					String("max"),json_max_players,
					String("online"),Double(player_count),
					String("sample"),std::move(arr)
				);
				
				//	Add "players" object to the
				//	root object
				JSON::Object root;
				root.Add(
					String("players"),std::move(players)
				);
				
				//	Add "version" object
				JSON::Object version;
				version.Add(
					String("name"),String("MCPP"),
					String("protocol"),Double(1)
				);
				
				root.Add(
					String("version"),std::move(version)
				);
				
				//	Add "description" object
				JSON::Object description;
				description.Add(
					String("text"),server.GetMessageOfTheDay()
				);
				
				root.Add(
					String("description"),std::move(description)
				);
				
				//	Add "favicon" if applicable
				auto icon=server.Data().GetBinary(favicon_key);
				if (!icon.IsNull()) {
				
					String favi(favicon_prefix);
					favi << Base64::Encode(*icon);
					
					root.Add(
						String("favicon"),std::move(favi)
					);
				
				}
				
				//	Create and send reply
				Packets::Status::Clientbound::Response packet;
				packet.Value=std::move(root);
				
				event.From->Send(packet);
			
			};
			
			//	Install handler for the ping request
			router(1,ProtocolState::Status)=[] (ReceiveEvent event) {
			
				auto & packet=event.Data.Get<Packets::Status::Serverbound::Ping>();
				
				//	Just send the time right back
				//	to the client
				Packets::Status::Clientbound::Ping reply;
				reply.Time=packet.Time;
				
				event.From->Send(reply);
				
				Server::Get().WriteLog(
					String::Format(
						ping_template,
						event.From->IP(),
						event.From->Port()
					),
					Service::LogType::Information
				);
			
			};
		
		}


};


INSTALL_MODULE(ServerListPing)

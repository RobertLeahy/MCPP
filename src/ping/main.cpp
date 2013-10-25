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


	private:
	
	
		//	Client sends this packet to
		//	request server status information
		typedef Packets::Status::Serverbound::Request request;
		//	Client sends this packet to
		//	attempt to establish latency
		//	to the server
		typedef Packets::Status::Serverbound::Ping ping_cs;
		
		
		//	Server sends this packet with
		//	status information
		typedef Packets::Status::Clientbound::Response response;
		//	Server sends this packet so
		//	client can establish latency
		typedef Packets::Status::Clientbound::Ping ping_sc;


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
			router(
				request::PacketID,
				ProtocolState::Status
			)=[] (ReceiveEvent event) {
			
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
							"name",client->GetUsername(),
							"id",String()
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
					"max",json_max_players,
					"online",player_count,
					"sample",std::move(arr)
				);
				
				//	Add "players" object to the
				//	root object
				JSON::Object root;
				root.Add(
					"players",std::move(players)
				);
				
				//	Add "version" object
				JSON::Object version;
				version.Add(
					"name",server.GetName(),
					"protocol",ProtocolVersion
				);
				
				root.Add(
					"version",std::move(version)
				);
				
				//	Add "description" object
				JSON::Object description;
				description.Add(
					"text",server.GetMessageOfTheDay()
				);
				
				root.Add(
					"description",std::move(description)
				);
				
				//	Add "favicon" if applicable
				auto icon=server.Data().GetBinary(favicon_key);
				if (!icon.IsNull()) {
				
					String favi(favicon_prefix);
					favi << Base64::Encode(*icon);
					
					root.Add(
						"favicon",std::move(favi)
					);
				
				}
				
				//	Create and send reply
				response packet;
				packet.Value=std::move(root);
				
				event.From->Send(packet);
			
			};
			
			//	Install handler for the ping request
			router(
				ping_cs::PacketID,
				ProtocolState::Status
			)=[] (ReceiveEvent event) {
			
				auto & packet=event.Data.Get<ping_cs>();
				
				//	Just send the time right back
				//	to the client
				ping_sc reply;
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

#include <player/player.hpp>
#include <entity_id/entity_id.hpp>
#include <utility>
#include <limits>


namespace MCPP {


	void Players::on_connect (SmartPointer<Client> client) {
	
		players_lock.Write([&] () {
		
			players.emplace(
				std::move(client),
				SmartPointer<Player>()
			);
		
		});
	
	}
	
	
	void Players::on_disconnect (SmartPointer<Client> client, const String &) {
	
		players_lock.Write([&] () {	players.erase(client);	});
	
	}
	
	
	void Players::on_login (SmartPointer<Client> client) {
	
		//	Get spawn coordinates
		
		Int32 spawn_x;
		Int32 spawn_y;
		Int32 spawn_z;
		SByte dimension;
		
		spawn_lock.Read([&] () {
		
			spawn_x=this->spawn_x;
			spawn_y=this->spawn_y;
			spawn_z=this->spawn_z;
			dimension=spawn_dimension;
		
		});
		
		//	TODO: Get player's actual
		//	position/other information
		//	from backing store
		
		//	TODO: Manipulate spawn_y so that
		//	the player always spawns in air,
		//	but right above ground
		
		//	Setup this player's object
		auto player=SmartPointer<Player>::Make();
		player->ID=EntityIDGenerator::Get();
		player->Conn=client;
		player->Position.X=spawn_x;	//	TEMP
		player->Position.Y=spawn_y;	//	TEMP
		player->Position.Z=spawn_z;	//	TEMP
		player->Position.Yaw=0;	//	TEMP
		player->Position.Pitch=0;	//	TEMP
		player->Position.Stance=74;	//	TEMP
		player->Dimension=dimension;
		
		//	Add to list of players before
		//	send
		players_lock.Write([&] () {	players.find(client)->second=player;	});
		
		//	Send 0x01 -- Login Request
		
		typedef PacketTypeMap<0x01> lr;
		
		Packet packet;
		packet.SetType<lr>();
		
		packet.Retrieve<lr,0>()=player->ID;
		packet.Retrieve<lr,1>()=World::Get().Type();
		packet.Retrieve<lr,2>()=0;	//	TEMP
		packet.Retrieve<lr,3>()=dimension;
		packet.Retrieve<lr,4>()=0;	//	TEMP
		packet.Retrieve<lr,5>()=0;	//	This is always 0
		auto max_players=Server::Get().MaximumPlayers;
		if (max_players==0) max_players=std::numeric_limits<decltype(max_players)>::max();
		if (max_players>static_cast<decltype(max_players)>(std::numeric_limits<SByte>::max())) max_players=static_cast<decltype(max_players)>(std::numeric_limits<SByte>::max());
		packet.Retrieve<lr,6>()=static_cast<SByte>(max_players);
		
		client->Send(packet);
		
		//	Send 0x06 -- Spawn Position
		
		typedef PacketTypeMap<0x06> sp;
		
		packet.SetType<sp>();
		
		packet.Retrieve<sp,0>()=spawn_x;
		packet.Retrieve<sp,1>()=spawn_y;
		packet.Retrieve<sp,2>()=spawn_z;
		
		client->Send(packet);
		
		//	Send 0x0D -- Player Position and Look
		client->Send(
			player->Position.ToPacket()
		);
		
		//	Get the columns flowing
		update_position(player);
	
	}
	
	
	static const String protocol_error("Protocol error");
	
	
	void Players::position_handler (SmartPointer<Client> client, Packet packet) {
	
		auto player=get(client);
		
		if (player.IsNull()) return;
		
		//	Update player's position
		Tuple<Double,UInt64,UInt64> t;
		player->Lock.Execute([&] () {	t=player->Position.FromPacket(packet);	});
		
		//	If the player moved, update
		//	their position
		if (t.Item<0>()!=0) {
		
			Server::Get().Pool().Enqueue([=] () mutable {
		
				update_position(player);
				
			});
			
		}
	
	}


}

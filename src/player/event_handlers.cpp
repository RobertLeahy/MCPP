#include <player/player.hpp>
#include <entity_id/entity_id.hpp>
#include <server.hpp>
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
		
		//	Send Join Game
		
		Packets::Play::Clientbound::JoinGame jg;
		jg.EntityID=player->ID;
		jg.GameMode=0;	//	Temp
		jg.Dimension=dimension;
		jg.Difficulty=0;	//	Temp
		auto max_players=Server::Get().MaximumPlayers;
		typedef decltype(jg.MaxPlayers) max_players_t;
		constexpr auto max=std::numeric_limits<max_players_t>::max();
		jg.MaxPlayers=(
			(max_players==0) ||
			(max_players>max)
		) ? max : static_cast<max_players_t>(max);
		jg.LevelType=World::Get().Type();
		
		client->Send(jg);
		
		//	Send Spawn Position
		
		Packets::Play::Clientbound::SpawnPosition sp;
		sp.X=spawn_x;
		sp.Y=spawn_y;
		sp.Z=spawn_z;
		
		client->Send(sp);
		
		//	Send Player Position and Look
		client->Send(
			player->Position.ToPacket()
		);
		
		//	Get the columns flowing
		update_position(player);
	
	}
	
	
	static const String protocol_error("Protocol error");
	
	
	void Players::position_handler (PacketEvent event) {
	
		auto player=get(event.From);
		
		if (player.IsNull()) return;
		
		//	Update player's position
		auto t=player->Lock.Execute([&] () {	return player->Position.FromPacket(event.Data);	});
		
		//	If the player moved, update
		//	their position
		if (t.Item<0>()!=0) {
		
			Server::Get().Pool().Enqueue([=] () mutable {	update_position(player);	});
			
		}
	
	}


}

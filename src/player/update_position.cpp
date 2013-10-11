#include <player/player.hpp>
#include <server.hpp>
#include <multi_scope_guard.hpp>
#include <exception>
#include <utility>


namespace MCPP {


	void Players::update_position (SmartPointer<Player> & player, std::function<void ()> then) {
	
		//	Create a list of columns
		//	which must be sent to the player
		Vector<ColumnID> add;
		//	Create a list of columns which
		//	must be removed from the player
		Vector<ColumnID> remove;
	
		player->Lock.Execute([&] () {
		
			//	Determine which column
			//	the player is currently
			//	in
			auto curr=ColumnID::GetContaining(
				player->Position.X,
				player->Position.Z,
				player->Dimension
			);
		
			//	Determine bounds for
			//	caching
			Int32 x_lower=curr.X-cache_distance;
			Int32 x_upper=curr.X+cache_distance;
			Int32 z_lower=curr.Z-cache_distance;
			Int32 z_upper=curr.Z+cache_distance;
		
			//	Which columns do we have
			//	to remove?
			for (auto & id : player->Columns) if (
				(id.Dimension!=player->Dimension) ||
				(id.X<x_lower) ||
				(id.X>x_upper) ||
				(id.Z<z_lower) ||
				(id.Z>z_upper)
			) remove.Add(id);
			
			for (auto & id : remove) player->Columns.erase(id);
			
			//	Which columns do we have to add?
			for (
				Int32 x=curr.X-view_distance;
				x<curr.X+static_cast<Int32>(view_distance)+1;
				++x
			) for (
				Int32 z=curr.Z-view_distance;
				z<curr.Z+static_cast<Int32>(view_distance)+1;
				++z
			) {
			
				ColumnID id{
					x,
					z,
					player->Dimension
				};
				
				if (player->Columns.count(id)==0) {
				
					//	We must add this column
				
					add.Add(id);
					
					player->Columns.insert(id);
				
				}
			
			}
		
		});
		
		//	Perform adds and removes
		
		for (auto & id : remove) World::Get().Remove(
			player->Conn,
			id
		);
		
		//	Scope guard to enable continuation
		MultiScopeGuard sg;
		
		bool async;
		if (then) {
		
			sg=MultiScopeGuard(
				std::move(then),
				std::function<void ()>(),	//	Nothing
				[] () {	Server::Get().Panic();	}
			);
			
			async=false;
			
		} else {
		
			async=true;
		
		}
		
		for (auto & id : add) try {
		
			cm->Enqueue(
				[=] (MultiScopeGuard) {
				
					try {
				
						World::Get().Add(player->Conn,id,async);
						
					} catch (...) {
					
						Server::Get().Panic(
							std::current_exception()
						);
					
					}
					
				},
				sg
			);
		
		} catch (...) {
		
			try {
		
				Server::Get().Panic();
				
			} catch (...) {	}
			
			throw;
		
		}
	
	}


}

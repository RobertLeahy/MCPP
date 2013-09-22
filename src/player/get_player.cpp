#include <player/player.hpp>


namespace MCPP {


	SmartPointer<Player> Players::get (const SmartPointer<Client> & client) noexcept {
	
		return players_lock.Read([&] () {
		
			auto iter=players.find(client);
			
			return (iter==players.end()) ? SmartPointer<Player>() : iter->second;
		
		});
	
	}


}

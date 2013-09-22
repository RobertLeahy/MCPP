#include <player/player.hpp>


namespace MCPP {


	Player::~Player () noexcept {
	
		//	Drop all columns so that they do
		//	not spuriously remain loaded
		for (auto & column : Columns) World::Get().Remove(
			Conn,
			column,
			true
		);
	
	}


}

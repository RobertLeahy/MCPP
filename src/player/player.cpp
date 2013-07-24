#include <player/player.hpp>


namespace MCPP {


	static const Word priority=1;
	static const String name("Player Support");


	Nullable<PlayerContainer> Players;
	
	
	const String & PlayerContainer::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word PlayerContainer::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void PlayerContainer::Install () {
	
		
	
	}


}


extern "C" {


	Module * Load () {
	
		if (Players.IsNull()) Players.Construct();
		
		return &(*Players);
	
	}
	
	
	void Unload () {
	
		Players.Destroy();
	
	}


}

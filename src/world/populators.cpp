#include <world/world.hpp>


namespace MCPP {


	void World::Add (const Populator * populator, SByte dimension, Word priority) {
	
		//	Ignore null populators
		if (populator==nullptr) return;
	
		//	Get the vector of populators associated
		//	with this dimension
		auto & vec=populators[dimension];
		
		//	Find insertion point
		Word i=0;
		for (
			;
			(i<vec.Count()) &&
			(priority>vec[i].Item<1>());
			++i
		);
		
		//	Perform insertion
		vec.Emplace(
			i,
			populator,
			priority
		);
	
	}


}

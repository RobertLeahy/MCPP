#include <world/world.hpp>


namespace MCPP {


	void World::Begin () {
		
		//	Acquire synchronously
		wlock->AcquireExclusive();
	
	}
	
	
	void World::End () {
	
		//	Release
		wlock->Release();
	
	}


}

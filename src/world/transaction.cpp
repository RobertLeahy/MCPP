#include <world/world.hpp>


namespace MCPP {


	void WorldContainer::Begin () {
		
		//	Acquire synchronously
		wlock->Acquire();
	
	}
	
	
	void WorldContainer::End () {
	
		//	Release
		wlock->Release();
	
	}


}

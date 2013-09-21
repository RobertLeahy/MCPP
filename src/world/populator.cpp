#include <world/world.hpp>


namespace MCPP {


	inline void World::start_populating () {
		
		populating_lock.Write([&] () {
		
			auto id=Thread::ID();
		
			auto iter=populating.find(id);
			
			if (iter==populating.end()) populating.emplace(
				id,
				1
			);
			else ++iter->second;
		
		});
	
	}
	
	
	inline void World::stop_populating () noexcept {
		
		populating_lock.Write([&] () {
		
			auto iter=populating.find(Thread::ID());
			
			if ((--iter->second)==0) populating.erase(iter);
		
		});
	
	}
	
	
	bool World::is_populating () const noexcept {
	
		return populating_lock.Read([&] () {	return populating.count(Thread::ID())!=0;	});
	
	}


	void World::populate (ColumnContainer & column) {
	
		//	Attempt to retrieve populators
		//	for this dimension
		auto iter=populators.find(column.ID().Dimension);
		
		if (iter!=populators.end()) {
		
			//	Start populating
			start_populating();
			
			try {

				//	Loop for each populator and invoke it
				for (const auto & populator : iter->second) (*populator.Item<0>())(column.ID());
				
			} catch (...) {
			
				stop_populating();
				
				throw;
			
			}
			
			stop_populating();
		
		}
	
	}


}

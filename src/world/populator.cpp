#include <world/world.hpp>


namespace MCPP {


	inline void WorldContainer::start_populating () {
	
		populating_lock.Write([&] () {	populating.insert(Thread::ID());	});
	
	}
	
	
	inline void WorldContainer::stop_populating () noexcept {
	
		populating_lock.Write([&] () {	populating.erase(Thread::ID());	});
	
	}
	
	
	bool WorldContainer::is_populating () const noexcept {
	
		return populating_lock.Read([&] () {	return populating.count(Thread::ID())!=0;	});
	
	}


	void WorldContainer::populate (ColumnContainer & column) {
	
		//	Attempt to retrieve populators
		//	for this dimension
		auto iter=populators.find(column.ID().Dimension);
		
		if (iter!=populators.end()) {
		
			//	Start populating
			start_populating();
			
			try {
		
				//	Begin a transaction
				Begin();
				
				try {
			
					//	Loop for each populator and invoke it
					for (const auto & populator : iter->second) populator(column.ID());
					
				} catch (...) {
				
					End();
					
					throw;
				
				}
				
				End();
				
			} catch (...) {
			
				stop_populating();
				
				throw;
			
			}
			
			stop_populating();
		
		}
	
	}


}

#include <world/world.hpp>


namespace MCPP {


	SmartPointer<ColumnContainer> World::get_column (ColumnID id) {
	
		return lock.Execute([&] () {
		
			SmartPointer<ColumnContainer> retr;
		
			//	Attempt to retrieve column if it
			//	already exists
			auto iter=world.find(id);
			if (iter==world.end()) {
			
				//	Column does not exist
				
				//	Create a new column
				retr=SmartPointer<ColumnContainer>::Make(id);
				
				//	Insert it
				world.emplace(
					id,
					retr
				);
			
			} else {
			
				//	Column exists, we can just
				//	return it
				retr=iter->second;
			
			}
			
			//	We must acquire interest in the
			//	column before releasing the lock
			//	to prevent it from being spuriously
			//	unloaded
			retr->Interested();
			
			return retr;
		
		});
	
	}


}

#include <world/world.hpp>


namespace MCPP {


	ColumnContainer * World::get_column (ColumnID id, bool create) {
	
		return lock.Execute([&] () -> ColumnContainer * {
		
			ColumnContainer * retr;
		
			//	Attempt to retrieve column if it
			//	already exists
			auto iter=world.find(id);
			if (iter==world.end()) {
			
				//	Column does not exist
				
				//	If we've been told not to create
				//	the column, simply return a
				//	null pointer
				if (!create) return nullptr;
				
				//	Create a new column
				std::unique_ptr<ColumnContainer> column(new ColumnContainer(id));
				retr=column.get();
				
				//	Insert it
				world.emplace(
					id,
					std::move(column)
				);
			
			} else {
			
				//	Column exists, we can just
				//	return it
				retr=iter->second.get();
			
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

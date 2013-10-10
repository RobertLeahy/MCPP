#include <world/world.hpp>


namespace MCPP {


	void World::populate (ColumnContainer & column, const WorldHandle * handle) {
	
		//	If there's no handle, create one
		Nullable<WorldHandle> local_handle;
		if (handle==nullptr) local_handle.Construct(
			Begin(
				BlockWriteStrategy::Dirty,
				BlockAccessStrategy::Generate
			)
		);
		
		PopulateEvent event{
			column.ID(),
			(handle==nullptr) ? *local_handle : *handle
		};
	
		//	Attempt to retrieve populators
		//	for this dimension
		auto iter=populators.find(event.ID.Dimension);
		
		if (iter!=populators.end()) {
		
			event.Handle.BeginPopulate();
			
			try {
		
				for (const auto & populator : iter->second) (*populator.Item<0>())(event);
				
			} catch (...) {
			
				//	Don't leak population
				//	count
				event.Handle.EndPopulate();
				
				throw;
			
			}
			
			event.Handle.EndPopulate();
			
		}
	
	}


}

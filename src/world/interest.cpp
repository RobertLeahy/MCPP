#include <world/world.hpp>


namespace MCPP {


	void World::Interested (ColumnID id, bool prepare) {
	
		//	This automatically acquires
		//	interest
		auto column=get_column(id);
		
		if (prepare) {
		
			//	Column must be prepared -- fully
			//	populated
		
			try {
			
				if (!(
					is_populating() ||
					column->WaitUntil(ColumnState::Populated)
				)) process(*column);
			
			} catch (...) {
			
				//	Don't leak interest
				column->EndInterest();
				
				throw;
			
			}
			
		}
	
	}
	
	
	void World::EndInterest (ColumnID id) noexcept {
	
		lock.Execute([&] () {
		
			//	Releasing interest is only relevant
			//	if the column-in-question actually
			//	is loaded
			
			auto iter=world.find(id);
			if (iter!=world.end()) iter->second->EndInterest();
		
		});
	
	}


}

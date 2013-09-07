#include <world/world.hpp>


namespace MCPP {


	bool WorldContainer::SetBlock (BlockID id, Block block) {
	
		auto column=get_column(id.GetContaining());
		
		try {
		
			//	Get column into necessary
			//	state
			if (!column->WaitUntil(ColumnState::Generated)) process(*column);
			
			//	Acquire lock
			wlock.Acquire();
			
			try {
			
				//	Set block
				column->SetBlock(id,block);
				
			} catch (...) {
			
				wlock.Release();
				
				throw;
			
			}
			
			wlock.Release();
		
		} catch (...) {
		
			//	We have to make sure we end
			//	interest so the column doesn't
			//	stay loaded indefinitely
			column->EndInterest();
			
			throw;
		
		}
		
		column->EndInterest();
	
	}


}

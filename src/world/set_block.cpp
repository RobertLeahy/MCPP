#include <world/world.hpp>


namespace MCPP {


	bool World::SetBlock (BlockID id, Block block, bool force) {
	
		bool retr=true;
	
		auto column=get_column(id.GetContaining());
		
		try {
		
			//	Get column into necessary
			//	state
			if (!column->WaitUntil(ColumnState::Generated)) process(*column);
			
			//	Acquire lock
			wlock->Acquire();
			
			try {
			
				auto old=column->GetBlock(id);
			
				if (
					//	Proceed if we're forcing
					force ||
					//	If we're not forcing, check to see
					//	if we can place the block there
					can_set(
						id,
						old,
						block
					)
				) {
				
					//	Set block
					column->SetBlock(id,block);
					
					//	Only fire events for columns
					//	that are populating or populated
					auto state=column->GetState();
					if (
						(state==ColumnState::Populating) ||
						(state==ColumnState::Populated)
					) on_set(
						id,
						old,
						block
					);
				
				} else {
				
					//	Could not place
					retr=false;
				
				}
			
			} catch (...) {
			
				try {
				
					wlock->Release();
					
				} catch (...) {	}
				
				throw;
			
			}
			
			wlock->Release();
		
		} catch (...) {
		
			//	We have to make sure we end
			//	interest so the column doesn't
			//	stay loaded indefinitely
			column->EndInterest();
			
			throw;
		
		}
		
		column->EndInterest();
		
		return retr;
	
	}


}

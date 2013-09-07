#include <world/world.hpp>


namespace MCPP {


	Block WorldContainer::GetBlock (BlockID id) {
	
		auto column=get_column(id.GetContaining());
		
		Block retr;
		
		try {
		
			//	Get column state into necessary state
			if (!column->WaitUntil(
				//	Ordinarily getting a block requires
				//	a fully populated column, because
				//	otherwise the retrieved block doesn't
				//	represent a final/complete view of the
				//	column, however, if the column is currently
				//	populating ON THIS THREAD then we'll
				//	block forever if we wait for the state
				//	to change, SINCE WE ARE THE ONES WHO
				//	SHOULD BE CHANGING THE STATE
				//
				//	Accordingly we record the thread ID of
				//	the threads which are populating, and
				//	those threads can perform reads on
				//	columns which are merely generated
				is_populating()
					?	ColumnState::Generated
					:	ColumnState::Populated
			)) process(*column);
			
			retr=column->GetBlock(id);
		
		} catch (...) {
		
			column->EndInterest();
			
			throw;
		
		}
		
		column->EndInterest();
		
		return retr;
	
	}


}

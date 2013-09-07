#include <world/world.hpp>


namespace MCPP {


	ColumnState WorldContainer::load (ColumnContainer & column) {
		
		//	Attempt to retrieve data
		Word len=column.Size();
		if (!(
			//	Make sure data is actually loaded
			//	from the backing store.
			//
			//	If it's not, the column will have
			//	to be generated.
			RunningServer->Data().GetBinary(
				key(column),
				column.Get(),
				&len
			) &&
			//	Make sure that the data coming
			//	from the backing store is a sane
			//	length, if it's not consider
			//	any loaded data bogus.
			(len==column.Size())
		)) return ColumnState::Generating;
		
		//	The column was loaded, but what
		//	state was it saved in?
		//
		//	Return its state based on this
		return column.Populated ? ColumnState::Populated : ColumnState::Generated;
	
	}


}

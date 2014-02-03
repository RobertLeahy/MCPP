#include <world/world.hpp>
#include <server.hpp>
#include <cstring>


namespace MCPP {


	ColumnState World::load (ColumnContainer & column) {
	
		//	Attemt to retrieve data
		auto buffer=Server::Get().Data().GetBinary(key(column));
		//	If no data was retrieved from
		//	the backing store, the column
		//	will have to be generated
		if (buffer.IsNull()) return ColumnState::Generating;
		
		//	Decompress
		auto decompressed=Inflate(
			buffer->begin(),
			buffer->end()
		);
		
		//	If the data is an invalid length,
		//	generate the column
		if (decompressed.Count()!=ColumnContainer::Size) return ColumnState::Generating;
		
		//	Copy the decompressed data
		std::memcpy(
			column.Get(),
			decompressed.begin(),
			ColumnContainer::Size
		);
		
		//	The column was loaded, but what
		//	stat was it in?
		//
		//	Return its state based on this
		return column.Populated ? ColumnState::Populated : ColumnState::Generated;
	
	}


}

#include <world/world.hpp>


namespace MCPP {


	void WorldContainer::generate (ColumnContainer & column) {
	
		get_generator(column.ID().Dimension)(column);
	
	}


}

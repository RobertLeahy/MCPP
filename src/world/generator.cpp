#include <world/world.hpp>


namespace MCPP {


	void World::generate (ColumnContainer & column) {
	
		get_generator(column.ID().Dimension)(column);
	
	}


}

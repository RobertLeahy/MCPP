#include <world/world.hpp>


namespace MCPP {


	WorldHandle World::Begin (BlockWriteStrategy write, BlockAccessStrategy access) {
	
		return WorldHandle(
			this,
			write,
			access
		);
	
	}


}

#include <world/world.hpp>


namespace MCPP {


	WorldInfo World::GetInfo () const noexcept {
	
		Word num=lock.Execute([&] () {	return world.size();	});
		
		return WorldInfo{
			Word(maintenances),
			UInt64(maintenance_time),
			Word(loaded),
			UInt64(load_time),
			Word(unloaded),
			Word(saved),
			UInt64(save_time),
			Word(generated),
			UInt64(generate_time),
			Word(populated),
			UInt64(populate_time),
			num,
			num*(
				sizeof(ColumnContainer::Blocks)+
				sizeof(ColumnContainer::Biomes)+
				sizeof(ColumnContainer::Populated)
			)
		};
	
	}


}

#include <world/world.hpp>


namespace MCPP {


	WorldGenerator::WorldGenerator (
		Generator generator,
		BiomeGenerator biome_generator
	) noexcept : generator(std::move(generator)), biome_generator(std::move(biome_generator)) {	}
	
	
	WorldGenerator::operator bool () const noexcept {
	
		return bool(generator) && bool(biome_generator);
	
	}
	
	
	Block WorldGenerator::operator () (const BlockID & id) const {
	
		return generator(id);
	
	}
	
	
	Byte WorldGenerator::operator () (Int32 x, Int32 z, SByte dimension) const {
	
		return biome_generator(x,z,dimension);
	
	}


}

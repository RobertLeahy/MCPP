#include <world/world.hpp>
#include <stdexcept>


namespace RLeahyLib {


	bool operator == (const Tuple<String,SByte> & a, const Tuple<String,SByte> & b) {
	
		return (
			(a.Item<0>()==b.Item<0>()) &&
			(a.Item<1>()==b.Item<1>())
		);
	
	}

	
}


namespace MCPP {


	static const ASCIIChar * gen_not_found="No such world generator";


	void WorldGeneratorContainer::Add (WorldGenerator generator, SByte dimension) {
	
		default_map[dimension]=std::move(generator);
	
	}
	
	
	void WorldGeneratorContainer::Add (WorldGenerator generator, String type, SByte dimension) {
	
		map[
			Tuple<String,SByte>(
				std::move(type),
				dimension
			)
		]=std::move(generator);
	
	}
	
	
	const WorldGenerator & WorldGeneratorContainer::operator () (String type, SByte dimension) const {
	
		auto iter=map.find(
			Tuple<String,SByte>(
				std::move(type),
				dimension
			)
		);
		
		if (iter==map.end()) {
		
			auto default_iter=default_map.find(dimension);
			
			if (default_iter==default_map.end()) throw std::runtime_error(gen_not_found);
			
			return default_iter->second;
		
		}
		
		return iter->second;
	
	}


}

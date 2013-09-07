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


	void WorldContainer::Add (const WorldGenerator * generator, SByte dimension) {
	
		if (generator==nullptr) return;
	
		default_generators[dimension]=std::move(generator);
	
	}
	
	
	void WorldContainer::Add (const WorldGenerator * generator, String type, SByte dimension) {
	
		if (generator==nullptr) return;
	
		generators[
			Tuple<String,SByte>(
				std::move(type),
				dimension
			)
		]=std::move(generator);
	
	}
	
	
	const WorldGenerator & WorldContainer::get_generator (SByte dimension) const {
	
		auto iter=generators.find(
			Tuple<String,SByte>(
				type,
				dimension
			)
		);
		
		if (iter==generators.end()) {
		
			auto default_iter=default_generators.find(dimension);
			
			if (default_iter==default_generators.end()) throw std::runtime_error(gen_not_found);
			
			return *(default_iter->second);
		
		}
		
		return *(iter->second);
	
	}


}

#include <world/world.hpp>
#include <new>
#include <type_traits>


namespace MCPP {


	bool World::can_set (const BlockSetEvent & event) noexcept {
	
		try {
	
			return (
				CanSet(event) &&
				CanReplace[event.From.GetType()](event) &&
				CanPlace[event.To.GetType()](event)
			);
			
		} catch (...) {	}
		
		return false;
	
	}
	
	
	void World::on_set (const BlockSetEvent & event) {
	
		OnSet(event);
		OnReplace[event.From.GetType()](event);
		OnPlace[event.To.GetType()](event);
	
	}
	
	
	template <Word n, typename T>
	static inline void cleanup_array (T (& arr) [n]) noexcept {
	
		for (auto & i : arr) i=T();
	
	}
	
	
	void World::cleanup_events () noexcept {
	
		cleanup_array(OnReplace);
		cleanup_array(OnPlace);
		cleanup_array(CanReplace);
		cleanup_array(CanPlace);
	
	}


}

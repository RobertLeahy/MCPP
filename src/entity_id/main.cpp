#include <entity_id/entity_id.hpp>
#include <singleton.hpp>


using namespace MCPP;


namespace MCPP {


	static const String name("Entity ID Generator");
	static const Word priority=1;


	EntityIDGenerator::EntityIDGenerator () noexcept {
	
		id=0;
	
	}
	
	
	const String & EntityIDGenerator::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word EntityIDGenerator::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void EntityIDGenerator::Install () {	}
	
	
	static Singleton<EntityIDGenerator> singleton;
	
	
	Int32 EntityIDGenerator::Get () noexcept {
	
		union {
			Int32 s;
			UInt32 u;
		};
		
		u=singleton.Get().id++;
		
		return s;
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(singleton.Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

#include <save/save.hpp>
#include <world/world.hpp>
#include <server.hpp>
#include <singleton.hpp>


using namespace MCPP;


namespace MCPP {


	static const String name("World Support");
	static const Word priority=1;
	const String World::verbose("world");
	static const String default_world_type("DEFAULT");
	static const Word default_maintenance_interval=5*60*1000;	//	5 minutes
	static const String seed_key("seed");
	static const String maintenance_interval_key("maintenance_interval");
	static const String type_key("world_type");
	static const String log_type("Set world type to \"{0}\"");


	World::World () noexcept {
		
		//	Initialize stat counters
		maintenances=0;
		maintenance_time=0;
		unloaded=0;
		loaded=0;
		load_time=0;
		saved=0;
		save_time=0;
		generated=0;
		generate_time=0;
		populated=0;
		populate_time=0;
	
	}
	
	
	const String & World::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word World::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void World::Install () {
	
		auto & server=Server::Get();
		
		//	Get settings
		
		//	Seed
		auto seed=server.Data().GetSetting(
			seed_key
		);
		set_seed(seed);
		if (seed.IsNull()) server.Data().SetSetting(
			seed_key,
			String(this->seed)
		);
		
		//	World type
		auto type=server.Data().GetSetting(
			type_key
		);
		this->type=type.IsNull() ? default_world_type : *type;
		server.WriteLog(
			String::Format(
				log_type,
				this->type
			),
			Service::LogType::Information
		);
		
		//	Install shutdown handler to cleanup
		//	any module code
		server.OnShutdown.Add([this] () mutable {	cleanup_events();	});
		
		//	Tie into the save loop
		SaveManager::Get().Add([this] () mutable {	maintenance();	});
	
	}
	
	
	const String & World::Type () const noexcept {
	
		return type;
	
	}
	
	
	static Singleton<World> singleton;
	
	
	World & World::Get () noexcept {
	
		return singleton.Get();
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(World::Get());	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

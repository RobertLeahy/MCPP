#include <world/world.hpp>
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
		
		//	Initialize events
		init_events();
	
	}
	
	
	World::~World () noexcept {
	
		destroy_events();
	
	}
	
	
	const String & World::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word World::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void World::Install () {
		
		//	Get settings
		
		//	Seed
		auto seed=Server::Get().Data().GetSetting(
			seed_key
		);
		set_seed(seed);
		if (seed.IsNull()) Server::Get().Data().SetSetting(
			seed_key,
			String(this->seed)
		);
		
		//	Time per maintenance cycle
		auto maintenance_interval=Server::Get().Data().GetSetting(
			maintenance_interval_key
		);
		if (
			maintenance_interval.IsNull() ||
			!maintenance_interval->ToInteger(&(this->maintenance_interval))
		) this->maintenance_interval=default_maintenance_interval;
		
		//	World type
		auto type=Server::Get().Data().GetSetting(
			type_key
		);
		this->type=type.IsNull() ? default_world_type : *type;
		Server::Get().WriteLog(
			String::Format(
				log_type,
				this->type
			),
			Service::LogType::Information
		);
		
		//	Install shutdown handler to cleanup
		//	any module code
		Server::Get().OnShutdown.Add([this] () {	cleanup_events();	});
		
		//	Begin maintenance
		Server::Get().Pool().Enqueue(
			this->maintenance_interval,
			[this] () {	maintenance();	}
		);
	
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

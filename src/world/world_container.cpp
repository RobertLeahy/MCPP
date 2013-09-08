#include <world/world.hpp>


namespace MCPP {


	static const String name("World Support");
	static const Word priority=1;
	static const String mods_dir("world_mods");
	static const String log_prepend("World Support: ");
	const String WorldContainer::verbose("world");
	static const String default_world_type("DEFAULT");
	static const Word default_maintenance_interval=5*60*1000;	//	5 minutes
	static const String seed_key("seed");
	static const String maintenance_interval_key("maintenance_interval");
	static const String type_key("world_type");
	static const String log_type("Set world type to \"{0}\"");
	
	
	
	static Nullable<ModuleLoader> mods;


	WorldContainer::WorldContainer () {
	
		//	Create a module loader
		mods.Construct(
			Path::Combine(
				Path::GetPath(
					File::GetCurrentExecutableFileName()
				),
				mods_dir
			),
			[] (const String & message, Service::LogType type) {
			
				String log(log_prepend);
				log << message;
				
				RunningServer->WriteLog(
					log,
					type
				);
			
			}
		);
		
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
	
	
	WorldContainer::~WorldContainer () noexcept {
	
		destroy_events();
	
		mods->Unload();
	
	}
	
	
	const String & WorldContainer::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word WorldContainer::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void WorldContainer::Install () {
	
		//	Create world lock now
		//	that we can grab a reference
		//	to the server's thread pool
		wlock.Construct(
			RunningServer->Pool(),
			[] () {	RunningServer->Panic();	}
		);
	
		//	Get mods
		mods->Load();
		
		//	Get settings
		
		//	Seed
		auto seed=RunningServer->Data().GetSetting(
			seed_key
		);
		set_seed(seed);
		if (seed.IsNull()) RunningServer->Data().SetSetting(
			seed_key,
			String(this->seed)
		);
		
		//	Time per maintenance cycle
		auto maintenance_interval=RunningServer->Data().GetSetting(
			maintenance_interval_key
		);
		if (
			maintenance_interval.IsNull() ||
			!maintenance_interval->ToInteger(&(this->maintenance_interval))
		) this->maintenance_interval=default_maintenance_interval;
		
		//	World type
		auto type=RunningServer->Data().GetSetting(
			type_key
		);
		this->type=type.IsNull() ? default_world_type : *type;
		RunningServer->WriteLog(
			String::Format(
				log_type,
				this->type
			),
			Service::LogType::Information
		);
		
		//	Install mods
		mods->Install();
		
		//	Begin maintenance
		RunningServer->Pool().Enqueue(
			this->maintenance_interval,
			[this] () {	maintenance();	}
		);
	
	}
	
	
	Nullable<WorldContainer> World;


}


extern "C" {


	Module * Load () {
	
		try {
		
			if (World.IsNull()) {
			
				mods.Destroy();
				
				World.Construct();
			
			}
			
			return &(*World);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		World.Destroy();
		
		if (!mods.IsNull()) mods->Unload();
	
	}
	
	
	void Cleanup () {
	
		mods.Destroy();
	
	}


}

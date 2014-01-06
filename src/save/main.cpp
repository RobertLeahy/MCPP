#include <save/save.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <thread_pool.hpp>
#include <exception>
#include <utility>


using namespace MCPP;


namespace MCPP {


	static const Word priority=1;
	static const String name("Save Manager");
	static const String debug_key("save");
	static const String save_complete("Save complete - took {0}ns");
	static const String save_paused("Save loop paused, skipping");
	static const String setting_key("save_frequency");
	static const Word frequency_default=5*60*1000;	//	Every 5 minutes
	
	
	bool SaveManager::is_verbose () {
	
		return Server::Get().IsVerbose(debug_key);
	
	}
	
	
	void SaveManager::save () {
	
		auto & server=Server::Get();
	
		try {
		
			Timer timer(Timer::CreateAndStart());
			
			for (auto & callback : callbacks) callback();
			
			auto elapsed=timer.ElapsedNanoseconds();
			
			this->elapsed+=elapsed;
			++count;
			
			if (is_verbose()) server.WriteLog(
				String::Format(
					save_complete,
					elapsed,
					frequency
				),
				Service::LogType::Debug
			);
		
		} catch (...) {
		
			try {
			
				server.Panic(std::current_exception());
				
			} catch (...) {	}
			
			throw;
		
		}
	
	}
	
	
	void SaveManager::save_loop () {
	
		auto & server=Server::Get();
	
		if (
			lock.Execute([&] () mutable {
			
				if (paused) return true;
		
				save();
				
				return false;
				
			}) &&
			is_verbose()
		) server.WriteLog(
			save_paused,
			Service::LogType::Debug
		);
		
		try {
		
			server.Pool().Enqueue(
				frequency,
				[this] () mutable {	save_loop();	}
			);
		
		} catch (...) {
		
			try {
			
				server.Panic(std::current_exception());
				
			} catch (...) {	}
			
			throw;
		
		}
	
	}


	static Singleton<SaveManager> singleton;
	
	
	SaveManager & SaveManager::Get () noexcept {
	
		return singleton.Get();
	
	}
	
	
	SaveManager::SaveManager () noexcept {
	
		paused=false;
	
		count=0;
		elapsed=0;
	
	}
	
	
	Word SaveManager::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & SaveManager::Name () const noexcept {
	
		return name;
	
	}
	
	
	void SaveManager::Install () {
	
		auto & server=Server::Get();
		
		//	On shutdown we must save everything
		//	one last time
		server.OnShutdown.Add([this] () mutable {
		
			save();
			
			//	Purge all callbacks held which
			//	may have originated from other
			//	modules
			callbacks.Clear();
		
		});
		
		//	Once the install phase is done, and
		//	all modules that wish to save have
		//	added their handlers, start the
		//	save loop
		server.OnInstall.Add([this] (bool) mutable {
		
			Server::Get().Pool().Enqueue(
				frequency,
				[this] () mutable {	save_loop();	}
			);
		
		});
		
		//	Get the frequency with which saves
		//	should be performed from the backing store
		frequency=server.Data().GetSetting(
			setting_key,
			frequency_default
		);
	
	}
	
	
	void SaveManager::Add (std::function<void ()> callback) {
	
		if (callback) callbacks.Add(std::move(callback));
	
	}
	
	
	void SaveManager::operator () () {
	
		lock.Execute([&] () mutable {	save();	});
	
	}
	
	
	void SaveManager::Pause () noexcept {
	
		lock.Execute([&] () mutable {	paused=true;	});
	
	}
	
	
	void SaveManager::Resume () noexcept {
	
		lock.Execute([&] () mutable {	paused=false;	});
	
	}
	
	
	SaveManagerInfo SaveManager::GetInfo () const noexcept {
	
		return SaveManagerInfo{
			frequency,
			paused,
			count,
			elapsed
		};
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(SaveManager::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

#include <player/player.hpp>
#include <hardware_concurrency.hpp>
#include <utility>


using namespace MCPP;


namespace MCPP {


	static const Word priority=1;
	static const String name("Player Support");
	
	
	Players::Players () noexcept
		:	view_distance(10),
			cache_distance(11),
			spawn_x(0),
			spawn_y(256),
			spawn_z(0),
			spawn_dimension(0),
			interested_locked(false)
	{	}
	
	
	const String & Players::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Players::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	static const String column_concurrency_key("column_concurrency");
	static const String column_concurrency_log("No user-supplied value for \"{0}\" - {1} concurrent column operations will be allowed");
	
	
	void Players::Install () {
	
		//	Determine the number of concurrent
		//	column operations that should be
		//	allowed
		auto concurrency_str=Server::Get().Data().GetSetting(column_concurrency_key);
		Word concurrency;
		if (
			concurrency_str.IsNull() ||
			!concurrency_str->ToInteger(&concurrency)
		) {
		
			//	No supplied value, attempt
			//	to deduce an appropriate value
			
			//	Determine the number of
			//	concurrent threads of
			//	execution allowed by the
			//	hardware we're running on
			auto hw=HardwareConcurrency();
			
			concurrency=(
				(
					//	Library couldn't determine
					//	hardware concurrency
					(hw==0) ||
					//	The server thread pool has
					//	not been provisioned with
					//	this many workers (we don't
					//	want to saturate the thread
					//	pool)
					(hw>Server::Get().Pool().Count())
				)
					?	Server::Get().Pool().Count()
					:	hw
			);
			
			//	Don't saturate
			concurrency/=2;
			
			//	Log
			Server::Get().WriteLog(
				String::Format(
					column_concurrency_log,
					column_concurrency_key,
					concurrency
				),
				Service::LogType::Information
			);
		
		}
	
		//	Create the concurrency manager
		//
		//	This is done here rather than
		//	during construction since the server's
		//	thread pool is not available during
		//	construction
		cm.Construct(
			Server::Get().Pool(),
			concurrency,
			[] () {	Server::Get().Panic();	}
		);
	
		//	Install event handlers
		
		Server::Get().OnConnect.Add([this] (SmartPointer<Client> client) {	on_connect(std::move(client));	});
		
		Server::Get().OnDisconnect.Add([this] (SmartPointer<Client> client, const String & reason) {	on_disconnect(std::move(client),reason);	});
		
		Server::Get().OnLogin.Add([this] (SmartPointer<Client> client) {	on_login(std::move(client));	});
		
		std::function<void (SmartPointer<Client>, Packet)> handler=[this] (SmartPointer<Client> client, Packet packet) {
		
			position_handler(
				std::move(client),
				std::move(packet)
			);
		
		};
		
		//	Handle packets 0x0A, 0x0B, 0x0C, and 0x0D
		Server::Get().Router[0x0A]=handler;
		Server::Get().Router[0x0B]=handler;
		Server::Get().Router[0x0C]=handler;
		Server::Get().Router[0x0D]=handler;
		
		//	Set spawn when the server finishes
		//	installing all mods (so we know
		//	world container is available and
		//	setup)
		Server::Get().OnInstall.Add([this] (bool) mutable {
		
			//	We can ignore boolean parameter,
			//	the time to fire before install
			//	was before this function was even
			//	invoked
			
			Mutex lock;
			CondVar wait;
			bool done=false;
			
			set_spawn([&] () {
			
				lock.Acquire();
				done=true;
				wait.Wake();
				lock.Release();
			
			});
			
			lock.Acquire();
			while (!done) wait.Sleep(lock);
			lock.Release();
		
		});
	
	}
	
	
	static Nullable<Players> singleton;
	static Mutex singleton_lock;
	
	
	Players & Players::Get () noexcept {
	
		singleton_lock.Acquire();
		if (singleton.IsNull()) singleton.Construct();
		singleton_lock.Release();
		return *singleton;
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(Players::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

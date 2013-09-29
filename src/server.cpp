#include <server.hpp>
#include <singleton.hpp>
#include <cstdlib>
#include <exception>


namespace MCPP {
	
	
	//	Messages
	static const String startup_error="Startup: Unhandled exception";
	static const String exception_what="{0}: {1}";
	static const String endpoint_template="{0}:{1}";
	static const String list_separator=", ";
	static const String binding_to="Startup: Attempting to bind to {0}";
	static const String couldnt_parse_bind="Startup: Could not parse bind \"{0}\"";
	static const String connected="{0}:{1} connected, there {3} now {2} client{4} connected";
	static const String disconnected="{0}:{1} disconnected, there {3} now {2} client{4} connected";
	static const String disconnected_with_reason="{{0}}:{{1}} disconnected (with reason: \"{0}\"), there {{3}} now {{2}} client{{4}} connected";
	static const String error_processing_recv="Error processing received data";
	static const String buffer_too_long="Buffer too long";
	
	
	//	Constants
	static const String binds_setting="binds";
	static const String num_threads_setting="num_threads";
	static const UInt16 default_port=25565;	//	Default MC port
	static const Word default_num_threads=10;
	static const String main_thread_desc="Listening Thread";
	static const Word default_max_bytes=0;	//	Unlimited
	static const String max_bytes_setting="max_bytes";
	static const Word default_max_players=0;
	static const String max_players_setting="max_players";


	Server::Server () :
		running(false),
		data(nullptr),
		Router(true)
	{
	
		debug=false;
		log_all_packets=false;
		verbose_all=false;
		num_shutdowns=0;
		
		OnReceive=[=] (SmartPointer<Connection> conn, Vector<Byte> & buffer) {
		
			try {
			
				auto client=Clients[*conn];
				
				//	Loop while there's a packet to
				//	extract
				while (client->Receive(&buffer)) Router(client,client->CompleteReceive());
				
				//	Check buffer
				//	if it's gotten bigger than
				//	allowed, kill the client
				if (
					//	0 = unlimited
					(MaximumBytes!=0) &&
					(
						//	Check both buffers
						(buffer.Count()>MaximumBytes) ||
						(client->Count()>MaximumBytes)
					)
				) client->Disconnect(buffer_too_long);
			
			} catch (const std::exception & e) {
			
				conn->Disconnect(error_processing_recv);
				
				throw;
			
			} catch (...) {
			
				conn->Disconnect(error_processing_recv);
				
				throw;
			
			}
		
		};
	
	}
	
	
	Server::~Server () noexcept {
	
		stop_impl();
		
		//	Make sure we don't destroy this
		//	object while a thread is calling
		//	Stop on it.
		shutdown_lock.Acquire();
		while (num_shutdowns!=0) shutdown_wait.Sleep(shutdown_lock);
		shutdown_lock.Release();
	
	}
	
	
	inline void Server::cleanup_events () noexcept {
	
		Router.Clear();
		OnAccept.Clear();
		OnConnect.Clear();
		OnDisconnect.Clear();
		OnLog.Clear();
		OnLogin.Clear();
		OnInstall.Clear();
		OnShutdown.Clear();
		OnReceive=ReceiveCallback();
	
	}
	
	
	void Server::Stop () noexcept {
	
		shutdown_lock.Acquire();
		++num_shutdowns;
		shutdown_lock.Release();
	
		try {
	
			Thread t([this] () mutable {
			
				try {
			
					stop_impl();
					
					//	None of this can throw
					shutdown_lock.Acquire();
					if ((--num_shutdowns)==0) shutdown_wait.WakeAll();
					shutdown_lock.Release();
					
				} catch (...) {
				
					shutdown_lock.Acquire();
					if ((--num_shutdowns)==0) shutdown_wait.WakeAll();
					shutdown_lock.Release();
				
					Panic(std::current_exception());
					
					throw;
				
				}
			
			});
			
		} catch (...) {
		
			shutdown_lock.Acquire();
			if ((--num_shutdowns)==0) shutdown_wait.WakeAll();
			shutdown_lock.Release();
		
			Panic(std::current_exception());
		
		}
	
	}
	
	
	static const String no_restart("Restart requested without a restart handler");
	
	
	void Server::Restart () noexcept {
	
		try {
		
			Thread t([this] () mutable {
			
				try {
				
					if (OnRestart) OnRestart();
					else Panic(no_restart);
				
				} catch (...) {
				
					Panic(std::current_exception());
					
					throw;
				
				}
			
			});
		
		} catch (...) {
		
			Panic(std::current_exception());
		
		}
	
	}
	
	
	void Server::WriteLog (const String & message, Service::LogType type) noexcept {
	
		//	Don't throw errors, eat them
		try {
		
			//	Fire event
			try {
			
				OnLog(message,type);
			
			} catch (...) {	}
			
			if (data!=nullptr) data->WriteLog(message,type);
			
		} catch (...) {	}
	
	}
	
	
	void Server::WriteChatLog (const String & from, const Vector<String> & to, const String & message, const Nullable<String> & notes) noexcept {
	
		//	Don't throw errors, eat them
		try {
		
			//	Fire event
			try {
			
				OnChatLog(
					from,
					to,
					message,
					notes
				);
			
			} catch (...) {	}
			
			if (data!=nullptr) data->WriteChatLog(
				from,
				to,
				message,
				notes
			);
		
		} catch (...) {	}
	
	}
	
	
	void Server::Start () {
	
		//	Hold the state lock while
		//	changing the server's state
		state_lock.Acquire();
		
		try {
		
			//	Server is already running, can't
			//	start a running server
			if (running) {
			
				state_lock.Release();
				
				return;
			
			}
			
			//	START
			try {
			
				try {

					server_startup();
					
				} catch (...) {
				
					if (data!=nullptr) {
					
						delete data;
						data=nullptr;
						
					}
					
					interpreter=nullptr;
					provider=nullptr;
					
					connections.Destroy();
					pool.Destroy();
					
					cleanup_events();
					
					mods.Destroy();
					
					throw;
				
				}
				
			} catch (const std::exception & e) {
			
				try {
			
					WriteLog(
						String::Format(
							exception_what,
							startup_error,
							e.what()
						),
						Service::LogType::Error
					);
						
				} catch (...) {	}
			
				throw;
			
			} catch (...) {
			
				WriteLog(
					startup_error,
					Service::LogType::Error
				);
			
				throw;
			
			}
			
		} catch (...) {
		
			state_lock.Release();
			
			throw;
		
		}
		
		//	Startup a success
		running=true;
		
		state_lock.Release();
	
	}
	
	
	void Server::stop_impl () {
	
		//	Hold the state lock before
		//	attempting to change the
		//	server's state
		state_lock.Acquire();
		
		//	Server is already stopped, can't
		//	stop a stopped server
		if (!running) {
		
			state_lock.Release();
			
			return;
		
		}
		
		//	STOP
		
		//	Disconnect all connected clients
		//	and stop the flow of new connections
		connections.Destroy();
		
		//	Now we wait on all pending
		//	tasks and then kill the
		//	thread pool
		pool.Destroy();
		
		//	Clear all events et cetera
		//	that might have module resources
		//	loaded into them
		try {
		
			OnShutdown();
		
		} catch (...) {
		
			Panic(std::current_exception());
			
			throw;
		
		}
		cleanup_events();
		
		//	Now we unload all modules
		if (!mods.IsNull()) mods->Unload();
		
		//	Kill all modules
		mods.Destroy();
		
		//	Clean up data provider
		delete data;
		data=nullptr;
		
		//	Clean up command interpreter
		interpreter=nullptr;
		//	Clean up chat provider
		provider=nullptr;
		
		//	Shutdown complete
		running=false;
		
		state_lock.Release();
	
	}
	
	
	inline void Server::panic_impl (std::exception_ptr except) noexcept {
	
		if (OnPanic) try {
		
			OnPanic(except);
			
			return;
		
		} catch (...) {	}
		
		std::abort();
	
	}
	
	
	static const String panic_message="Panic!";
	
	
	void Server::Panic () noexcept {
	
		try {
		
			WriteLog(
				panic_message,
				Service::LogType::Critical
			);
		
		} catch (...) {	}
		
		panic_impl();
	
	}
	
	
	void Server::Panic (const String & reason) noexcept {
	
		try {
		
			WriteLog(
				reason,
				Service::LogType::Critical
			);
		
		} catch (...) {	}
		
		panic_impl();
	
	}
	
	
	void Server::Panic (std::exception_ptr except) noexcept {
	
		try {
		
			try {
			
				std::rethrow_exception(except);
				
			} catch (const std::exception & e) {
			
				WriteLog(
					e.what(),
					Service::LogType::Critical
				);
			
			}
			
		} catch (...) {	}
		
		panic_impl(except);
	
	}
	
	
	void Server::SetDebug (bool debug) noexcept {
	
		this->debug=debug;
	
	}
	
	
	void Server::SetDebugAllPackets (bool log_all_packets) noexcept {
	
		this->log_all_packets=log_all_packets;
	
	}
	
	
	void Server::SetDebugPacket (Byte packet_id, ProtocolDirection direction) {
	
		logged_packets_lock.Write([&] () {
		
			auto iter=logged_packets.find(packet_id);
			
			if (iter==logged_packets.end()) logged_packets.emplace(packet_id,direction);
			else iter->second=direction;
		
		});
	
	}
	
	
	void Server::UnsetDebugPacket (Byte packet_id) noexcept {
	
		logged_packets_lock.Write([&] () {	logged_packets.erase(packet_id);	});
	
	}
	
	
	void Server::SetVerboseAll (bool verbose_all) noexcept {
	
		this->verbose_all=verbose_all;
	
	}
	
	
	void Server::SetVerbose (String verbose) {
	
		verbose_lock.Write([&] () {	this->verbose.insert(std::move(verbose));	});
	
	}
	
	
	void Server::UnsetVerbose (const String & verbose) {
	
		verbose_lock.Write([&] () {	this->verbose.erase(verbose);	});
	
	}
	
	
	bool Server::LogPacket (Byte type, ProtocolDirection direction) const noexcept {
	
		//	If we're not debugging at all, short-circuit out
		if (!debug) return false;
		
		//	If we're debugging all packets, return
		//	true immediately
		if (log_all_packets) return true;
		
		//	Lock and determine if we're logging
		//	this particular packet type in this
		//	particular direction
		return logged_packets_lock.Read([&] () {
		
			auto iter=logged_packets.find(type);
			
			return (
				(iter!=logged_packets.end()) &&
				(
					(direction==ProtocolDirection::Both) ||
					(direction==iter->second)
				)
			);
		
		});
	
	}
	
	
	bool Server::IsVerbose (const String & key) const {
	
		//	If we're not debugging at all, short-circuit out
		if (!debug) return false;
		
		//	If everything is verbose, return true at once
		if (verbose_all) return true;
		
		//	Lock and see if this particular key is
		//	verbose
		return verbose_lock.Read([&] () {	return verbose.count(key)!=0;	});
	
	}
	
	
	static const String server_id_setting("server_id");
	static const String motd_setting("motd");
	static const String default_server_id("Minecraft++");


	DataProvider & Server::Data () {

		if (data==nullptr) throw std::out_of_range(NullPointerError);

		return *data;

	}


	ThreadPool & Server::Pool () {

		if (pool.IsNull()) throw std::out_of_range(NullPointerError);
		
		return *pool;

	}
	
	
	ModuleLoader & Server::Loader () {
	
		if (mods.IsNull()) throw std::out_of_range(NullPointerError);
		
		return *mods;
	
	}

		
	String Server::GetMessageOfTheDay () {

		if (data==nullptr) return String();
		
		Nullable<String> motd=data->GetSetting(motd_setting);
		
		if (motd.IsNull()) return String();
		
		return *motd;

	}
	
	
	static const String build_date(
		__DATE__
		" "
		__TIME__
	);
	
	
	const String & Server::BuildDate () const noexcept {
	
		return build_date;

	}
	
	
	#define stringify_impl(x) #x
	#define stringify(x) stringify_impl(x)
	
	
	static const String compiled_with(
		#ifdef __GNUC__
		"GNU C++ Compiler (g++) "
		stringify(__GNUC__)
		"."
		stringify(__GNUC_MINOR__)
		"."
		stringify(__GNUC_PATCHLEVEL__)
		#else
		"UNKNOWN"
		#endif
	);


	const String & Server::CompiledWith () const noexcept {
	
		return compiled_with;

	}
	
	
	static Singleton<Server> singleton;


	Server & Server::Get () noexcept {
	
		return singleton.Get();

	}
	
	
	void Server::Destroy () noexcept {
	
		singleton.Destroy();
	
	}
	
	
	static const String mods_dir("mods");
	static const String startup_prepend("Startup: ");


	inline void Server::server_startup () {

		//	Get a data provider
		data=DataProvider::GetDataProvider();

		//	Log callback for components
		MCPP::LogType log(
			[=] (const String & message, Service::LogType type) -> void {	WriteLog(message,type);	}
		);
		
		//	Load mods
		mods.Construct(
			Path::Combine(
				Path::GetPath(
					File::GetCurrentExecutableFileName()
				),
				mods_dir
			),
			[this] (const String & message, Service::LogType type) {
			
				String log(startup_prepend);
				log << message;
				
				WriteLog(log,type);
			
			}
		);
		mods->Load();

		//	Clear clients in case an existing
		//	instance is being recycled
		Clients.Clear();
		//	Clear routes for same reason
		Router.Clear();
		
		//	Grab settings
		
		//	Maximum number of bytes to buffer
		Nullable<String> max_bytes_str(data->GetSetting(max_bytes_setting));
		if (
			max_bytes_str.IsNull() ||
			!max_bytes_str->ToInteger(&MaximumBytes)
		) MaximumBytes=default_max_bytes;
		
		//	Maximum number of players
		Nullable<String> max_players_str(data->GetSetting(max_players_setting));
		if (
			max_players_str.IsNull() ||
			!max_players_str->ToInteger(&MaximumPlayers)
		) MaximumPlayers=default_max_players;

		//	Initialize a thread pool
		
		//	Attempt to grab number of threads
		//	from the data source
		Nullable<String> num_threads_str(data->GetSetting(num_threads_setting));
		
		Word num_threads;
		if (
			num_threads_str.IsNull() ||
			!num_threads_str->ToInteger(&num_threads) ||
			(num_threads==0)
		) num_threads=default_num_threads;
		
		//	Panic callback
		PanicType panic([=] () -> void {	Panic();	});
		
		//	Fire up the thread pool
		pool.Construct(num_threads,panic);
		
		//	Install mods
		OnInstall(true);
		mods->Install();
		OnInstall(false);
		
		//	Get binds
		Nullable<String> binds_str(data->GetSetting(binds_setting));
		
		Vector<Endpoint> binds;
		
		//	Parse binds
		if (!binds_str.IsNull()) {
		
			//	Binds are separated by a semi-colon
			Vector<String> split(Regex(";").Split(*binds_str));
			
			for (String & str : split) {
			
				bool set_port=false;
				bool set_ip=false;
			
				//	Attempt to extract port number
				RegexMatch port_match=Regex(
					"(?<!\\:)\\:\\s*(\\d+)\\s*$",
					RegexOptions().SetRightToLeft()
				).Match(str);
				
				UInt16 port_no;
				
				if (
					port_match.Success() &&
					port_match[1].Value().ToInteger(&port_no)
				) set_port=true;
				else port_no=default_port;
				
				IPAddress ip(IPAddress::Any());
				
				//	Attempt to extract IP address
				RegexMatch ip_match=Regex(
					"^(?:[^\\:]|\\:(?=\\:))+"
				).Match(str);
				
				if (ip_match.Success()) {
				
					try {
					
						IPAddress extracted_ip(ip_match.Value().Trim());
						
						ip=extracted_ip;
						
						set_ip=true;
					
					} catch (...) {	}
				
				}
				
				//	Did we actually set either of
				//	the values?
				if (set_port || set_ip) {
				
					//	Add bind
					binds.EmplaceBack(ip,port_no);
				
				} else {
				
					//	Log that we couldn't parse this bind
					WriteLog(
						String::Format(
							couldnt_parse_bind,
							str
						),
						Service::LogType::Warning
					);
					
				}
			
			}
		
		//	No binds
		} else {
		
			no_binds:
			
			//	IPv4 all, default port
			binds.EmplaceBack(IPAddress::Any(),default_port);
			//	IPv6 all, default port
			binds.EmplaceBack(IPAddress::Any(true),default_port);
		
		}
		
		//	If no binds
		if (binds.Count()==0) goto no_binds;
		
		//	Log binds
		String binds_desc;
		for (Word i=0;i<binds.Count();++i) {
		
			if (i!=0) binds_desc << list_separator;
			
			binds_desc << String::Format(
				endpoint_template,
				binds[i].IP(),
				binds[i].Port()
			);
		
		}
		
		WriteLog(
			String::Format(
				binding_to,
				binds_desc
			),
			Service::LogType::Information
		);
		
		//	Try and fire up the connection
		//	manager
		connections.Construct(
			binds,
			OnAccept,
			[=] (SmartPointer<Connection> conn) {
			
				//	Save IP and port number
				IPAddress ip=conn->IP();
				UInt16 port=conn->Port();
			
				try {
					
					//	Create client object
					auto client=SmartPointer<Client>::Make(
						std::move(
							conn
						)
					);
					
					//	Add to the client list
					Clients.Add(client);
					
					//	Fire event handler
					OnConnect(std::move(client));
					
				} catch (...) {
				
					//	Panic on error
					Panic();
					
					throw;
				
				}
				
				try {
					
					//	Log
					auto clients=Clients.Count();
					WriteLog(
						String::Format(
							connected,
							ip,
							port,
							clients,
							(clients==1) ? "is" : "are",
							(clients==1) ? "" : "s"
						),
						Service::LogType::Information
					);
					
				} catch (...) {	}
			
			},
			[=] (SmartPointer<Connection> conn, const String & reason) {
			
				try {
				
					//	Look up the client
					auto client=Clients[*conn];
					
					//	Remove the client from the
					//	list
					Clients.Remove(*conn);
					
					//	Fire event handler
					OnDisconnect(client,reason);
					
				} catch (...) {
				
					//	Panic on error
					Panic();
					
					throw;
				
				}
				
				try {
					
					//	Log
					String disconnect_template;
					
					//	If there's no reason, choose
					//	the template with no reason
					if (reason.Size()==0) {
					
						disconnect_template=disconnected;
					
					} else {
					
						//	Fill the reason into the template
						disconnect_template=String::Format(
							disconnected_with_reason,
							reason
						);
					
					}
					
					auto clients=Clients.Count();
					WriteLog(
						String::Format(
							disconnect_template,
							conn->IP(),
							conn->Port(),
							clients,
							(clients==1) ? "is" : "are",
							(clients==1) ? "" : "s"
						),
						Service::LogType::Information
					);
					
				} catch (...) {	}
			
			},
			OnReceive,
			log,
			panic,
			*pool
		);
	
	}
	
	
	void Server::SetCommandInterpreter (CommandInterpreter * interpreter) noexcept {
	
		if (interpreter!=nullptr) this->interpreter=interpreter;
	
	}
	
	
	CommandInterpreter & Server::GetCommandInterpreter () const {
	
		if (interpreter==nullptr) throw std::out_of_range(NullPointerError);
		
		return *interpreter;
	
	}
	
	
	void Server::SetChatProvider (ChatProvider * provider) noexcept {
	
		if (provider!=nullptr) this->provider=provider;
	
	}
	
	
	ChatProvider & Server::GetChatProvider () const {
	
		if (provider==nullptr) throw std::out_of_range(NullPointerError);
		
		return *provider;
	
	}


}

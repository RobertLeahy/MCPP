#include <server.hpp>
#include <format.hpp>
#include <singleton.hpp>
#include <cstdlib>
#include <exception>


#define stringify_impl(x) #x
#define stringify(x) stringify_impl(x)


namespace MCPP {
	
	
	//	Messages
	static const String startup_error="Startup: Unhandled exception";
	static const String exception_what="{0}: {1}";
	static const String endpoint_template="{0}:{1}";
	static const String list_separator=", ";
	static const String binding_to="Startup: Attempting to bind to {0}:{1}";
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
	static const String name_template="{0} {1}";
	
	
	const String Server::BuildDate(
		__DATE__
		" "
		__TIME__
	);
	const String Server::CompiledWith(
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
	const Word Server::MajorVersion=0;
	const Word Server::MinorVersion=0;
	const Word Server::Patch=0;
	const String Server::Name="Minecraft++";


	Server::Server () :
		running(false),
		data(nullptr)
	{
	
		debug=false;
		log_all_packets=false;
		verbose_all=false;
		num_shutdowns=0;
		
		OnReceive=[=] (ReceiveEvent event) {
		
			try {
			
				auto client=Clients[*event.Conn];
				
				//	Loop while a packet can be
				//	parsed
				while (client->Receive(event.Buffer)) Router(
					{
						client,
						client->GetPacket()
					},
					client->GetState()
				);
				
				//	Check buffer
				//	if it's gotten bigger than
				//	allowed, kill the client
				if (
					//	0 = unlimited
					(MaximumBytes!=0) &&
					(
						//	Check both buffers
						(event.Buffer.Count()>MaximumBytes) ||
						(client->Count()>MaximumBytes)
					)
				) client->Disconnect(buffer_too_long);
			
			} catch (...) {
				
				try {
			
					event.Conn->Disconnect(error_processing_recv);
					
				} catch (...) {
				
					Panic(std::current_exception());
					
				}
				
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
		OnReceive=ReceiveType();
	
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
	
	
	void Server::SetDebugPacket (UInt32 packet_id, ProtocolState state, ProtocolDirection direction) {
	
		logged_packets_lock.Write([&] () {
		
			logged_packets.emplace(packet_id,state,direction);
		
		});
	
	}
	
	
	void Server::UnsetDebugPacket (UInt32 packet_id, ProtocolState state, ProtocolDirection direction) noexcept {
	
		Tuple<UInt32,ProtocolState,ProtocolDirection> t(packet_id,state,direction);
	
		logged_packets_lock.Write([&] () {	logged_packets.erase(t);	});
	
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
	
	
	bool Server::LogPacket (UInt32 type, ProtocolState state, ProtocolDirection direction) const noexcept {
	
		//	If we're not debugging at all, short-circuit out
		if (!debug) return false;
		
		//	If we're debugging all packets, return
		//	true immediately
		if (log_all_packets) return true;
		
		Tuple<UInt32,ProtocolState,ProtocolDirection> t(type,state,direction);
		
		//	Lock and determine if we're logging
		//	this particular packet type in this
		//	particular direction
		return logged_packets_lock.Read([&] () {
		
			return logged_packets.find(t)!=logged_packets.end();
		
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
	
	
	ConnectionHandler & Server::Handler () {
	
		if (connections.IsNull()) throw std::out_of_range(NullPointerError);
		
		return *connections;
	
	}

		
	String Server::GetMessageOfTheDay () {

		if (data==nullptr) return String();
		
		Nullable<String> motd=data->GetSetting(motd_setting);
		
		if (motd.IsNull()) return String();
		
		return *motd;

	}
	
	
	static Singleton<Server> singleton;


	Server & Server::Get () noexcept {
	
		return singleton.Get();

	}
	
	
	void Server::Destroy () noexcept {
	
		singleton.Destroy();
	
	}
	
	
	void Server::bind_to (IPAddress ip, UInt16 port) {
		
		WriteLog(
			String::Format(
				binding_to,
				ip,
				port
			),
			Service::LogType::Information
		);
		
		connections->Listen(
			get_endpoint(
				ip,
				port
			)
		);	
	
	}
	
	
	LocalEndpoint Server::get_endpoint (IPAddress ip, UInt16 port) {
	
		LocalEndpoint ep;
		ep.IP=ip;
		ep.Port=port;
		ep.Connect=[this] (ConnectEvent event) mutable {
		
			//	Save IP and port number
			IPAddress ip=event.Conn->IP();
			UInt16 port=event.Conn->Port();
			
			try {
			
				//	Create client object
				auto client=SmartPointer<Client>::Make(std::move(event.Conn));
				
				//	Add to list of connected clients
				Clients.Add(client);
				
				//	Fire event handler
				OnConnect(std::move(client));
			
			} catch (...) {
			
				Panic(std::current_exception());
				
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
		
		};
		ep.Disconnect=[this] (DisconnectEvent event) mutable {
		
			try {
			
				//	Look up the client
				auto client=Clients[*event.Conn];
				
				//	Remove the client from the
				//	list
				Clients.Remove(*event.Conn);
				
				//	Fire event handler
				OnDisconnect(client,event.Reason.IsNull() ? String() : *event.Reason);
			
			} catch (...) {
			
				Panic(std::current_exception());
				
				throw;
			
			}
			
			try {
				
				//	Log
				String disconnect_template;
				
				//	If there's no reason, choose
				//	the template with no reason
				if (
					event.Reason.IsNull() ||
					(event.Reason->Size()==0)
				) {
				
					disconnect_template=disconnected;
				
				} else {
				
					//	Fill the reason into the template
					disconnect_template=String::Format(
						disconnected_with_reason,
						*event.Reason
					);
				
				}
				
				auto clients=Clients.Count();
				WriteLog(
					String::Format(
						disconnect_template,
						event.Conn->IP(),
						event.Conn->Port(),
						clients,
						(clients==1) ? "is" : "are",
						(clients==1) ? "" : "s"
					),
					Service::LogType::Information
				);
				
			} catch (...) {	}
		
		};
		ep.Receive=[this] (ReceiveEvent event) mutable {	OnReceive(std::move(event));	};
		ep.Accept=[this] (AcceptEvent event) mutable {
		
			return OnAccept(
				event.RemoteIP,
				event.RemotePort,
				event.LocalIP,
				event.LocalPort
			);
		
		};
		
		return ep;
	
	}
	
	
	void Server::get_default_binds () {
	
		bind_to(IPAddress::Any(false),default_port);
		bind_to(IPAddress::Any(true),default_port);
	
	}
	
	
	static Nullable<IPAddress> get_ip (String & str) {
	
		str.Trim();
	
		Nullable<IPAddress> retr;
	
		try {
		
			retr.Construct(str);
		
		} catch (...) {	}
		
		return retr;
	
	}
	
	
	static const Regex endpoint_split("(?<!\\:)\\:(?!\\:)");
	
	
	void Server::get_bind (const String & str) {
	
		//	Split on colon to separate IP
		//	address from port
		auto split=endpoint_split.Split(str);
		
		//	Filter out garbage
		if (
			(split.Count()==0) ||
			(split.Count()>2)
		) return;
		
		//	IPs that we'll bind to
		Vector<IPAddress> ips;
		//	Port that we'll bind to
		UInt16 port=default_port;
		
		//	Get the IP
		auto ip=get_ip(split[0]);
		
		//	If it's null we add both
		//	IPv6 and IPv4 addresses
		if (ip.IsNull()) {
		
			ips.Add(IPAddress::Any(true));
			ips.Add(IPAddress::Any(true));
		
		//	Otherwise add the specified
		//	IP
		} else {
		
			ips.Add(*ip);
		
		}
		
		//	If there was a second element in
		//	the split, it's the port, attempt
		//	to get it
		if (split.Count()==2) split[1].ToInteger(&port);
		
		//	BIND
		for (auto & ip : ips) bind_to(ip,port);
	
	}
	
	
	static const Regex bind_split(";");

	
	void Server::get_binds () {
	
		//	Get binds
		auto binds_str=data->GetSetting(binds_setting);
		
		//	If there are no binds,
		//	go with the defaults
		if (binds_str.IsNull()) get_default_binds();
		//	Parse and add binds
		else for (auto & str : bind_split.Split(*binds_str)) get_bind(str);
	
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
		MaximumBytes=data->GetSetting(max_bytes_setting,default_max_bytes);
		
		//	Maximum number of players
		MaximumPlayers=data->GetSetting(max_players_setting,default_max_players);

		//	Initialize a thread pool
		
		//	Attempt to grab number of threads
		//	from the data source
		auto num_threads=data->GetSetting(num_threads_setting,default_num_threads);
		
		//	Panic callback for components
		MCPP::PanicType panic(
			[this] (std::exception_ptr ex) mutable {	Panic(std::move(ex));	}
		);
		
		//	Fire up the thread pool
		pool.Construct(
			num_threads,
			panic
		);
		
		//	Fire up connection handler
		connections.Construct(
			*pool,
			Nullable<Word>(),
			std::move(panic)
		);
		
		//	Install mods
		OnInstall(true);
		mods->Install();
		OnInstall(false);
		
		//	Bind
		get_binds();
	
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
	
	
	String Server::GetName () {
	
		return String::Format(
			name_template,
			Name,
			FormatVersion(
				MajorVersion,
				MinorVersion,
				Patch
			)
		);
	
	}


}

#include <server.hpp>
#include <exception>


namespace MCPP {


	//	Name of server
	static const String service_name="Minecraft++";
	
	
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
	static const String panic_message="Panic!";
	
	
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
	
	
	Nullable<Server> RunningServer;


	Server::Server () :
		Service(service_name),
		running(false),
		data(nullptr),
		Router(true)
	{
	
		debug=false;
		direction=static_cast<Word>(ProtocolDirection::Both);
		log_all_packets=false;
		verbose_all=false;
		
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
	
		OnStop();
	
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
	
	
	void Server::StartInteractive (const Vector<const String> & args) {
	
		is_interactive=true;
	
		OnStart(args);
	
	}
	
	
	
				
			
			
	void Server::WriteLog (const String & message, Service::LogType type) noexcept {
	
		//	Don't throw errors, eat them
		try {
		
			//	Fire event
			try {
			
				OnLog(message,type);
			
			} catch (...) {	}
			
			//	Use the data provider to log
			//	if it's present, otherwise
			//	fall back on default service
			//	logging
			if (data==nullptr) Service::WriteLog(message,type);
			else data->WriteLog(message,type);
			
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
			
			//	If there's no data provider, don't
			//	try and log at all
			if (data!=nullptr) data->WriteChatLog(
				from,
				to,
				message,
				notes
			);
		
		} catch (...) {	}
	
	}
	
	
	void Server::OnStart (const Vector<const String> & args) {
	
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
	
	
	void Server::OnStop () {
	
		//	Hold the state lock before
		//	attempting to change the
		//	server's state
		state_lock.Acquire();
		
		try {
		
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
			
			//	Now we unload all modules
			if (!mods.IsNull()) mods->Unload();
			
			//	Clear all events et cetera
			//	that might have module resources
			//	loaded into them
			try {
			
				OnShutdown();
			
			} catch (...) {
			
				Panic();
				
				throw;
			
			}
			cleanup_events();
			
			
			//	Kill all modules
			mods.Destroy();
			
			//	Clean up data provider
			delete data;
			data=nullptr;
			
			//	Reset this for next time
			//	the server is started
			is_interactive=false;
			
		} catch (...) {
		
			state_lock.Release();
			
			throw;
		
		}
		
		//	Shutdown complete
		running=false;
		
		state_lock.Release();
	
	}
	
	
	void Server::Panic () noexcept {
	
		try {
		
			WriteLog(
				panic_message,
				Service::LogType::Critical
			);
		
		} catch (...) {	}
	
		try {
		
			OnStop();
		
		} catch (...) {	}
		
		//	ABORT
		std::terminate();
	
	}
	
	
	void Server::SetDebug (bool debug) noexcept {
	
		this->debug=debug;
	
	}
	
	
	void Server::SetDebugProtocolDirection (ProtocolDirection direction) noexcept {
	
		this->direction=static_cast<Word>(direction);
	
	}
	
	
	void Server::SetDebugAllPackets (bool log_all_packets) noexcept {
	
		this->log_all_packets=log_all_packets;
	
	}
	
	
	void Server::SetDebugPacket (Byte packet_id) {
	
		logged_packets_lock.Write([&] () {	logged_packets.insert(packet_id);	});
	
	}
	
	
	void Server::SetVerboseAll (bool verbose_all) noexcept {
	
		this->verbose_all=verbose_all;
	
	}
	
	
	void Server::SetVerbose (String verbose) {
	
		verbose_lock.Write([&] () {	this->verbose.insert(std::move(verbose));	});
	
	}
	
	
	bool Server::LogPacket (Byte type, ProtocolDirection direction) const noexcept {
	
		//	If we're not debugging at all, short-circuit out
		if (!debug) return false;
		
		auto d=static_cast<ProtocolDirection>(
			static_cast<Word>(
				this->direction
			)
		);
		
		//	We must be either logging packets in both
		//	directions, or the correct direction, to
		//	log this packet
		if (
			(d!=direction) &&
			(d!=ProtocolDirection::Both)
		) return false;
		
		//	If we're logging all packets, return true
		//	immediately
		if (log_all_packets) return true;
		
		//	Lock and determine if we're logging this
		//	particular packet type
		return logged_packets_lock.Read([&] () {	return logged_packets.count(type)!=0;	});
	
	}
	
	
	bool Server::IsVerbose (const String & key) const noexcept {
	
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


	bool Server::IsInteractive () const noexcept {

		return is_interactive;

	}


	DataProvider & Server::Data () {

		if (data==nullptr) throw std::out_of_range(NullPointerError);

		return *data;

	}


	ThreadPool & Server::Pool () {

		if (pool.IsNull()) throw std::out_of_range(NullPointerError);
		
		return *pool;

	}

		
	String Server::GetMessageOfTheDay () {

		if (data==nullptr) return String();
		
		Nullable<String> motd=data->GetSetting(motd_setting);
		
		if (motd.IsNull()) return String();
		
		return *motd;

	}


	static const Regex date_trim(" {2,}");
	static const RegexReplacement date_trim_r(" ");
	String Server::BuildDate () const {

		return date_trim.Replace(
			String::Format(
				"{0} {1}",
				__DATE__,
				__TIME__
			),
			date_trim_r
		);

	}


	String Server::CompiledWith () const {

		#ifdef __GNUC__
		return String::Format(
			"GNU C++ Compiler (g++) {0}.{1}.{2}",
			__GNUC__,
			__GNUC_MINOR__,
			__GNUC_PATCHLEVEL__
		);
		#else
		return "UNKNOWN";
		#endif

	}


	Server & Server::Get () noexcept {

		return *RunningServer;

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


}

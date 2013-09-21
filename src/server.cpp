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
	
	
	#include "server_setup.cpp"
	#include "server_getters_setters.cpp"


	Server::Server () :
		Service(service_name),
		running(false),
		data(nullptr),
		ProtocolAnalysis(false),
		Direction(ProtocolDirection::Both),
		LogAllPackets(false),
		VerboseAll(false),
		Router(true)
	{
		
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
	
	
	bool Server::LogPacket (Byte type, ProtocolDirection direction) const noexcept {
	
		return (
			//	Protocol analysis has to be on
			ProtocolAnalysis &&
			(
				//	Have to be analyzing traffic in
				//	the appropriate direction...
				(Direction==direction) ||
				//	...or both directions
				(Direction==ProtocolDirection::Both)
			) &&
			(
				//	We have to either be logging all
				//	packets...
				LogAllPackets ||
				//	...or this packet specifically
				(LoggedPackets.find(type)!=LoggedPackets.end())
			)
		);
	
	}
	
	
	bool Server::IsVerbose (const String & key) const noexcept {
	
		return (
			//	Protocol analysis has to be on
			ProtocolAnalysis &&
			(
				//	Either all modules/components are
				//	verbose...
				VerboseAll ||
				//	...or this one specifically is
				(Verbose.find(key)!=Verbose.end())
			)
		);
	
	}


}

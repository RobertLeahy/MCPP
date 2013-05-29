#include <server.hpp>


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
	
	
	//	Constants
	static const String binds_setting="binds";
	static const String num_threads_setting="num_threads";
	static const UInt16 default_port=25565;
	static const Word default_num_threads=10;
	static const String main_thread_desc="Listening Thread";
	
	
	Nullable<Server> RunningServer;
	
	
	#include "server_setup.cpp"


	Server::Server () : Service(service_name), running(false), data(nullptr) {
	
		OnConnect.Add(
			[=] (SmartPointer<Connection> conn) {
				
				IPAddress ip=conn->IP();
				UInt16 port=conn->Port();
			
				Clients.Add(SmartPointer<Client>::Make(std::move(conn)));
				
				WriteLog(
					String::Format(
						"{0}:{1} connected, there are now {2} clients connected",
						ip,
						port,
						Clients.Count()
					),
					Service::LogType::Information
				);
				
			}
		);
		
		OnDisconnect.Add(
			[=] (SmartPointer<Connection> conn, const String & reason) {
			
				Clients.Remove(*conn);	
			
				String disconnect_template;
				
				if (reason.Size()==0) {
				
					disconnect_template="{0}:{1} disconnected, there are now {2} clients connected";
				
				} else {
				
					disconnect_template=String::Format(
						"{{0}}:{{1}} disconnected (with reason: \"{0}\"), there are now {{2}} clients connected",
						reason
					);
				
				}
				
				WriteLog(
					String::Format(
						disconnect_template,
						conn->IP(),
						conn->Port(),
						Clients.Count()
					),
					Service::LogType::Information
				);
			
			}
		);
		
		OnReceive=[=] (SmartPointer<Connection> conn, Vector<Byte> & buffer) {
		
			try {
			
				auto client=Clients[*conn];
				auto & packet=client->InProgress;
				
				//	Attempt to finish a packet
				if (packet.FromBytes(buffer)) {
				
					//	Success, route
					
					Router(client,std::move(packet));
				
				} else {
				
					//	TODO: Buffer too long etc. checks
				
				}
			
				/*
				auto client=Clients[*conn];
				auto & packet=client->InProgress;
				
				if (packet.FromBytes(buffer)) {
				
					if (packet.Type()==0xFE) {
					
						Packet packet;
						packet.SetType<PacketTypeMap<0xFF>>();
						
						String & message=packet.Retrieve<String>(0);
						CodePoint null=0;
						
						message	<< "§1" << null
								<< "61" << null
								<< "1.5.2" << null
								<< "Minecraft++ Test" << null
								<< String(Clients.Count()) << null
								<< String(std::numeric_limits<SByte>::max());
								
						auto handle=client->Conn()->Send(packet.ToBytes());
						
						handle->Wait();
						
						client->Conn()->Disconnect("Ping complete");
					
					}
				
				}*/
			
			} catch (...) {
			
				conn->Disconnect("Error receiving data");
				
				throw;
			
			}
		
		};
	
	}
	
	
	Server::~Server () noexcept {
	
		OnStop();
	
		//if (data!=nullptr) delete data;
	
	}
	
	
	#ifdef DEBUG
	void Server::TestStart () {
	
		OnStart(Vector<const String>());
	
	}
	#endif
	
	
	void Server::WriteLog (const String & message, Service::LogType type) {
	
		auto callback=[=] () {
		
			//	Don't throw errors, eat them
			try {
			
				if (data!=nullptr) {
				
					data->WriteLog(message,type);
					
					OnLog(message,type);
				
				} else {
				
					Service::WriteLog(message,type);
				
				}
				
				#ifdef DEBUG
				console_lock.Execute([&] () {	StdOut << message << Newline;	});
				#endif
			
			} catch (...) {	}
		
		};
		
		//	Is there a thread pool?
		if (pool.IsNull()) {
		
			//	Log synchronously
			callback();
		
		} else {
		
			//	Log asynchronously
			pool->Enqueue(callback);
		
		}
	
	}
	
	
	void Server::OnStart (const Vector<const String> & args) {
	
		//	Server is already running, can't
		//	start a running server
		if (state_lock.Execute(
			[this] () {
			
				if (running) return true;
				
				running=true;
				
				return false;
			
			}
		)) return;
		
		//	START
		try {
		
			try {

				server_startup();
				
			} catch (...) {
			
				if (data!=nullptr) {
				
					delete data;
					data=nullptr;
					
				}
				
				socket_server.Destroy();
				connections.Destroy();
				pool.Destroy();
			
				state_lock.Execute([=] () {	running=false;	});
				
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
		
	
	}
	
	
	void Server::OnStop () {
	
		//	Server is already stopped, can't
		//	stop a stopped server
		if (state_lock.Execute(
			[this] () {
			
				if (!running) return true;
				
				running=false;
				
				return false;
			
			}
		)) return;
		
		//	STOP
		
		//	First we stop the flow of
		//	new connections by killing
		//	the listening handler
		socket_server.Destroy();
		
		//	Next we disconnect all
		//	connected clients
		connections.Destroy();
		
		//	Now we wait on all pending
		//	tasks and then kill the
		//	thread pool
		pool.Destroy();
		
		//	Clean up data provider
		/*delete data;
		data=nullptr;*/
	
	}


}

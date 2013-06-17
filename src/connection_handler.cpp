#include <connection_manager.hpp>


namespace MCPP {


	static const Word max=SocketSet::MaxSetSize();
	static const String purge_error("Error disconnecting {0}:{1}");
	static const String purge_error_what("Error disconnecting {0}:{1}: {2}");


	ConnectionHandler::ConnectionHandler (ConnectionManager & parent) : stop(false), normal_shutdown(false), sync(2), parent(&parent) {
	
		//	Attempt to start send thread
		send=Thread(send_thread,this);
		
		//	Attempt to start receive thread
		try {
		
			recv=Thread(recv_thread,this);
		
		} catch (...) {
		
			//	Creating receive thread failed
			
			//	Tell send thread it needs to shutdown
			stop=true;
			
			//	Release it
			sync.Enter();
			
			//	Wait for it
			send->Join();
			
			//	Propagate error
			throw;
		
		}
	
	}


	ConnectionHandler::~ConnectionHandler () noexcept {
	
		//	Order threads to stop
		stop=true;
		
		send->Join();
		recv->Join();
		
		//	Purge all connections
		//	so that they disassociate from this
		//	handler
		try {
		
			purge_connections(connections);
			
		} catch (...) {
		
			//	We have to make sure that
			//	the connections disconnect
			//	from the handler or we'll
			//	have access violations
			//	on our hands
			for (auto & conn : connections) conn->disconnect();
		
		}

		//	Safe for automatic cleanup now
	
	}
	
	
	Word ConnectionHandler::Count () noexcept {
	
		connections_lock.Read();
		
		Word count=connections.Count();
		
		connections_lock.CompleteRead();
		
		return count;
	
	}
	
	
	Word ConnectionHandler::Max () noexcept {
	
		return max;
	
	}
	
	
	Word ConnectionHandler::Available () noexcept {
	
		return Max()-Count();
	
	}
	
	
	inline void ConnectionHandler::purge_connections (Vector<SmartPointer<Connection>> purge) {
	
		//	Avoid locking unnecessarily
		if (purge.Count()!=0) {
		
			connections_lock.Write();
			
			try {
				
				for (auto & conn : purge) {
				
					//	Disassociate connection from
					//	this handler
					conn->disconnect();
				
					//	Search handler's list of connections
					for (Word i=0;i<connections.Count();++i) {
					
						if (static_cast<Connection *>(connections[i])==static_cast<Connection *>(conn)) {

							//	Capture these callbacks.
							//
							//	This avoids a situation wherein
							//	the connection manager is
							//	destroyed while the asynchronous
							//	disconnect handling callback
							//	is still running, which leads
							//	to access violations or simply
							//	insidious behaviour.
							const auto & disconnect=parent->disconnect;
							const auto & panic=parent->panic;
							const auto & log_callback=parent->log;
							parent->pool->Enqueue([=] () mutable {
							
								String reason;
								try {
								
									reason=conn->reason_lock.Execute([&] () {	return conn->reason;	});
								
								} catch (...) {	}
								
								try {
								
									try {
									
										disconnect(conn,reason);
									
									} catch (const std::exception & e) {
									
										try {
										
											log_callback(
												String::Format(
													purge_error_what,
													conn->IP(),
													conn->Port(),
													e.what()
												),
												Service::LogType::Error
											);
										
										} catch (...) {	}
										
										throw;
									
									} catch (...) {
									
										try {
										
											log_callback(
												String::Format(
													purge_error,
													conn->IP(),
													conn->Port()
												),
												Service::LogType::Error
											);
										
										} catch (...) {	}
									
										throw;
									
									}
								
								} catch (...) {
								
									try {
									
										panic();
									
									} catch (...) {	}
									
									throw;
								
								}
							
							});
							
							//	Delete it
							connections.Delete(i);
							
							//	No need to loop anymore,
							//	in fact due to the above
							//	looping is probably dangerous
							break;
						
						}
					
					}
				
				}
			
			} catch (...) {
			
				connections_lock.CompleteWrite();
				
				throw;
			
			}
			
			connections_lock.CompleteWrite();
		
		}
	
	}


}


#include "receive_thread.cpp"
#include "send_thread.cpp"

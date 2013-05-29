#include <connection_manager.hpp>


namespace MCPP {


	static const Word max=SocketSet::MaxSetSize();


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
	
	
	inline void ConnectionHandler::purge_connections (Vector<SmartPointer<Connection>> & purge) {
	
		if (purge.Count()!=0) {
		
			connections_lock.Write();
			
			try {
			
				for (Word i=0;i<connections.Count();) {
				
					bool found=false;
					
					for (Word n=0;n<purge.Count();++n) {
					
						if (static_cast<Connection *>(connections[i])==static_cast<Connection *>(purge[n])) {
						
							found=true;
							
							//	Fire disconnect handler
							//
							//	We don't catch when this throws,
							//	because if it's fails that's
							//	very bad.
							parent->pool->Enqueue(
								[this] (SmartPointer<Connection> conn) mutable {
								
									String reason=conn->reason_lock.Execute(
										[this,conn] () {
										
											try {
										
												try {
											
													return conn->reason;
													
												} catch (const std::exception & e) {
												
													//	TODO: Log
													
													throw;
												
												} catch (...) {
												
													//	TODO: Log
													
													throw;
												
												}
												
											} catch (...) {
											
												parent->panic();
												
												throw;
												
											}
										
										}
									);
								
									try {
									
										try {
										
											parent->disconnect(conn,reason);
										
										} catch (const std::exception & e) {
										
											//	TODO: Log
										
										} catch (...) {
										
											//	TODO: Log
										
											throw;
										
										}
										
									} catch (...) {
									
										parent->panic();
										
										throw;
									
									}
								
								},
								connections[i]
							);
							
							connections.Delete(i);
							purge.Delete(n);
							
							break;
						
						}
					
					}
					
					if (purge.Count()==0) break;
					
					if (!found) ++i;
				
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

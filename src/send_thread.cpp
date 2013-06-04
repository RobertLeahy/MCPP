#include <connection_manager.hpp>


namespace MCPP {


	//	Duration of send sleep on condvar
	static Word send_sleep_timeout=50;
	//	Duration of wait for sockets to change state
	static Word send_wait_timeout=0;


	void ConnectionHandler::send_thread (void * ptr) {
	
		ConnectionHandler * ch=reinterpret_cast<ConnectionHandler *>(ptr);
		
		try {
		
			try {
			
				ch->send_thread_impl();
			
			} catch (const std::exception & e) {
			
				//	TODO: Log
			
				throw;
			
			} catch (...) {
			
				//	TODO: Log
			
				throw;
			
			}
			
		} catch (...) {
		
			try {
		
				ch->parent->panic();
				
			} catch (...) {	}
			
			throw;
		
		}
	
	}
	
	
	void ConnectionHandler::send_thread_impl () {
	
		//	Wait for startup to complete
		sync.Enter();
		
		try {
		
			//	Loop until told to shut down
			while (!stop) {
			
				//	Check for connections that have
				//	pending data, if there are none
				//	sleep
				send_lock.Acquire();
				
				try {
				
					//	Flag to be set if there are pending
					//	writes
					bool pending=false;
				
					connections_lock.Read();
					
					try {
					
						for (SmartPointer<Connection> & conn : connections) {
						
							if (!(
								conn->saturated ||
								(conn->send_queue.Count()==0) ||
								conn->disconnect_flag
							)) {
							
								pending=true;
								
								break;
							
							}
						
						}
					
					} catch (...) {
					
						connections_lock.CompleteRead();
						
						throw;
					
					}
					
					connections_lock.CompleteRead();
					
					//	Nothing to do, wait
					if (!pending) send_sleep.Sleep(
						send_lock,
						send_sleep_timeout
					);
				
				} catch (...) {
				
					send_lock.Release();
					
					throw;
				
				}
				
				send_lock.Release();
			
				//	Create set of sockets
				//	which need to be written
				Word num=0;
				SocketSet write;
				
				//	List of connections that
				//	must be purged
				//
				//	The average scan will not have
				//	connections that need to be purged,
				//	so we save ourself a heap allocation
				//	be allocating this vector with zero
				//	capacity
				Vector<SmartPointer<Connection>> purge(0);
				
				connections_lock.Read();
				
				try {
				
					for (SmartPointer<Connection> & conn : connections) {
					
						//	Is this socket to be disconnected?
						if (conn->disconnect_flag) {
						
							purge.EmplaceBack(conn);
						
						//	Otherwise, handle sending
						} else {
					
							//	Clear saturated flag
							conn->saturated=false;
						
							//	Check for data to be written
							if (conn->send_queue.Count()!=0) {
							
								//	If data must be written, check the socket
								write.Add(conn->socket);
								
								++num;
								
							}
							
						}
					
					}
					
					//	Check sockets
					if (
						(num!=0) &&
						(Socket::Select(nullptr,&write,nullptr,send_wait_timeout)!=0)
					)
					for (SmartPointer<Connection> & conn : connections)
					if (write.Contains(conn->socket)) {
					
						//	Obtain lock
						conn->send_lock.Acquire();
						
						try {
						
							//	Write
							while (conn->send_queue.Count()!=0) {
							
								Vector<Byte> & buffer=conn->send_queue[0].Item<0>();
								SmartPointer<SendHandle> & handle=conn->send_queue[0].Item<1>();
								
								//	No longer pending
								handle->lock.Acquire();
								handle->state=SendState::Sending;
								handle->wait.WakeAll();
								handle->lock.Release();
								
								//	Number of bytes we're going to try and send
								Word num_bytes=buffer.Count();
								//	Number of bytes actually sent
								Word bytes_sent=0;
							
								//	Whether we sent all bytes
								bool sent_all=false;
								
								//	Attempt to send
								try {
								
									sent_all=(bytes_sent=conn->socket.Send(&buffer))==num_bytes;
								
								} catch (...) {
								
									//	Socket exceptions cause
									//	connection to be purged
									purge.EmplaceBack(conn);
									
									break;
								
								}
								
								//	Update sent count in handle
								handle->sent+=bytes_sent;
								
								if (sent_all) {
								
									//	Sent all bytes successfully, notify
									//	handle
									handle->lock.Acquire();
									handle->state=SendState::Sent;
									
									try {
									
										for (auto & callback : handle->callbacks) {
										
											parent->pool->Enqueue(
												callback,
												SendState::Sent
											);
										
										}
										
									} catch (...) {
									
										handle->wait.WakeAll();
										handle->lock.Release();
										
										throw;
									
									}
									
									handle->wait.WakeAll();
									handle->lock.Release();
									
									//	Purge buffer to move onto next
									//	buffer (if there is one)
									conn->send_queue.Delete(0);
									
								} else {
								
									//	Didn't send all bytes, mark
									//	this socket as saturated and
									//	abort (for now)
									conn->saturated=true;
									
									break;
									
								}
							
							}
						
						} catch (...) {
						
							conn->send_lock.Release();
							
							throw;
						
						}
						
						conn->send_lock.Release();
					
					}
				
				} catch (...) {
				
					connections_lock.CompleteRead();
					
					throw;
				
				}
				
				connections_lock.CompleteRead();
				
				//	Purge
				
				purge_connections(purge);
			
			}
			
		//	Something went wrong, command receive
		//	thread to shut down
		} catch (...) {
		
			stop=true;
			
			//	Propagate error so it gets logged
			throw;
		
		}
		
		//	DIE
	
	}


}

#include <connection_manager.hpp>


namespace MCPP {


	//	Duration of send sleep on condvar
	static Word send_sleep_timeout=50;
	//	Duration of wait for sockets to change state
	static Word send_wait_timeout=0;
	static const String send_thread_error("Error in send thread");
	static const String send_thread_error_what("Error in send thread: {0}");


	void ConnectionHandler::send_thread (void * ptr) {
	
		ConnectionHandler * ch=reinterpret_cast<ConnectionHandler *>(ptr);
		
		try {
		
			try {
			
				ch->send_thread_impl();
			
			} catch (const std::exception & e) {
			
				try {
				
					ch->parent->log(
						String::Format(
							send_thread_error_what,
							e.what()
						),
						Service::LogType::Error
					);
				
				} catch (...) {	}
			
				throw;
			
			} catch (...) {
			
				try {
				
					ch->parent->log(
						send_thread_error,
						Service::LogType::Error
					);
				
				} catch (...) {	}
			
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
	
		#ifdef DEBUG
		try {	parent->log("Send thread up",Service::LogType::Information);	} catch (...) {	}
		#endif
	
		//	Wait for startup to complete
		sync.Enter();
		
		try {
		
			//	Create poll state object
			//	to use in polling
			PollState ps;
			ps.SetWrite();
		
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
						
							//	If a connection has been flagged for
							//	disconnect, move to disconnect it
							//	at once
							if (conn->disconnect_flag) {
							
								pending=true;
								
								break;
							
							}
							
							//	Don't check for pending data
							//	if we marked the socket as saturated,
							//	we'd just waste CPU cycles spinning
							//	waiting for the data to get flushed out
							//	across the network
							if (!conn->saturated) {
							
								conn->send_lock.Acquire();
								if (conn->send_queue.Count()!=0) pending=true;
								conn->send_lock.Release();
								
								if (pending) break;
							
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
					
						//	If this socket has to be disconnected
						//	do not attempt operations on it,
						//	just queue it for removal
						if (conn->disconnect_flag) {
						
							purge.EmplaceBack(conn);
						
						//	Otherwise we can handle sending
						} else {
						
							//	Clear the saturated flag
							conn->saturated=false;
							
							//	Acquire send lock
							conn->send_lock.Acquire();
							
							try {
							
								//	Loop until all data has been sent,
								//	or socket becomes no longer writable
								//	without blocking
								while (
									(conn->send_queue.Count()!=0) &&
									conn->socket.Poll(ps,send_wait_timeout).Write()
								) {
								
									//	Retrieve next item to be sent
									Vector<Byte> & buffer=conn->send_queue[0].Item<0>();
									SmartPointer<SendHandle> & handle=conn->send_queue[0].Item<1>();
									
									//	Change send handle state
									//	to sending if it isn't
									//	sending already
									handle->lock.Acquire();
									if (handle->state!=SendState::Sending) {
									
										handle->state=SendState::Sending;
										handle->wait.WakeAll();
									
									}
									handle->lock.Release();
									
									//	Number of bytes we're going to try
									//	and send
									Word num_bytes=buffer.Count();
									//	Number of bytes actually sent
									//	(for comparison)
									Word bytes_sent=0;
									
									//	Attempt to send
									try {
									
										bytes_sent=conn->socket.Send(&buffer);
									
									} catch (...) {
									
										//	Socket exceptions cause connection
										//	to be purged
										purge.EmplaceBack(conn);
										
										//	We also do not attempt to write
										//	anymore
										break;
									
									}
									
									//	Update sent count in handle
									handle->sent+=bytes_sent;
									//	Update sent count in connection
									conn->sent+=bytes_sent;
									
									if (bytes_sent==num_bytes) {
									
										//	Sent all bytes successfully, notify
										//	handle
										handle->lock.Acquire();
										handle->state=SendState::Sent;
										handle->wait.WakeAll();
										
										//	Fire callbacks
										try {
										
											for (auto & callback : handle->callbacks) {
											
												parent->pool->Enqueue(
													callback,
													SendState::Sent
												);
											
											}
										
										} catch (...) {
										
											handle->lock.Release();
											
											throw;
										
										}
										
										handle->lock.Release();
										
										//	Purge buffer to move onto next
										//	buffer (if there is one)
										conn->send_queue.Delete(0);
									
									}
								
								}
								
								//	If we failed to write all data
								//	mark socket as saturated
								if (conn->send_queue.Count()!=0) conn->saturated=true;
								
							} catch (...) {
							
								conn->send_lock.Release();
								
								throw;
							
							}
							
							conn->send_lock.Release();
						
						}
					
					}
				
				} catch (...) {
				
					connections_lock.CompleteRead();
					
					throw;
				
				}
				
				connections_lock.CompleteRead();
				
				//	Purge
				
				purge_connections(std::move(purge));
			
			}
			
		//	Something went wrong, command receive
		//	thread to shut down
		} catch (...) {
		
			stop=true;
			
			#ifdef DEBUG
			try {	parent->log("Send thread down",Service::LogType::Information);	} catch (...) {	}
			#endif
			
			//	Propagate error so it gets logged
			throw;
		
		}
		
		#ifdef DEBUG
		try {	parent->log("Send thread down",Service::LogType::Information);	} catch (...) {	}
		#endif
		
		//	DIE
	
	}


}

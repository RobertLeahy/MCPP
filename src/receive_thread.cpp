#include <connection_manager.hpp>


namespace MCPP {


	//	Duration of receive thread's wait for
	//	sockets to have data
	static const Word recv_wait_timeout=50;
	static const String recv_thread_error("Error in receive thread");
	static const String recv_thread_error_what("Error in receive thread: {0}");
	
	
	void ConnectionHandler::recv_thread (void * ptr) {
	
		ConnectionHandler * ch=reinterpret_cast<ConnectionHandler *>(ptr);
		
		//	Did an error occur?
		//
		//	I.e. should we panic?
		bool error=false;
		
		try {
		
			try {
			
				ch->recv_thread_impl();
			
			} catch (const std::exception & e) {
			
				try {
				
					ch->parent->log(
						String::Format(
							recv_thread_error_what,
							e.what()
						),
						Service::LogType::Error
					);
				
				} catch (...) {	}
				
				throw;
			
			} catch (...) {
			
				try {
				
					ch->parent->log(
						recv_thread_error,
						Service::LogType::Error
					);
				
				} catch (...) {	}
				
				throw;
			
			}
			
		} catch (...) {
		
			//	We should panic
			error=true;
		
		}
		
		//	We cannot move on until
		//	the send thread has stopped
		ch->send->Join();
		
		//	There are three situations wherein a handler
		//	is shutting down:
		//
		//	1.	The server is shutting down and the parent
		//		is therefore commanding its shutdown.
		//	2.	The handler has no connections and is
		//		therefore cleaning itself up.
		//	3.	An error has occurred.
		//
		//	In situations 2 and 3 the handler must
		//	reach out to the parent and remove itself,
		//	but in situation 1 the parent is removing us
		//	which means that if we tried reaching into
		//	the parent and removing ourselves we'd be
		//	waiting on the write lock, and the parent
		//	would be holding the write lock waiting
		//	for us to clean ourselves up -- i.e.
		//	dead lock.
		//
		//	The way around this is that we asynchronously
		//	try to clean ourselves up, keeping track of
		//	the number of pending handler cleanups through
		//	a counter in the parent.
		//
		//	We know the parent will not clean itself up
		//	until this thread has ended, therefore
		//	accessing the parent through the pointer
		//	we have to the parent is always safe, and
		//	we know that the parent won't clean itself up
		//	until that list of tasks flushes out, so
		//	accessing the parent through pointers in
		//	that asynchronous callback is always safe.
		//
		//	However, what if by some freak accident
		//	the asynchronous callback doesn't fire until
		//	after the parent has completed cleaning up
		//	all handlers?
		//
		//	To hedge against this we use a barrier so
		//	that we know the asynchronous callback
		//	is safe before we die.
		
		ConnectionManager & cm=*(ch->parent);
		
		Barrier cleanup_safe(2);
		
		try {
		
			cm.pool->Enqueue(
				[ch,&cm,&cleanup_safe] () {
				
					//	Increment the count in the parent
					cm.cleanup_lock.Acquire();
					
					++cm.cleanup_count;
					
					cm.cleanup_lock.Release();
					
					//	Tell parent to move on
					cleanup_safe.Enter();
					
					//	Cleanup parent
					cm.handlers_lock.Acquire();
					
					//	Scan list of handlers
					for (Word i=0;i<cm.handlers.Count();++i) {
					
						//	Is this the one?
						if (static_cast<ConnectionHandler *>(cm.handlers[i])==ch) {
						
							//	KILL
							
							cm.handlers.Delete(i);
							
							break;
						
						}
					
					}
					
					cm.handlers_lock.Release();
					
					//	Decrement count and alent waiting
					//	manager cleanup
					cm.cleanup_lock.Acquire();
					
					--cm.cleanup_count;
					
					cm.cleanup_wait.WakeAll();
					
					cm.cleanup_lock.Release();
					
					//	DONE
				
				}
			);
			
			//	Wait for worker...
			cleanup_safe.Enter();
		
		} catch (...) {
		
			//	We should definitely panic,
			//	parent might be in invalid
			//	state
			error=true;
		
		}
		
		#ifdef DEBUG
		try {	ch->parent->log("Receive thread down",Service::LogType::Information);	} catch (...) {	}
		#endif
		
		//	Panic of any of this was the
		//	result of, or resulted in an
		//	error
		if (error) {
		
			try {
		
				cm.panic();
				
			} catch (...) {	}
			
		}
	
	}
	
	
	void ConnectionHandler::recv_thread_impl () {
	
		#ifdef DEBUG
		try {	parent->log("Receive thread up",Service::LogType::Information);	} catch (...) {	}
		#endif
	
		//	Release send thread
		sync.Enter();
		
		try {
		
			//	Poll state to use
			//	in polling
			PollState ps;
			ps.SetRead();
		
			//	Loop until told to stop
			while (!stop) {
			
				//	Attempt to grab new connections
				//	pending on the parent
				
				while (Available()!=0) {
				
					Nullable<SmartPointer<Connection>> conn=parent->Dequeue();
					
					if (conn.IsNull()) break;
					
					(*conn)->connect(&send_lock,&send_sleep);
					
					connections_lock.Write();
					
					try {
					
						connections.EmplaceBack(std::move(*conn));
					
					} catch (...) {
					
						connections_lock.CompleteWrite();
						
						throw;
					
					}
					
					connections_lock.CompleteWrite();
				
				}
				
				//	List of connections that
				//	must be purged
				//
				//	The average scan will not have
				//	connections that need to be purged,
				//	so we save ourself a heap allocation
				//	be allocating this vector with zero
				//	capacity
				Vector<SmartPointer<Connection>> purge(0);
				
				//	Receive
				
				connections_lock.Read();
				
				//	List we'll build select set from
				Vector<SmartPointer<Connection>> select_conn(0);
				
				try {
				
					//	If there are no connections,
					//	kill this handler
					if (connections.Count()==0) {
					
						//	Stop the send thread
						stop=true;
						
						//	Release the lock
						connections_lock.CompleteRead();
						
						//	Break out of the infinite loop
						break;
					
					}
					
					//	Create copy of connections list
					//	(just smart pointers, not expensive)
					//	so we can release the lock while we
					//	select.
					//
					//	Most of the time releasing the lock wouldn't
					//	matter, but it can render the send thread
					//	unresponsive for a brief time after
					//	a disconnect occurs.
					//
					//	We could populate the set to select on,
					//	then release the lock, but that wouldn't
					//	cause the lifetime of the sockets to
					//	dilate, which means select might throw.
					select_conn=connections;
					
				} catch (...) {
				
					connections_lock.CompleteRead();
					
					throw;
				
				}
				
				connections_lock.CompleteRead();
				
				//	Build select set
				SocketSet read;
				Word num=0;
			
				//	Get all sockets
				for (SmartPointer<Connection> & conn : connections) {
				
					read.Add(conn->socket);
				
					++num;
					
				}
				
				//	Only proceed if there are sockets
				//	to select from
				if (num!=0) {

					if (Socket::Select(&read,nullptr,nullptr,recv_wait_timeout)!=0) {
						
						//	Acquire lock
						connections_lock.Read();
						
						try {
						
							for (SmartPointer<Connection> & conn : connections)
							if (read.Contains(conn->socket)) {
							
								//	Lock to receive data
								conn->recv_lock.Acquire();
								
								try {
								
									//	Did we kill the client?
									bool killed=false;
									
									//	In order to avoid blocking
									//	while data is being handled
									//	we use an alternate buffer
									//	while a task processing
									//	received data is running
									//
									//	This ensures that data is
									//	processed/received contiguously
									//	and that this thread doesn't
									//	block
									bool use_primary=!conn->recv_task;
									
									Vector<Byte> & buffer=use_primary ? conn->recv : conn->recv_alt;
									
									//	Amount of data in the buffer
									//	before the receive
									Word before=buffer.Count();
									
									//	We know we can read at least some
									//	data, but we don't know how much
									//	
									//	To avoid the overhead of looping
									//	again just to read from this
									//	socket once more, we poll the socket
									//	and keep reading until either it
									//	dies or until it's no longer
									//	flagged as readable
									do {
									
										//	If the buffer is at maximum
										//	capacity, make it bigger
										if (buffer.Capacity()==buffer.Count()) buffer.SetCapacity();
										
										//	Try and receive
										try {
										
											Word count=conn->socket.Receive(&buffer);
											
											//	If we received nothing, that
											//	means that the socket was flagged
											//	readable but had no data to be read,
											//	i.e. the client disconnected
											if (count==0) {
											
												killed=true;
												
												purge.EmplaceBack(conn);
											
											}
										
										} catch (const SocketException &) {
										
											//	Exception means something
											//	went wrong on the socket,
											//	so kill it
											
											killed=true;
											
											purge.EmplaceBack(conn);
										
										}
									
									} while (
										!killed &&	//	Don't try and read from a socket that's dead
										conn->socket.Poll(
											ps,
											0	//	Don't block
										).Read()
									);
									
									//	If we're reading into
									//	the primary buffer,
									//	and if we added to that
									//	buffer, we need to dispatch
									//	a worker to process
									//	that data
									if (
										use_primary &&
										(buffer.Count()!=before)
									) {
									
										//	Capture this
										const auto & recv=parent->recv;
										//	Dispatch worker
										parent->pool->Enqueue([=] () mutable {
										
											//	We come back here if there's
											//	more data to process after
											//	the callback completes
											loop:
										
											//	Invoke the processing
											//	callback
											try {
											
												recv(
													conn,
													conn->recv
												);
											
											} catch (...) {	}
											
											//	Check to see if more
											//	data was written into
											//	the alternate buffer
											//	while we processed
											
											conn->recv_lock.Acquire();
											
											try {
											
												//	If there's more data,
												//	copy the alternate
												//	buffer and loop
												if (conn->recv_alt.Count()!=0) {
												
													conn->recv.Add(
														conn->recv_alt.begin(),
														conn->recv_alt.end()
													);
													
													conn->recv_alt.SetCount(0);
													
													//	Be sure to release the lock...
													conn->recv_lock.Release();
													
													//	LOOP
													goto loop;
												
												}
											
											} catch (...) {
											
												conn->recv_task=false;
												conn->recv_wait.WakeAll();
												conn->recv_lock.Release();
												
												throw;
											
											}
											
											conn->recv_task=false;
											conn->recv_wait.WakeAll();
											conn->recv_lock.Release();
										
										});
										
										//	Flag a task as being
										//	in progress
										conn->recv_task=true;
									
									}
									
								} catch (...) {
								
									conn->recv_lock.Release();
									
									throw;
								
								}
								
								conn->recv_lock.Release();
							
							}
				
						} catch (...) {
						
							connections_lock.CompleteRead();
							
							throw;
						
						}
						
						connections_lock.CompleteRead();
						
					}
					
				}
				
				//	If necessary, purge
				//	connections
				
				purge_connections(std::move(purge));
			
			}
		
		} catch (...) {
		
			//	Tell other thread to shut down
			stop=true;
			
			//	Propagate error so it gets logged
			throw;
		
		}
	
	}


}

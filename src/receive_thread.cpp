#include <connection_manager.hpp>


namespace MCPP {


	//	Duration of receive thread's wait for
	//	sockets to have data
	static const Word recv_wait_timeout=50;
	
	
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
			
				//	TODO: Log
				
				throw;
			
			} catch (...) {
			
				//	TODO: Log
				
				throw;
			
			}
			
		} catch (...) {
		
			//	We should panic
			error=true;
		
		}
		
		//	We cannot move on until
		//	the send thread has stopped
		ch->send->Join();
		
		//	We must disconnect all connections from
		//	this handler before we can safely
		//	clean it up
		for (SmartPointer<Connection> & conn : ch->connections) {
		
			conn->connected_lock.Acquire();
			conn->connected=false;
			conn->connected_lock.Release();
		
		}
		
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
	
		//	Release send thread
		sync.Enter();
		
		try {
		
			//	Loop until told to stop
			while (!stop) {
			
				//	Attempt to grab new connections
				//	pending on the parent
				
				while (Available()!=0) {
				
					Nullable<SmartPointer<Connection>> conn=parent->Dequeue();
					
					if (conn.IsNull()) break;
					
					(*conn)->Connect(&send_lock,&send_sleep);
					
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
				
					//	Select
					if (Socket::Select(&read,nullptr,nullptr,recv_wait_timeout)!=0) {
						
						//	Acquire lock
						connections_lock.Read();
						
						try {
						
							for (SmartPointer<Connection> & conn : connections)
							if (read.Contains(conn->socket)) {
							
								//	Receive data
								conn->recv_lock.Acquire();
								
								try {
								
									//	If a receive processing task is running
									//	we read into the alternate buffer
									bool use_primary=!conn->recv_task;
									
									//	If we're reading into the primary buffer,
									//	there's no task, but there's a chance
									//	there's data in the alternate buffer,
									//	so copy it over
									if (use_primary) {
									
										//	Copy alternate buffer
										conn->recv.Add(
											conn->recv_alt.begin(),
											conn->recv_alt.end()
										);
										
										//	Clear alternate buffer
										conn->recv_alt.SetCount(0);
									
									}
									
									Vector<Byte> & buffer=use_primary ? conn->recv : conn->recv_alt;
									
									//	If buffer is as large is possible, resize
									if (buffer.Capacity()==buffer.Count()) buffer.SetCapacity();
									
									//	Attempt to receive into buffer
									try {
									
										if (conn->socket.Receive(&buffer)==0) {
										
											//	Remote end closed, die
											purge.EmplaceBack(conn);
										
										} else {
										
											//	Receive successful
											
											//	If we're reading into the
											//	main buffer, dispatch event
											if (use_primary) {
											
												parent->pool->Enqueue(
													[this,conn] () mutable {
													
														loop:
													
														//	Invoke callback
														try {
														
															parent->recv(
																conn,
																conn->recv
															);
														
														} catch (...) {	}
														
														//	Check for more data
														conn->recv_lock.Acquire();
														
														try {
														
															//	If there's more data, continue
															//	after copying buffers
															if (conn->recv_alt.Count()!=0) {
															
																conn->recv.Add(
																	conn->recv_alt.begin(),
																	conn->recv_alt.end()
																);
																
																conn->recv_alt.SetCount(0);
																
																//	Make sure we release lock
																conn->recv_lock.Release();
																
																//	Back to the top...
																goto loop;
															
															}
														
														} catch (...) {
													
															conn->recv_lock.Release();
															
															conn->recv_task=false;
															
															conn->recv_wait.WakeAll();
															
															throw;
													
														}
														
														//	Clear the task 
														conn->recv_task=false;
														
														//	We're done, alert
														conn->recv_wait.WakeAll();
														
														conn->recv_lock.Release();
													
													}
												);
												
												//	Task in progress...
												conn->recv_task=true;
												
											}
										
										}
									
									} catch (const SocketException & e) {
									
										//	Connection was closed ungracefully
										purge.EmplaceBack(conn);
									
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
				
				purge_connections(purge);
			
			}
		
		} catch (...) {
		
			//	Tell other thread to shut down
			stop=true;
			
			//	Propagate error so it gets logged
			throw;
		
		}
	
	}


}

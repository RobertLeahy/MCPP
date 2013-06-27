namespace MCPP {


	//	Maximum number of events the worker
	//	will receive at once
	static const Word max_events=64;
	//	For passing to epoll_create
	static const Word epoll_size=32;


	//
	//	SEND HANDLE
	//
	
	
	SendHandle::SendHandle (Vector<Byte> send) noexcept
		:	send(std::move(send)),
			sent(0),
			state(SendState::Pending)
	{	}
	
	
	//
	//	CONNECTION
	//
	
	
	bool Connection::disconnect () noexcept {
	
		//	Obtain self lock
		lock.Acquire();
		
		//	Do not proceed if we're already
		//	been shutdown
		if (is_shutdown) {
		
			lock.Release();
			
			return false;
		
		}
		
		//	Shutdown
		shutdown(socket,SHUT_RDWR);
		is_shutdown=true;
		
		lock.Release();
		
		return true;
	
	}
	
	
	Connection::Connection (
		int socket,
		Endpoint endpoint,
		int fd
	)	:	socket(socket),
			endpoint(endpoint),
			fd(fd),
			pending_recv(false),
			recv(recv_buf_size),
			recv_alt(recv_buf_size),
			is_shutdown(false),
			is_registered(false)
	{
	
		sent=0;
		received=0;
	
	}
	
	
	Connection::~Connection () noexcept {
	
		//	Release all threads waiting on
		//	sends that will never complete
		for (auto & handle : sends) {
		
			handle->lock.Acquire();
			handle->state=SendState::Failed;
			//	Fire all callbacks
			for (auto & c : handle->callbacks) try {
			
				c(SendState::Failed);
			
			} catch (...) {	}
			handle->wait.WakeAll();
			handle->lock.Release();
		
		}
		
		close(socket);
	
	}
	
	
	SmartPointer<SendHandle> Connection::Send (Vector<Byte> buffer) {
	
		//	Prepare a send handle to represent
		//	this send operation
		auto handle=SmartPointer<SendHandle>::Make(std::move(buffer));
		
		//	Lock to send
		lock.Acquire();
		
		try {
		
			//	Are we disconnected?
			if (is_shutdown) {
			
				//	YES, fail at once
				
				handle->state=SendState::Failed;
			
			} else {
			
				//	Store the send handle
				sends.EmplaceBack(handle);
				
				//	Data is now ready to be sent,
				//	modify this socket within the
				//	epoll fd to watch for sendability
				//	but only if we've been registered
				//	with the epoll fd
				if (is_registered) {
				
					struct epoll_event event;
					event.data.ptr=this;
					event.events=EPOLLIN|EPOLLOUT|EPOLLET;
					
					if (epoll_ctl(
						fd,
						EPOLL_CTL_MOD,
						socket,
						&event
					)!=0) {
					
						//	Cleanup and throw
						sends.Delete(sends.Count()-1);
						
						throw std::system_error(
							std::error_code(
								errno,
								std::system_category()
							)
						);
					
					}
				
				}
				
				//	Data's all setup to be sent
			
			}
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
		
		return handle;
	
	}
	
	
	Word Connection::Pending () const noexcept {
	
		lock.Acquire();
		auto pending=sends.Count();
		lock.Release();
		
		return pending;
	
	}


	//
	//	CONNECTION HANDLER
	//
	
	
	static inline void set_nonblocking (int fd) {
	
		int old_flags=fcntl(fd,F_GETFL);
		
		if (
			(old_flags==-1) ||
			(fcntl(
				fd,
				F_SETFL,
				old_flags|O_NONBLOCK
			)==-1)
		) throw std::system_error(
			std::error_code(
				errno,
				std::system_category()
			)
		);
	
	}
	
	
	static inline void set_blocking (int fd) {
	
		int old_flags=fcntl(fd,F_GETFL);
		
		if (
			(old_flags==-1) ||
			(fcntl(
				fd,
				F_SETFL,
				old_flags&(~O_NONBLOCK)
			)==-1)
		) throw std::system_error(
			std::error_code(
				errno,
				std::system_category()
			)
		);
	
	}
	
	
	ConnectionHandler::ConnectionHandler (
		const Vector<Endpoint> & binds,
		AcceptCallback accept_callback,
		ConnectCallback connect_callback,
		DisconnectCallback disconnect_callback,
		ReceiveCallback recv,
		LogCallback log_callback,
		PanicCallback panic_callback,
		ThreadPool & pool
	)	:	disconnect_callback(disconnect_callback),
			recv(recv),
			connect_callback(connect_callback),
			accept_callback(accept_callback),
			log_callback(log_callback),
			panic_callback(panic_callback),
			pool(pool),
			bound(binds.Count())
	{
	
		//	Create a socket pair
		if (socketpair(
			PF_LOCAL,
			SOCK_STREAM,
			0,
			pair
		)!=0) throw std::system_error(
			std::error_code(
				errno,
				std::system_category()
			)
		);
		
		try {
		
			//	The end that the worker will wait on
			//	is non-blocking
			set_nonblocking(pair[1]);
			//	The end that the destructor will use
			//	is blocking
			set_blocking(pair[0]);
	
			//	Attempt to bind sockets
			for (auto & ep : binds) {
			
				int socket=::socket(
					ep.IP().IsV6() ? AF_INET6 : AF_INET,
					SOCK_STREAM,
					IPPROTO_TCP
				);
				
				if (socket==-1) SocketException::Raise(errno);
				
				try {
				
					//	Make our listening sockets
					//	non-blocking
					set_nonblocking(socket);
				
					struct sockaddr_storage addr;
					ep.IP().ToOS(
						&addr,
						ep.Port()
					);
					
					if (
						(bind(
							socket,
							reinterpret_cast<struct sockaddr *>(&addr),
							sizeof(struct sockaddr_storage)
						)==-1) ||
						(listen(
							socket,
							SOMAXCONN
						)==-1)
					) SocketException::Raise(errno);
					
					bound.Add(socket);
				
				} catch (...) {
				
					close(socket);
					
					throw;
				
				}
			
			}
			
			//	Create epoll fd
			if ((fd=epoll_create(epoll_size))==-1) throw std::system_error(
				std::error_code(
					errno,
					std::system_category()
				)
			);
			
			try {
			
				//	Associate one end of the socket
				//	pair with the epoll fd
				struct epoll_event event;
				event.data.ptr=nullptr;	//	Null identifies own socket pair
				event.events=EPOLLIN|EPOLLET;	//	We want to read commands
				
				//	Add to the epoll fd
				if (epoll_ctl(
					fd,
					EPOLL_CTL_ADD,
					pair[1],
					&event
				)!=0) throw std::system_error(
					std::error_code(
						errno,
						std::system_category()
					)
				);
				
				//	Add bound sockets to the epoll fd
				for (auto s : bound) {
				
					event.data.ptr=&bound;	//	Pointer to bound identifies server socket
					//	We still want to read (i.e. accept)
					//	so don't change that member
					
					if (epoll_ctl(
						fd,
						EPOLL_CTL_ADD,
						s,
						&event
					)!=0) throw std::system_error(
						std::error_code(
							errno,
							std::system_category()
						)
					);
				
				}
				
				//	epoll has been setup, launch
				//	worker
				worker=Thread([=] () {	worker_func();	});
			
			} catch (...) {
			
				//	Ensure epoll fd gets
				//	closed
				close(fd);
				
				throw;
			
			}
			
		} catch (...) {
		
			//	Ensure bound sockts (if any)
			//	get closed
			for (auto s : bound) close(s);
		
			//	Ensure pair gets closed
			close(pair[0]);
			close(pair[1]);
			
			throw;
		
		}
	
	}
	
	
	ConnectionHandler::~ConnectionHandler () noexcept {
	
		//	Tell the worker thread to stop
		Byte b=0;
		send(pair[0],&b,1,0);
		
		//	Wait for the worker to end
		worker.Join();
		
		//	Disconnect all connections that are
		//	still associated with this handler
		//	and attempt to fire their disconnect
		//	callbacks.
		for (auto & pair : connections) {
		
			pair.second->Disconnect();
			
			try {
			
				remove(static_cast<Connection *>(pair.second));
			
			} catch (...) {	}
			
		}
		
		//	Kill the listening sockets
		for (auto s : bound) close(s);
		
		//	Kill the socket pair used for
		//	inter thread communication
		close(pair[0]);
		close(pair[1]);
		
		//	Wait for all async events
		//	to end
		async_lock.Acquire();
		while (running_async!=0) async_wait.Sleep(async_lock);
		async_lock.Release();
		
		//	Close the epoll fd
		close(fd);
	
	}
	
	
	inline void ConnectionHandler::remove (Connection * conn) {
	
		//	In old kernel versions there's
		//	a bug where even when deleting
		//	the struct epoll_event * can't
		//	be null, accomodate this
		struct epoll_event event;
	
		//	YES, disassociate it
		//	from the epoll fd
		if (epoll_ctl(
			fd,
			EPOLL_CTL_DEL,
			conn->socket,
			&event
		)==-1) throw std::system_error(
			std::error_code(
				errno,
				std::system_category()
			)
		);
		
		//	Launch disconnect handler
		//	and remove connection from
		//	handler
		connections_lock.Acquire();
		auto iter=connections.find(conn);
		auto conn_handle=std::move(iter->second);
		connections.erase(iter);
		connections_lock.Release();
		
		++running_async;
		try {
		
			pool.Enqueue(
				[this] (SmartPointer<Connection> conn) {
				
					try {
					
						auto & reason=conn->reason;
					
						disconnect_callback(
							std::move(conn),
							reason
						);
					
					} catch (...) {	}
					
					end_async();
				
				},
				std::move(conn_handle)
			);
		
		} catch (...) {
		
			--running_async;
			
			throw;
		
		}
	
	}
	
	
	void ConnectionHandler::worker_func () noexcept {
	
		//	Don't throw
		try {
		
			//	Prepare a buffer into which to
			//	receive events
			struct epoll_event events [max_events];
		
			//	Loop forever
			for (;;) {
			
				//	Wait on epoll
				int num=epoll_wait(
					fd,
					events,
					max_events,
					-1	//	INFINITE
				);
				
				//	Error checking
				if (num==-1) {
				
					//	Ignore interrupts
					if (errno==EINTR) continue;
				
					throw std::system_error(
						std::error_code(
							errno,
							std::system_category()
						)
					);
					
				}
				
				//	Loop for all available connections
				for (Word i=0;i<static_cast<Word>(num);++i) {
				
					auto & event=events[i];
					
					//	Data ready to be received on
					//	control socket
					//
					//	Right now all the control socket
					//	does is tell us to shut down, so
					//	do so...
					if (event.data.ptr==nullptr) goto end;
					
					//	New connection available
					if (event.data.ptr==&bound) {
					
						//	Loop over bound sockets
						for (auto s : bound) {
						
							//	Loop so long as there are
							//	new connections waiting
							for (;;) {
							
								struct sockaddr_storage addr;
								socklen_t addr_len=sizeof(struct sockaddr_storage);
							
								int socket=accept(
									s,
									reinterpret_cast<struct sockaddr *>(&addr),
									&addr_len
								);
								
								//	Did not accept a connection
								if (socket==-1) {
								
									//	Would block, carry on
									if (
										(errno==EAGAIN)
										#if EAGAIN!=EWOULDBLOCK
										|| (errno==EWOULDBLOCK)
										#endif
									) break;
									
									//	ERROR
									SocketException::Raise(errno);
								
								}
								
								try {
								
									//	Setup the connection
									set_nonblocking(socket);
									
									//	Create an endpoint for
									//	the connection
									Endpoint ep(get_endpoint(&addr));
									
									//	Create a connection handle to
									//	encapsulate the connection
									auto handle=SmartPointer<Connection>::Make(
										socket,
										std::move(ep),
										fd
									);
									
									//	Dispatch asynchronous
									//	handler
									++running_async;
									try {
									
										pool.Enqueue(
											[this] (SmartPointer<Connection> conn) {
											
												//	Install into handler
												connections_lock.Acquire();
												
												try {
												
													connections.emplace(
														static_cast<Connection *>(conn),
														conn
													);
												
												} catch (...) {
												
													connections_lock.Release();
													
													end_async();
													
													try {	panic_callback();	} catch (...) {	}
													
													throw;
												
												}
												
												connections_lock.Release();
												
												//	Callback
												try {
												
													connect_callback(conn);
												
												} catch (...) {
												
													//	Callback throws, kill
													//	the connection
													kill(static_cast<Connection *>(conn));
													
													//	Remove it from the handler
													try {
													
														remove(static_cast<Connection *>(conn));
														
													} catch (...) {
													
														try {	panic_callback();	} catch (...) {	}
													
													}
													
													end_async();
													
													throw;
												
												}
												
												//	Lock so that the state is
												//	consistent
												conn->lock.Acquire();
												
												//	Has the connection been killed
												//	already?
												if (conn->is_shutdown) {
												
													//	Do not proceed, allow
													//	the connection to die
													conn->lock.Release();
													
													try {
													
														remove(static_cast<Connection *>(conn));
													
													} catch (...) {
													
														end_async();
													
														try {	panic_callback();	} catch (...) {	}
														
														throw;
													
													}
													
													end_async();
													
													return;
												
												}
												
												//	Start receive loop by subscribing
												//	this socket to the epoll fd
												struct epoll_event event;
												event.data.ptr=static_cast<Connection *>(conn);
												event.events=EPOLLIN|EPOLLET;
												
												//	If there are pending writes, also
												//	subscribe initially for writes
												if (conn->sends.Count()!=0) event.events|=EPOLLOUT;
												
												//	Subscribe
												if (epoll_ctl(
													fd,
													EPOLL_CTL_ADD,
													conn->socket,
													&event
												)==-1) {
												
													conn->lock.Release();
												
													//	Kill the connection
													kill(static_cast<Connection *>(conn));
													
													try {
													
														remove(static_cast<Connection *>(conn));
													
													} catch (...) {
													
														try {	panic_callback();	} catch (...) {	}
													
													}
													
													end_async();
													
													throw std::system_error(
														std::error_code(
															errno,
															std::system_category()
														)
													);
												
												}
												
												//	Registered to epoll fd now
												conn->is_registered=true;
												
												conn->lock.Release();
												
												end_async();
											
											},
											std::move(handle)
										);
									
									} catch (...) {
									
										--running_async;
										
										throw;
									
									}
									
								} catch (...) {
								
									close(socket);
									
									throw;
								
								}
							
							}
						
						}
					
					//	Operation ready on some miscellaneous
					//	socket
					} else {
					
						//	Data is a pointer to a Connection
						//	object
						auto & conn=*reinterpret_cast<Connection *>(event.data.ptr);
						
						//	Check what event(s) occurred
						if (!(
							((event.events&EPOLLHUP)==0) &&
							#ifdef EPOLLRDHUP
							((event.events&EPOLLRDHUP)==0) &&
							#endif
							((event.events&EPOLLERR)==0)
						)) {
						
							//	Client has gone away
							
							kill(&conn);
						
						}
						
						//	Socket is readable
						if ((event.events&EPOLLIN)!=0) {
						
							//	Did we kill the connection?
							bool killed=false;
						
							//	Lock the connection so we
							//	can access its receive
							//	buffers
							conn.lock.Acquire();
							
							try {
							
								//	Decide which buffer to use
								Vector<Byte> & buffer=conn.pending_recv ? conn.recv_alt : conn.recv;
								//	Number of bytes we actually read,
								//	for use in deciding whether to
								//	dispatch a handler or not
								SafeWord num_read;
							
								//	Loop until we encounter an error
								//	or read all data
								for (;;) {
								
									//	If the buffer is full, make it bigger
									if (buffer.Capacity()==buffer.Count()) buffer.SetCapacity();
									
									//	Attempt to receive
									ssize_t result=::recv(
										conn.socket,
										static_cast<Byte *>(buffer)+buffer.Count(),
										size_t(SafeWord(buffer.Capacity()-buffer.Count())),
										0
									);
									
									//	Check for error
									if (result==-1) {
									
										//	If the call would block, that's
										//	fine, just stop reading, otherwise
										//	kill the connection.
										if (!(
											(errno==EAGAIN)
											#if EAGAIN!=EWOULDBLOCK
											|| (errno==EWOULDBLOCK)
											#endif
										)) killed=true;
										
										//	Stop reading
										break;
									
									}
									
									try {
									
										SafeWord recvd(static_cast<Word>(result));
									
										//	Increment the number of bytes
										//	we read
										num_read+=recvd;
										
										Word buffer_count=Word(SafeWord(buffer.Count())+recvd);
										
										//	Set the buffer count appropriately
										buffer.SetCount(buffer_count);
										
									} catch (...) {
									
										//	Kill connection on overflows
									
										killed=true;
										
										break;
									
									}
									
									//	And increment the number of bytes
									//	received in the Connection object
									conn.received+=static_cast<UInt64>(result);
									
									//	Did we read nothing?  That would
									//	indicate that the other end
									//	hung up
									if (result==0) {
										
										killed=true;
										
										break;
									
									}
								
								}
								
								//	Did we read anything?
								if (static_cast<Word>(num_read)!=0) {
								
									//	YES
									
									//	Were we reading into the primary buffer?
									if (!conn.pending_recv) {
									
										//	NO, we're good to launch
										//	an asynchronous callback
										
										//	Get the handle
										connections_lock.Acquire();
										auto conn_handle=connections[&conn];
										connections_lock.Release();
										
										conn.pending_recv=true;
										++running_async;
										try {
										
											pool.Enqueue(
												[this] (SmartPointer<Connection> conn) {
												
													retry_recv:
													
													//	Dispatch the receive callback
													try {
													
														recv(conn,conn->recv);
													
													} catch (...) {
													
														//	If the receive callback throws
														//	kill the connection
														kill(static_cast<Connection *>(conn));
														
														conn->lock.Acquire();
														conn->pending_recv=false;
														conn->lock.Release();
														
														//	...and bail out
														end_async();
														return;
													
													}
													
													//	Everything went smoothly,
													//	check for more data in the
													//	secondary buffer
													conn->lock.Acquire();
													
													try {
													
														//	More data
														if (conn->recv_alt.Count()!=0) {
														
															//	Copy it over
															conn->recv.Add(
																conn->recv_alt.begin(),
																conn->recv_alt.end()
															);
															
															conn->recv_alt.SetCount(0);
															
															//	Release lock and dispatch
															//	receive callback again
															conn->lock.Release();
															
															goto retry_recv;
														
														}
													
													} catch (...) {
													
														//	Abort
														conn->pending_recv=false;
													
														conn->lock.Release();
														
														kill(static_cast<Connection *>(conn));
														
														end_async();
														
														throw;
													
													}
													
													//	Done
													conn->pending_recv=false;
													
													conn->lock.Release();
													
													end_async();
													
												},
												std::move(conn_handle)
											);
										
										} catch (...) {
										
											--running_async;
											conn.pending_recv=false;
											
											throw;
										
										}
									
									}
								
								}
							
							} catch (...) {
							
								conn.lock.Release();
								
								throw;
							
							}
							
							conn.lock.Release();
							
							//	Did we kill the connection?
							if (killed) {
							
								//	YES
							
								kill(&conn);
							
								remove(&conn);
								
								//	Don't try to send
								continue;
							
							}	
						
						}
						
						//	Socket is writable
						if ((event.events&EPOLLOUT)!=0) {
						
							//	We don't wait for this event
							//	by default, this is added
							//	to the socket-in-question
							//	whenever a write occurs
							
							//	Lock the connection so we
							//	can access its send handles
							conn.lock.Acquire();
							
							try {
							
								//	Loop until broken or until
								//	we flush all the buffers out
								//	to the wire
								while (conn.sends.Count()!=0) {
								
									//	Grab the first handle
									//	in the queue
									auto & handle=conn.sends[0];
									
									//	Send
									ssize_t sent=send(
										conn.socket,
										static_cast<Byte *>(handle->send)+handle->sent,
										size_t(SafeWord(handle->send.Count()-handle->sent)),
										#ifdef MSG_NOSIGNAL
										MSG_NOSIGNAL
										#else
										0
										#endif
									);
									
									//	That's an error
									if (sent==-1) {
									
										//	Call would've blocked, so we're
										//	done with this socket for now
										if (
											(errno==EAGAIN)
											#if EAGAIN!=EWOULDBLOCK
											|| (errno==EWOULDBLOCK)
											#endif
										) break;
										
										//	Actually an error, kill this
										//	socket
										kill(&conn);
									
										//	Stop handling sends
										break;
									
									}
									
									//	We sent some data
									handle->sent+=static_cast<Word>(sent);
									
									//	Update connection's counter
									conn.sent+=static_cast<UInt64>(sent);
									
									//	Did we send all the data?
									if (handle->sent==handle->send.Count()) {
									
										//	Make a copy for the async
										//	callback
										auto handle_cpy=handle;
										
										conn.sends.Delete(0);
									
										//	Deploy an asynchronous callback
										//	to deal with this
										pool.Enqueue([=] () mutable {
										
											handle_cpy->lock.Acquire();
											handle_cpy->state=SendState::Sent;
											for (auto & c : handle_cpy->callbacks) try {
											
												c(SendState::Sent);
											
											} catch (...) {	}
											handle_cpy->wait.WakeAll();
											handle_cpy->lock.Release();
										});
									
									}
										
								}
								
								//	Did we flush it all out?
								if (conn.sends.Count()==0) {
								
									//	YES, no longer wait on
									//	EPOLLOUT
									struct epoll_event event;
									event.data.ptr=&conn;
									event.events=EPOLLIN|EPOLLET;
									
									if (epoll_ctl(
										fd,
										EPOLL_CTL_MOD,
										conn.socket,
										&event
									)!=0) throw std::system_error(
										std::error_code(
											errno,
											std::system_category()
										)
									);
								
								}
								
							} catch (...) {
							
								conn.lock.Release();
								
								throw;
							
							}
							
							conn.lock.Release();
						
						}
					
					}
				
				}
			
			}
		
		} catch (...) {
		
			try {	panic_callback();	} catch (...) {	}
		
		}
		
		//	Exit worker
		end:;
	
	}


}

namespace MCPP {


	//	ConnectionHandler accept thread startup error
	static const char * accept_startup_error="Error launching accept thread";
	
	
	//
	//	OVERLAPPED DATA
	//
	
	
	OverlappedData::OverlappedData (NetworkCommand command) noexcept : Command(command) {
	
		memset(this,0,sizeof(WSAOVERLAPPED));
	
	}
	
	
	//
	//	SEND HANDLE
	//
	
	
	SendHandle::SendHandle (Vector<Byte> send) noexcept
		:	overlapped(NetworkCommand::CompleteSend),
			send(std::move(send)),
			state(SendState::Pending)
	{
	
		//	Zero out overlapped structure
		//	as required
		memset(&overlapped,0,sizeof(WSAOVERLAPPED));
		
		//	Initialize buffer
		buf.len=static_cast<u_long>(this->send.Length());
		buf.buf=reinterpret_cast<char FAR *>(
			static_cast<Byte *>(
				this->send
			)
		);
	
	}
	
	
	//
	//	CONNECTION
	//


	bool Connection::disconnect () noexcept {
	
		//	Obtain self lock
		lock.Acquire();
		
		//	Have we already been shutdown?
		if (is_shutdown) {
		
			//	YES, DO NOT PROCEED
			lock.Release();
			
			return false;
		
		}
		
		//	NO, SHUTDOWN
		shutdown(socket,SD_BOTH);
		is_shutdown=true;
		
		//	Done
		lock.Release();
		
		return true;
	
	}
	
	
	Connection::Connection (
		SOCKET socket,
		Endpoint endpoint,
		HANDLE iocp
	)	:	socket(socket),
			endpoint(endpoint),
			pending_recv(false),
			recv(recv_buf_size),
			overlapped(NetworkCommand::BeginReceive),
			recv_flags(0),
			is_shutdown(false)
	{
	
		//	Add ourselves to the completion
		//	port
		if (CreateIoCompletionPort(
			reinterpret_cast<HANDLE>(socket),
			iocp,
			reinterpret_cast<ULONG_PTR>(this),
			0
		)!=iocp) throw std::system_error(
			std::error_code(
				GetLastError(),
				std::system_category()
			)
		);
	
		//	Initialize counters
		sent=0;
		received=0;
	
	}
	
	
	Connection::~Connection () noexcept {
	
		//	Release all threads waiting
		//	on sends that will never
		//	complete
		for (auto & pair : sends) {
		
			pair.second->lock.Acquire();
			pair.second->state=SendState::Failed;
			//	Fire all callbacks
			for (auto & c : pair.second->callbacks) try {
			
				c(SendState::Failed);
			
			} catch (...) {	}
			pair.second->wait.WakeAll();
			pair.second->lock.Release();
		
		}
	
		closesocket(socket);
	
	}
	
	
	SmartPointer<SendHandle> Connection::Send (Vector<Byte> buffer) {
	
		//	Prepare a send handle for this
		//	send operation
		auto handle=SmartPointer<SendHandle>::Make(std::move(buffer));
		
		//	If the send fails, we kill the
		//	connection, but we have to unlock
		//	or else there'll be a deadlock
		bool kill=false;
		
		//	Lock to send
		lock.Acquire();
		
		try {
		
			//	Are we disconnected?
			if (is_shutdown) {
			
				//	YES -- we fail straight away
				
				handle->state=SendState::Failed;
			
			} else {
			
				//	Store the send handle
				sends.emplace(
					static_cast<SendHandle *>(handle),
					handle
				);
			
				//	Asynchronously send data
				int result=WSASend(
					socket,
					&(handle->buf),
					1,
					nullptr,
					0,
					&(handle->overlapped),
					nullptr
				);
				
				//	Success is all data being sent
				//	immediately, or data being
				//	enqueued
				if (!(
					(result==0) ||
					(WSAGetLastError()==WSA_IO_PENDING)
				)) {
				
					//	FAILURE
					
					//	Mark socket for death
					kill=true;
					
					//	Remove pending send
					sends.erase(static_cast<SendHandle *>(handle));
					
					//	Mark send as FAILED
					handle->state=SendState::Failed;
				
				}
			
			}
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
		
		//	If send failed, kill the connection
		if (kill) Disconnect();
		
		return handle;
	
	}
	
	
	//
	//	CONNECTION HANDLER
	//
	
	
	inline void ConnectionHandler::remove (SmartPointer<Connection> conn) {
	
		//	Remove from handler
		connections_lock.Acquire();
		connections.erase(static_cast<Connection *>(conn));
		connections_lock.Release();
		
		//	Deploy async callback
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
				std::move(conn)
			);
		
		} catch (...) {
		
			--running_async;
			
			throw;
		
		}
	
	}
	
	
	//	Worker thread
	//
	//	Handles completion port operations.
	void ConnectionHandler::worker_func () noexcept {
	
		//	DO NOT THROW
		try {
		
			//	Loop forever
			for (;;) {
			
				//	Setup to get queued tasks
				DWORD num_bytes;
				OverlappedData * data;
				Connection * conn;
				
				//	Dequeue
				int result=GetQueuedCompletionStatus(
					iocp,
					&num_bytes,
					reinterpret_cast<PULONG_PTR>(&conn),
					reinterpret_cast<LPOVERLAPPED *>(&data),
					INFINITE
				);
				
				//	Success?
				if (!result) {
				
					//	Did we dequeue?
					if (data==nullptr) {
					
						//	This is bad, abort
						throw std::system_error(
							std::error_code(
								GetLastError(),
								std::system_category()
							)
						);
					
					}
				
				}
				
				//	Were we passed a null overlapped
				//	structure?
				//
				//	That's a request for us to end
				if (data==nullptr) break;
				
				connections_lock.Acquire();
				
				//	Make sure this connection
				//	can't be deleted while we're
				//	working on it
				auto conn_handle=connections[conn];
				
				connections_lock.Release();
				
				//	What instruction have we been given?
				switch (data->Command) {
					
					//	A send has finished
					case NetworkCommand::CompleteSend:{
					
						//	The overlapped structure we received
						//	is actually a pointer to a SendHandle
						auto & handle=*reinterpret_cast<SendHandle *>(data);
					
						//	Increment the connection's
						//	sent bytes count
						conn->sent+=num_bytes;
						
						//	Complete the send
						conn->lock.Acquire();
						auto iter=conn->sends.find(&handle);
						auto handle_ptr=iter->second;
						conn->sends.erase(iter);
						conn->lock.Release();
						
						//	Was the send successful?
						if (result && (num_bytes==handle.send.Count())) {
						
							//	Complete asynchronously so callbacks
							//	do not stall this thread
							pool.Enqueue([=] () mutable {
							
								handle_ptr->lock.Acquire();
								
								handle_ptr->state=SendState::Sent;
								
								//	Callbacks
								for (auto & c : handle_ptr->callbacks) try {
								
									c(SendState::Sent);
								
								} catch (...) {	}
								
								handle_ptr->wait.WakeAll();
								
								handle_ptr->lock.Release();
							
							});
						
						} else {
							
							kill(conn);
						
						}
					
					}break;
					
					//	A receive has completed
					case NetworkCommand::CompleteReceive:{
					
						//	We make sure there's room in the
						//	buffer before dispatching, which
						//	means that a number of bytes
						//	received of zero means the client
						//	disconnected.
						if ((num_bytes==0) || !result) {
						
							kill(conn);
							
							conn->lock.Acquire();
							conn->pending_recv=false;
							conn->lock.Release();
							
							break;
							
						}
						
						//	Increment number of bytes received
						//	on this connection
						conn->received+=num_bytes;
						
						//	Set the count of the receive buffer
						Word recv_count;
						try {
						
							recv_count=Word(
								SafeWord(conn->recv.Count())+
								SafeWord(num_bytes)
							);
							
						} catch (...) {
						
							//	Kill connection on overflow
							kill(conn);
							
							conn->lock.Acquire();
							conn->pending_recv=false;
							conn->lock.Release();
							
							break;
						
						}
						
						conn->recv.SetCount(recv_count);
						
						//	Dispatch an asynchronous event
						//	to handle this.
						++running_async;
						
						try {
						
							pool.Enqueue(
								[this] (SmartPointer<Connection> conn) {
							
									//	Dispatch the receive
									//	callback
									try {
									
										recv(conn,conn->recv);
									
									} catch (...) {
									
										//	If the receive callback throws,
										//	kill the connection...
										kill(static_cast<Connection *>(conn));
										
										conn->lock.Acquire();
										conn->pending_recv=false;
										
										if (conn->is_shutdown && (conn->sends.size()==0)) try {
										
											remove(std::move(conn));
											
										} catch (...) {
										
											end_async();
											
											conn->lock.Release();
											
											try {	panic_callback();	} catch (...) {	}
											
											throw;
										
										}
										
										conn->lock.Release();
										
										//	...and bail out
										end_async();
										
										throw;
									
									}
									
									//	Everything went smoothly,
									//	queue up another receive
									conn->overlapped.Command=NetworkCommand::BeginReceive;
									
									//	Zero out the overlapped structure
									memset(&conn->overlapped,0,sizeof(OVERLAPPED));
									
									//	If this fails there's literally
									//	nothing we can do about it, so
									//	react by killing the connection
									//	so at least it's not leaked
									if (!PostQueuedCompletionStatus(
										iocp,
										0,
										reinterpret_cast<ULONG_PTR>(static_cast<Connection *>(conn)),
										reinterpret_cast<LPOVERLAPPED>(&(conn->overlapped))
									)) {
										
										kill(static_cast<Connection *>(conn));
										
										end_async();
										
										conn->lock.Acquire();
										conn->pending_recv=false;
										
										if (conn->is_shutdown && (conn->sends.size()==0)) try {
										
											remove(std::move(conn));
											
										} catch (...) {	}
										
										conn->lock.Release();
										
										try {	panic_callback();	} catch (...) {	}
										
										throw std::system_error(
											std::error_code(
												GetLastError(),
												std::system_category()
											)
										);
									
									}
									
									//	Release the destructor
									//	(if it's waiting on us)
									end_async();
									
								},
								conn_handle
							);
							
						} catch (...) {
						
							--running_async;
							
							throw;
						
						}
						
					}break;
					
					//	A receive has been requested
					case NetworkCommand::BeginReceive:{
					
						//	We've been asked to receive
						
						//	Is the connection's receive
						//	buffer full?
						//
						//	If so, resize
						if (conn->recv.Count()==conn->recv.Capacity()) conn->recv.SetCapacity();
						
						//	Setup buffer
						conn->recv_buf.len=static_cast<u_long>(
							conn->recv.Capacity()-conn->recv.Count()
						);
						conn->recv_buf.buf=reinterpret_cast<char FAR *>(
							static_cast<Byte *>(
								conn->recv
							)+conn->recv.Count()
						);
						
						//	Next command will be to complete
						//	the receive
						data->Command=NetworkCommand::CompleteReceive;
						
						//	Receive into that buffer
						int result=WSARecv(
							conn->socket,
							&(conn->recv_buf),
							1,
							nullptr,
							&(conn->recv_flags),
							data,
							nullptr
						);
						
						//	Error?
						if (!(
							//	Zero is returned when recv succeeds at once
							(result==0) ||
							//	An "error" may just mean that the IO will
							//	be performed sometime in the future,
							//	check for that
							(WSAGetLastError()==WSA_IO_PENDING)
						)) {
						
							//	Actually an error, KILL
							kill(conn);
							
							//	We didn't perform that receive,
							//	because it failed, so there's
							//	no longer a pending receive
							conn->lock.Acquire();
							conn->pending_recv=false;
							conn->lock.Release();
						
						}
					
					}break;
					
					default:break;
				
				}
				
				//	Reach into this connection
				//	and see if it's fit for removal
				
				conn->lock.Acquire();
				
				try {
				
					//	Remove if...
					if (
						//	There are no pending receives
						!conn->pending_recv &&
						//	The socket has been shutdown
						conn->is_shutdown &&
						//	All sends have completed
						(conn->sends.size()==0)
					) remove(std::move(conn_handle));
					
				} catch (...) {
				
					conn->lock.Release();
					
					throw;
				
				}
				
				conn->lock.Release();
			
			}
		
		} catch (...) {
		
			try {	panic_callback();	} catch (...) {	}
		
		}
	
	}
	
	
	int ConnectionHandler::accept_filter (LPWSABUF addr, LPWSABUF, LPQOS, LPQOS, LPWSABUF, LPWSABUF, GROUP FAR *, DWORD_PTR ptr) noexcept {
	
		//	Get the endpoint
		auto ep=get_endpoint(
			reinterpret_cast<struct sockaddr_storage *>(
				addr->buf
			)
		);
		
		try {
		
			//	Call the filter function
			if (
				reinterpret_cast<ConnectionHandler *>(
					ptr
				)->accept_callback(
					ep.IP(),
					ep.Port()
				)
			) return CF_ACCEPT;
		
		} catch (...) {	}
		
		//	Fallback to reject -- the
		//	callback has to explicitly
		//	approve incoming connections
		return CF_REJECT;
	
	}
	
	
	void ConnectionHandler::accept_func () noexcept {
	
		//	Startup
		
		//	Create an array of events
		//	for use and reuse
		
		WSAEVENT events [2];
		events[1]=WSACreateEvent();
		
		//	Verify that event was created
		//	properly
		if (events[1]==WSA_INVALID_EVENT) {
		
			handshake=false;
			
			barrier.Enter();
			
			return;
		
		}
		
		//	Add our events to the handle
		for (auto s : bound) {
		
			if (WSAEventSelect(
				s,
				events[1],
				FD_ACCEPT
			)!=0) {
			
				handshake=false;
				
				WSACloseEvent(events[1]);
				
				barrier.Enter();
				
				return;
				
			}
		
		}
		
		//	Startup successful
		barrier.Enter();
		
		events[0]=stop;
	
		//	Do not throw
		try {
		
			//	Loop until told to shutdown
			while (WSAWaitForMultipleEvents(
				2,
				events,
				false,
				INFINITE,
				false
			)!=WSA_WAIT_EVENT_0) {
				
				//	Iterate to discover
				//	which sockets have pending
				//	connections
				for (auto s : bound) {
				
					WSANETWORKEVENTS status;
					
					if (WSAEnumNetworkEvents(
						s,
						events[1],
						&status
					)!=0) throw std::system_error(
						std::error_code(
							WSAGetLastError(),
							std::system_category()
						)
					);
					
					//	Connection waiting on this socket
					if (status.lNetworkEvents!=0) {
					
						struct sockaddr_storage addr;
						int addr_size=sizeof(struct sockaddr_storage);
						
						//	ACCEPT
						SOCKET socket=WSAAccept(
							s,
							reinterpret_cast<struct sockaddr *>(&addr),
							&addr_size,
							accept_filter,
							reinterpret_cast<DWORD_PTR>(this)
						);
						
						//	Error checking
						if (socket==INVALID_SOCKET) {
						
							auto last_error=WSAGetLastError();
							
							//	If the condition function
							//	rejected or deferred the connection,
							//	just loop
							if (
								(last_error==WSAECONNREFUSED) ||
								(last_error==WSATRY_AGAIN)
							) continue;
							
							//	Otherwise that's a serious problem
							SocketException::Raise(last_error);
						
						}
						
						try {
						
							//	CONNECTION ACCEPTED
							
							//	Create a connection object
							//	to wrap it.
							auto conn=SmartPointer<Connection>::Make(
								socket,
								get_endpoint(&addr),
								iocp
							);
							
							//	Dispatch asynchronous handler
							++running_async;
							try {
							
								pool.Enqueue(
									[this] (SmartPointer<Connection> conn) {
									
										//	Install into the handler
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
										
											//	Callback throws, kill the
											//	connection
											kill(static_cast<Connection *>(conn));
											
											//	And remove it from the handler
											conn->lock.Acquire();
											bool erase=conn->sends.size()==0;
											conn->lock.Release();
											
											if (erase) try {
											
												remove(std::move(conn));
											
											} catch (...) {
											
												try {	panic_callback();	} catch (...) {	}
											
											}
											
											end_async();
											
											throw;
										
										}
										
										//	Lock so that the state is
										//	consistent
										conn->lock.Acquire();
										
										//	Has connection been killed
										//	already?
										//
										//	Or is it a candidate to be
										//	killed?
										if (conn->is_shutdown) {
										
											//	Do not proceed, allow
											//	the connection to die
											bool erase=conn->sends.size()==0;
											
											conn->lock.Release();
											
											if (erase) try {
											
												remove(std::move(conn));
											
											} catch (...) {
											
												end_async();
												
												try {	panic_callback();	} catch (...) {	}
												
												throw;
											
											}
											
											end_async();
											
											return;
										
										}
										
										//	Start the receive loop by
										//	enqueuing a receive command
										if (!PostQueuedCompletionStatus(
											iocp,
											0,
											reinterpret_cast<ULONG_PTR>(
												static_cast<Connection *>(
													conn
												)
											),
											reinterpret_cast<LPOVERLAPPED>(&(conn->overlapped))
										)) {
										
											conn->lock.Release();
											
											kill(static_cast<Connection *>(conn));
											
											conn->lock.Acquire();
											bool erase=conn->sends.size()==0;
											conn->lock.Release();
											
											if (erase) try {
											
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
										
										//	Receive is now pending
										conn->pending_recv=true;
										conn->lock.Release();
										
										end_async();
									
									},
									std::move(conn)
								);
							
							} catch (...) {
							
								--running_async;
								
								throw;
							
							}
							
						} catch (...) {
						
							//	Free the socket
							closesocket(socket);
							
							throw;
						
						}
					
					}
				
				}
				
			}
		
		} catch (...) {
		
			try {	panic_callback();	} catch (...) {	}
		
		}
		
		//	Cleanup
		WSACloseEvent(events[1]);
	
	}
	
	
	ConnectionHandler::~ConnectionHandler () noexcept {
	
		//	Tell the thread accepting connections
		//	to stop.
		WSASetEvent(stop);
		
		//	Tell the worker thread to stop
		PostQueuedCompletionStatus(iocp,0,0,nullptr);
		
		//	Wait for threads to end
		worker.Join();
		accept.Join();
		
		//	Close the stop event
		WSACloseEvent(stop);
		
		//	Disconnect all connections that are
		//	still associated with this handler
		//	and attempt to fire their disconnect
		//	callbacks.
		for (auto & pair : connections) {
		
			pair.second->Disconnect();
			
			try {
			
				remove(std::move(pair.second));
			
			} catch (...) {	}
			
		}
		
		//	Kill listening sockets
		for (auto s : bound) closesocket(s);
		
		//	Wait for all async events to end
		async_lock.Acquire();
		while (running_async!=0) async_wait.Sleep(async_lock);
		async_lock.Release();
		
		//	Close the completion port
		CloseHandle(iocp);
		
		//	Cleanup Winsock
		WSACleanup();
	
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
			bound(binds.Count()),
			barrier(2),
			handshake(true)
	{
	
		//	Try to initialize WinSock
		WSADATA temp;
		int result=WSAStartup(
			MAKEWORD(2,2),
			&temp
		);
		
		if (result!=0) throw std::system_error(
			std::error_code(
				result,
				std::system_category()
			)
		);
		
		try {
		
			//	Create a stop event
			if ((stop=WSACreateEvent())==WSA_INVALID_EVENT) throw std::system_error(
				std::error_code(
					WSAGetLastError(),
					std::system_category()
				)
			);
		
			try {

				//	Attempt to bind sockets	
				for (auto & ep : binds) {
				
					//	New TCP socket with appropriate
					//	address type
					SOCKET socket=::socket(
						ep.IP().IsV6() ? AF_INET6 : AF_INET,
						SOCK_STREAM,
						IPPROTO_TCP
					);
					
					if (socket==INVALID_SOCKET) SocketException::Raise(WSAGetLastError());
					
					try {
						
						//	Bind to interface and begin listening
						struct sockaddr_storage addr;
						ep.IP().ToOS(
							&addr,
							ep.Port()
						);
						
						if (!(
							(bind(
								socket,
								reinterpret_cast<struct sockaddr *>(&addr),
								sizeof(struct sockaddr_storage)
							)==0) &&
							(listen(
								socket,
								SOMAXCONN
							)==0)
						)) SocketException::Raise(WSAGetLastError());
						
						bound.Add(socket);
						
					} catch (...) {
					
						closesocket(socket);
						
						throw;
					
					}
				
				}
				
				//	Create an I/O completion port
				if ((iocp=CreateIoCompletionPort(
					INVALID_HANDLE_VALUE,
					nullptr,
					0,
					0
				))==nullptr) throw std::system_error(
					std::error_code(
						GetLastError(),
						std::system_category()
					)
				);
				
				running_async=0;
				
				//	Launch connection that will
				//	accept incoming connections
				accept=Thread([=] () {	accept_func();	});
				
				//	Wait for it to start...
				barrier.Enter();
				
				//	Did it succeed?
				if (!handshake) {
				
					//	NO!
					accept.Join();
					
					throw std::runtime_error(accept_startup_error);
				
				}
				
				//	Launch worker thread
				worker=Thread([=] () {	worker_func();	});
				
			} catch (...) {
			
				//	Make sure whatever sockets we
				//	created get cleaned up
				for (auto s : bound) closesocket(s);
				
				throw;
			
			}
			
		} catch (...) {
		
			WSACleanup();
			
			throw;
		
		}
	
	}
	
	
}

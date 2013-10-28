#include <hardware_concurrency.hpp>
#include <network.hpp>
#include <system_error>
#include <utility>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	template <typename T, typename... Args>
	void ConnectionHandler::enqueue (const T & callback, Args &&... args) {
	
		lock.Acquire();
		++pending;
		lock.Release();
		
		//	Make sure the count gets decremented
		//	if we throw
		try {
		
			//	Dispatch asynchronous callback
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wpedantic"
			pool.Enqueue([this,callback=std::bind(callback,std::forward<Args>(args)...)] () mutable {
			
				try {
				
					callback();
				
				} catch (...) {
				
					lock.Acquire();
					--pending;
					wait.WakeAll();
					lock.Release();
				
					throw;
				
				}
			
			});
			#pragma GCC diagnostic pop
		
		} catch (...) {
		
			lock.Acquire();
			--pending;
			wait.WakeAll();
			lock.Release();
			
			throw;
		
		}
	
	}
	
	
	SmartPointer<Connection> ConnectionHandler::get (Connection * conn) noexcept {
	
		return connections_lock.Read([&] () mutable {	return connections[conn];	});
	
	}
	
	
	void ConnectionHandler::kill (SmartPointer<Connection> & conn) {
	
		if (conn->Kill()) {
		
			//	WE CAN KILL
		
			//	Extract the reason for the disconnect
			//	(if any)
			String reason(conn->Reason());
			
			//	Delete from the collection of managed
			//	connections
			connections_lock.Write([&] () mutable {	connections.erase(static_cast<Connection *>(conn));	});
			
			//	Dispatch disconnect callback
			if (disconnect) enqueue(
				disconnect,
				conn,
				std::move(reason)
			);
		
		}
	
	}
	
	
	void ConnectionHandler::make (SOCKET s, Tuple<IPAddress,UInt16,IPAddress,UInt16> addr) {
	
		++connected;
	
		try {
		
			//	Dispatch callback so connection
			//	can be approved or disapproved
			enqueue([=] () mutable {
			
				bool accepted=false;
				try {
			
					if (accept) accepted=accept(
						addr.Item<0>(),
						addr.Item<1>(),
						addr.Item<2>(),
						addr.Item<3>()
					);
					else accepted=true;
				
				//	Ignore callback exceptions
				} catch (...) {	}
				
				//	If connection was not accepted,
				//	close the connection and move
				//	on
				if (!accepted) {
				
					closesocket(s);
					
					return;
				
				}
				
				//	Connection was accepted
				
				++accepted;
				
				SmartPointer<Connection> conn;
				try {
				
					conn=SmartPointer<Connection>::Make(
						s,
						addr.Item<0>(),
						addr.Item<1>(),
						iocp,
						addr.Item<2>(),
						addr.Item<3>()
					);
				
				} catch (...) {
				
					closesocket(s);
					
					//	We're in an asynchronous callback,
					//	this will not be transmitted unless
					//	we do it explicitly
					do_panic(std::current_exception());
					
					throw;
				
				}
				
				//	Socket is in a connection object now,
				//	we don't need to manage it
				
				//	If the user callback to connect throws,
				//	we don't catch it, we let this connection
				//	die.
				if (connect) connect(conn);
				
				//	We care about exceptions now,
				//	nothing should throw
				try {
				
					//	Add the client to our collection
					//	of clients
					connections_lock.Write([&] () mutable {
					
						connections.emplace(
							static_cast<Connection *>(conn),
							conn
						);
					
					});
					
					//	Pump a receive
					if (!conn->Dispatch()) kill(conn);
				
				} catch (...) {
				
					do_panic(std::current_exception());
					
					throw;
				
				}
			
			});
		
		} catch (...) {
		
			closesocket(s);
			
			throw;
		
		}
	
	}


	void ConnectionHandler::worker_func () {
	
		//	Loop forever, or until told
		//	to stop
		for (;;) {
		
			//	Get a completion port packet
			auto packet=iocp.Get();
			
			//	If the command is null, that's
			//	an order to quit
			if (packet.Command==nullptr) break;
			
			switch (packet.Command->Command) {
			
				//	ACCEPT
				case NetworkCommand::Accept:{
				
					//	Command is actually an AcceptCommand
					auto command=reinterpret_cast<AcceptCommand *>(packet.Command);
					
					//	Conn is actually a ListeningSocket
					auto socket=reinterpret_cast<ListeningSocket *>(packet.Conn);
					
					//	If the accept succeeded, make a new
					//	connection
					if (packet.Result) {
					
						//	Get the local and remote addresses
						//	for this connection
						auto addr=command->GetEndpoints();
						
						//	Create the new connection
						make(
							command->Get(),
							std::move(addr)
						);
					
					}
					
					//	Tell the listening socket to clean
					//	up this command
					socket->Complete(command);
					
					//	Pump a new AcceptCommand
					socket->Dispatch();
				
				}break;
				
				//	SEND
				case NetworkCommand::Send:{
				
					sent+=packet.Num;
				
					//	Command is actually a send handle
					SendHandle * send=reinterpret_cast<SendHandle *>(packet.Command);
					
					//	Conn is actually a Connection
					auto conn=get(reinterpret_cast<Connection *>(packet.Conn));
					
					conn->Send(packet.Num);
					
					if (!send->Complete(packet,pool)) kill(conn);
					
					if (conn->Complete(send)) kill(conn);
				
				}break;
				
				//	RECEIVE
				case NetworkCommand::Receive:{
				
					received+=packet.Num;
				
					//	Command is actually a ReceiveCommand
					auto command=reinterpret_cast<ReceiveCommand *>(packet.Command);
					
					//	Conn is actually a Connection
					auto conn=get(reinterpret_cast<Connection *>(packet.Conn));
					
					conn->Receive(packet.Num);
					
					if (command->Complete(packet)) {
					
						//	Receive success
						
						//	Dispatch asynchronous handler
						enqueue([=] () mutable {
						
							//	Dispatch callback
							try {
							
								if (recv) recv(conn,command->Buffer);
							
							//	Ignore exceptions -- not our problem
							} catch (...) {	}
							
							//	Pump a new receive
							if (!conn->Dispatch()) kill(conn);
						
						});
					
					} else {
					
						//	Receive failed
						
						kill(conn);
					
					}
					
					if (conn->Complete()) kill(conn);
				
				}break;
			
			}
		
		}
	
	}
	
	
	void ConnectionHandler::worker_init () noexcept {
	
		//	Wait for all other workers to startup
		startup.Acquire();
		while (!proceed) startup_wait.Sleep(startup);
		startup.Release();
		
		//	Proceed on success
		if (success) try {
		
			worker_func();
		
		} catch (...) {
		
			do_panic(std::current_exception());
		
			throw;
		
		}
	
	}
	
	
	void ConnectionHandler::do_panic (std::exception_ptr ex) noexcept {
	
		if (panic) try {
		
			panic(ex);
		
		//	We're already panicking, can't
		//	do anything about this
		} catch (...) {	}
	
	}
	
	
	ConnectionHandler::~ConnectionHandler () noexcept {
	
		//	Order all threads to shutdown
		for (Word i=0;i<workers.Count();++i) iocp.Dispatch(nullptr,nullptr);
		
		//	Wait for all threads to shutdown
		for (auto & thread : workers) thread.Join();
		
		//	Wait for all asynchronous callbacks
		//	to complete
		lock.Acquire();
		while (pending!=0) wait.Sleep(lock);
		lock.Release();
		
		//	Kill all connections
		connections.clear();
		
		//	Kill all listening sockets
		listening.Clear();
		
		//	Close the I/O Completion Port
		iocp.Destroy();
		
		//	Clean up winsock
		WSACleanup();
	
	}
	
	
	static Word num_workers_helper (const Nullable<Word> & in) noexcept {
	
		Word retr=in.IsNull() ? HardwareConcurrency() : *in;
		
		if (retr==0) retr=1;
		
		return retr;
	
	}
	
	
	static void winsock_init () {
	
		WSADATA temp;
		int result=WSAStartup(MAKEWORD(2,2),&temp);
		if (result!=0) throw std::system_error(
			std::error_code(
				result,
				std::system_category()
			)
		);
	
	}
	
	
	ConnectionHandler::ConnectionHandler (
		const Vector<Tuple<IPAddress,UInt16>> & binds,
		AcceptCallback accept,
		ConnectCallback connect,
		DisconnectCallback disconnect,
		ReceiveCallback recv,
		PanicCallback panic,
		ThreadPool & pool,
		Nullable<Word> num_workers
	)	:	pool(pool),
			recv(std::move(recv)),
			disconnect(std::move(disconnect)),
			accept(std::move(accept)),
			connect(std::move(connect)),
			panic(std::move(panic)),
			pending(0),
			listening(binds.Count()),
			workers(num_workers_helper(num_workers)),
			proceed(false),
			success(true)
	{
	
		//	Initialize winsock
		winsock_init();
	
		//	Initialize statistics
		sent=0;
		received=0;
		connected=0;
		accepted=0;
		
		Word num=num_workers_helper(num_workers);
		
		//	Create listening sockets
		for (const auto & ep : binds) {
		
			listening.EmplaceBack(
				ep.Item<0>(),
				ep.Item<1>(),
				iocp
			);
			
			//	Each listener gets an asynchronous
			//	accept for each worker
			for (Word i=0;i<num;++i) listening[listening.Count()-1].Dispatch();
		
		}
		
		//	Create workers
		try {
		
			for (Word i=0;i<num;++i) workers.EmplaceBack(
				[this] () mutable {	worker_init();	}
			);
		
		} catch (...) {
		
			//	Notify workers that startup failed,
			//	and that they should exit at once
			startup.Acquire();
			success=false;
			proceed=true;
			startup_wait.WakeAll();
			startup.Release();
		
			for (auto & t : workers) t.Join();
			
			throw;
		
		}
		
		//	Notify workers that startup succeeded,
		//	and that they should proceed at
		//	once
		startup.Acquire();
		proceed=true;
		startup_wait.WakeAll();
		startup.Release();
	
	}


}

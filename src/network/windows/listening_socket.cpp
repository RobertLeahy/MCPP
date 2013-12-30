#include <network.hpp>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	void ListeningSocket::Complete (Packet packet) {
	
		auto command=reinterpret_cast<AcceptCommand *>(packet.Command);
		
		//	Check for failure
		if (!packet.Result) {
		
			//	Failure can mean one of two things:
			//
			//	1.	This socket is shutting down and
			//		all pending accepts are failing,
			//		which is fine, we just pass.
			//	2.	Something has gone wrong, PANIC
			
			//	Everything is okay, destroy the
			//	accept command and pass
			if (lock.Execute([&] () {
			
				//	FAIL
				if (socket!=INVALID_SOCKET) return false;
				
				//	Socket has been closed, this failure
				//	should have occurred
				
				//	Erase accept command
				pending.erase(packet.Command);
				
				//	Wake waiting threads
				wait.WakeAll();
				
				return true;
			
			})) return;
			
			//	PANIC
			Raise(packet.Error);
		
		}
		
		++handler.Incoming;
		
		//	For some reason std::function copies
		//	rather than moving the target functor
		//
		//	Work around this
		auto data=command->Get();
		auto s=data.Get();
		
		//	We're responsible for the socket's lifetime
		try {
		
			//	Dispatch callback
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wpedantic"
			handler.Enqueue([
				//	Accept callback
				accept=ep.Accept,
				//	Connect callback
				connect=ep.Connect,
				//	Disconnect callback
				disconnect=ep.Disconnect,
				//	Receive callback
				receive=ep.Receive,
				//	Get data about this incoming
				//	connection
				data,
				//	We need a reference to the handler
				&handler=handler,
				//	The socket
				s
			] () mutable {
			
				try {
				
					//	Is the collection accepted?
					bool accepted=false;
					try {
					
						if (accept) accepted=accept(AcceptEvent{
							data.RemoteIP,
							data.RemotePort,
							data.LocalIP,
							data.LocalPort
						});
						else accepted=true;
					
					//	We interpret exceptions as meaning
					//	that the connection should be declined
					} catch (...) {	}
					
					//	If the connection is not accepted, end
					if (!accepted) {
					
						closesocket(s);
					
						return;
						
					}
					
					//	The connection is accepted, increment
					//	statistic within handler
					++handler.Accepted;
					
					SmartPointer<Connection> conn;
					try {
					
						//	Create the new connection object
						conn=SmartPointer<Connection>::Make(
							s,
							data.RemoteIP,
							data.RemotePort,
							data.LocalIP,
							data.LocalPort,
							handler,
							std::move(receive),
							std::move(disconnect)
						);
					
					} catch (...) {
					
						//	Remember the socket
						closesocket(s);
						
						throw;
					
					}
					
					//	Socket is inside a Connection object,
					//	which is responsible for its lifetime,
					//	so we don't have to manually manage it
					//	anymore
					
					//	Add connection to handler
					handler.Add(conn);
					
					//	Attach to completion port
					conn->Attach();
					
					//	Fire connect handler if appropriate
					if (connect) try {
					
						connect(ConnectEvent{conn});
						
					//	These exceptions are not our problem
					} catch (...) {	}
					
					//	Begin the receive loop
					conn->Begin();
				
				//	If uncaught exceptions are thrown,
				//	we must notify the handler by
				//	panicking
				} catch (...) {
				
					handler.Panic(std::current_exception());
					
					throw;
				
				}
			
			});
			#pragma GCC diagnostic pop
			
		} catch (...) {
		
			closesocket(s);
			
			throw;
		
		}
		
		//	Dispatch another accept command
		//	if we're able, otherwise we shut
		//	down
		lock.Execute([&] () mutable {
		
			if (socket==INVALID_SOCKET) {
			
				//	Shutting down
				
				pending.erase(packet.Command);
				
				wait.WakeAll();
				
				//	Last out, remove ourselves from
				//	the handler
				if (pending.size()==0) handler.Remove(this);
			
			} else {
			
				//	Dispatch again
				
				command->Dispatch(socket);
			
			}
		
		});
	
	}
	
	
	void ListeningSocket::Attach () {
	
		handler.Port.Attach(socket,this);
	
	}
	
	
	void ListeningSocket::Dispatch () {
	
		//	Create an accept command
		std::unique_ptr<AcceptCommand> command(new AcceptCommand(ep.IP));
		auto ptr=command.get();
	
		lock.Execute([&] () mutable {
			
			//	Insert accept command
			auto pair=pending.emplace(
				ptr,
				std::move(command)
			);
			
			//	Rollback on fail
			try {
			
				//	Dispatch
				ptr->Dispatch(socket);
			
			} catch (...) {
			
				pending.erase(pair.first);
				
				throw;
			
			}
		
		});
	
	}


	ListeningSocket::ListeningSocket (LocalEndpoint ep, ConnectionHandler & handler) : handler(handler), ep(std::move(ep)) {
	
		//	Create a new socket
		socket=MakeSocket(this->ep.IP.IsV6());
		
		//	Make sure socket is cleaned up
		//	if binding/listening/attaching
		//	fails
		try {
		
			//	Bind and start listening
			struct sockaddr_storage addr;
			this->ep.IP.ToOS(&addr,this->ep.Port);
			if (
				(bind(
					socket,
					reinterpret_cast<struct sockaddr *>(&addr),
					sizeof(addr)
				)==SOCKET_ERROR) ||
				(listen(
					socket,
					SOMAXCONN
				)==SOCKET_ERROR)
			) RaiseWSA();
			
		} catch (...) {
		
			closesocket(socket);
			
			throw;
		
		}
	
	}
	
	
	ListeningSocket::~ListeningSocket () noexcept {
	
		//	Close the socket if it's
		//	not already closed
		if (socket!=INVALID_SOCKET) closesocket(socket);
	
	}
	
	
	void ListeningSocket::Shutdown () noexcept {
	
		lock.Execute([&] () {
		
			//	Close the socket unless
			//	it's already closed
			if (socket!=INVALID_SOCKET) {
			
				//	This causes all pending asynchronous
				//	accepts to fail
				closesocket(socket);
				socket=INVALID_SOCKET;
			
			}
			
			//	Wait for all accepts to complete
			while (pending.size()!=0) wait.Sleep(lock);
		
		});
	
	}


}

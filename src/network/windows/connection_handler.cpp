#include <hardware_concurrency.hpp>
#include <network.hpp>
#include <cstdlib>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	void ConnectionHandler::Remove (const Connection * ptr) noexcept {
	
		connections_lock.Execute([&] () mutable {	connections.erase(const_cast<Connection *>(ptr));	});
	
	}
	
	
	void ConnectionHandler::Remove (const ListeningSocket * ptr) noexcept {
	
		listening_lock.Execute([&] () mutable {	listening.erase(const_cast<ListeningSocket *>(ptr));	});
	
	}
	
	
	SmartPointer<Connection> ConnectionHandler::Get (const Connection * ptr) noexcept {
	
		return connections_lock.Execute([&] () mutable {
		
			auto iter=connections.find(const_cast<Connection *>(ptr));
			
			if (iter==connections.end()) return SmartPointer<Connection>();
			
			return iter->second;
		
		});
	
	}
	
	
	SmartPointer<ListeningSocket> ConnectionHandler::Get (const ListeningSocket * ptr) noexcept {
	
		return listening_lock.Execute([&] () mutable {
		
			auto iter=listening.find(const_cast<ListeningSocket *>(ptr));
			
			if (iter==listening.end()) return SmartPointer<ListeningSocket>();
			
			return iter->second;
		
		});
	
	}
	
	
	Connection * ConnectionHandler::Add (SmartPointer<Connection> add) {
	
		auto ptr=static_cast<Connection *>(add);
	
		connections_lock.Execute([&] () mutable {
		
			connections.emplace(
				ptr,
				std::move(add)
			);
		
		});
		
		return ptr;
	
	}
	
	
	ListeningSocket * ConnectionHandler::Add (SmartPointer<ListeningSocket> add) {
	
		auto ptr=static_cast<ListeningSocket *>(add);
		
		listening_lock.Execute([&] () mutable {
		
			listening.emplace(
				ptr,
				std::move(add)
			);
		
		});
		
		return ptr;
	
	}
	
	
	void ConnectionHandler::Panic (std::exception_ptr ex) noexcept {
	
		if (panic) try {
			
			panic(ex);
			
			return;
			
		} catch (...) {	}
		
		//	Fallback to aborting
		std::abort();
	
	}
	
	
	void ConnectionHandler::worker () {
	
		//	Wait for instructions from the constructor
		lock.Execute([&] () mutable {	while (startup==StartupResult::None) wait.Sleep(lock);	});
		
		//	What was the result of startup?
		//	If failure, end at once
		if (startup==StartupResult::Failed) return;
		
		//	Catch all errors and turn them into
		//	panics
		try {
		
			//	Loop until told to stop
			for (;;) {
			
				//	Get a completion packet
				auto packet=Port.Get();
				
				//	Is it a shutdown command?
				//	If so, end at once
				if (packet.Command==nullptr) return;
				
				//	Switch on the type of command we're
				//	handling
				switch (packet.Command->Type) {
				
					//	Process accept commands against a listening
					//	socket
					case CommandType::Accept:
						reinterpret_cast<ListeningSocket *>(packet.Data)->Complete(std::move(packet));
						break;
					
					//	Process all other commands against a connection
					default:
						reinterpret_cast<Connection *>(packet.Data)->Complete(std::move(packet));
						break;
				
				}
			
			}
		
		} catch (...) {
		
			Panic(std::current_exception());
			
			throw;
		
		}
	
	}
	
	
	static Word num_workers_helper (Nullable<Word> num_workers) noexcept {
	
		//	If the provided nullable integer is
		//	null, use the number of cores in
		//	the system, otherwise use the provided
		//	integer
		Word retr=num_workers.IsNull() ? HardwareConcurrency() : *num_workers;
		
		//	If the result we obtained is zero, bump
		//	it up to one
		if (retr==0) retr=1;
		
		return retr;
	
	}
	
	
	ConnectionHandler::ConnectionHandler (ThreadPool & pool, Nullable<Word> num_workers, PanicType panic)
		:	workers(num_workers_helper(num_workers)),
			Pool(pool),
			startup(StartupResult::None),
			panic(std::move(panic))
	{
	
		//	Start workers
		Word n=num_workers_helper(num_workers);
		try {

			for (Word i=0;i<n;++i) {
			
				//	Do this first in case it throws
				workers.EmplaceBack();
				
				//	Start worker
				workers[workers.Count()-1]=Thread([this] () mutable {	worker();	});
			
			}
			
		} catch (...) {
		
			//	End workers that have already
			//	been started
			
			lock.Execute([&] () mutable {	startup=StartupResult::Failed;	});
			
			for (auto & t : workers) t.Join();
		
		}
		
		//	Initialize statistics
		Sent=0;
		Received=0;
		Incoming=0;
		Outgoing=0;
		Accepted=0;
		Disconnected=0;
		
		//	Instruct workers to begin
		lock.Execute([&] () mutable {
		
			startup=StartupResult::Succeeded;
			wait.WakeAll();
		
		});
	
	}
	
	
	ConnectionHandler::~ConnectionHandler () noexcept {
	
		//	Command all workers to
		//	shut down
		for (Word i=0;i<workers.Count();++i) Port.Post();
		
		//	Wait for all workers to shut
		//	down
		for (auto & t : workers) t.Join();
		
		//	Wait for all asynchronous callbacks
		//	which may depend on this object or
		//	its connections, or create/insert
		//	new connections to finish
		Manager.Wait();
	
	}
	
	
	void ConnectionHandler::Connect (RemoteEndpoint ep) {
	
		//	IPv6 or IPv4?
		bool is_v6=ep.IP.IsV6();
	
		//	Create a socket
		auto socket=MakeSocket(is_v6);
		
		//	Until the socket is safely in
		//	a connection object, we're responsible
		//	for its lifetime
		SmartPointer<Connection> conn;
		try {
		
			//	Create a connection object for
			//	this connection
			conn=SmartPointer<Connection>::Make(
				socket,
				ep.IP,
				ep.Port,
				//	Bogus IP/port
				IPAddress::Any(is_v6),
				0,
				*this,
				std::move(ep.Receive),
				std::move(ep.Disconnect),
				std::move(ep.Connect)
			);
		
		} catch (...) {
		
			closesocket(socket);
			
			throw;
		
		}
		
		//	Socket is now safe
		
		//	Add the connection to the handler
		auto ptr=Add(std::move(conn));
		
		//	We'll need to rollback by removing
		//	the connection if this fails
		try {
		
			//	Attach to the completion port
			ptr->Attach();
		
			//	Initiate asynchronous connection
			ptr->Connect();
		
		} catch (...) {
		
			Remove(ptr);
			
			throw;
		
		}
	
	}
	
	
	SmartPointer<ListeningSocket> ConnectionHandler::Listen (LocalEndpoint ep) {
	
		//	Create a handle to a new listening
		//	socket
		auto handle=SmartPointer<ListeningSocket>::Make(
			std::move(ep),
			*this
		);
		
		//	Add listening socket
		auto ptr=Add(handle);
		
		//	If an exception is thrown, we
		//	must rollback
		try {
		
			//	Attach to completion port
			handle->Attach();
		
			//	Dispatch asynchronous accepts
			for (Word i=0;i<workers.Count();++i) handle->Dispatch();
		
		} catch (...) {
		
			//	Shutdown
			handle->Shutdown();
			
			//	Remove
			Remove(ptr);
			
			throw;
		
		}
		
		return handle;
	
	}
	
	
	ConnectionHandlerInfo ConnectionHandler::GetInfo () const noexcept {
	
		Word listening=listening_lock.Execute([&] () {	return this->listening.size();	});
		Word connected=connections_lock.Execute([&] () {	return connections.size();	});
	
		return ConnectionHandlerInfo{
			Sent,
			Received,
			Outgoing,
			Incoming,
			Accepted,
			Disconnected,
			listening,
			connected,
			workers.Count()
		};
	
	}


}

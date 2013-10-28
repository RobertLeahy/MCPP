#include <network.hpp>
#include <utility>


using namespace MCPP::NetworkImpl;


namespace MCPP {
	
	
	Connection::Connection (
		SOCKET socket,
		IPAddress remote_ip,
		UInt16 remote_port,
		IOCP & iocp,
		IPAddress local_ip,
		UInt16 local_port
	)	:	socket(socket),
			is_shutdown(false),
			pending(0),
			remote_ip(remote_ip),
			remote_port(remote_port),
			local_ip(local_ip),
			local_port(local_port)
	{
	
		iocp.Attach(socket,this);
		
		sent=0;
		received=0;
	
	}
	
	
	Connection::~Connection () noexcept {
	
		closesocket(socket);
	
	}
	
	
	void Connection::disconnect () noexcept {
	
		//	Shutdown the socket but only if
		//	the socket has not already been
		//	shutdown
		sends_lock.Execute([&] () mutable {
		
			if (is_shutdown) return;
			
			is_shutdown=true;
			
			for (auto & pair : sends) pair.second->Fail();
		
		});
	
	}
	
	
	bool Connection::Complete (SendHandle * ptr) {
		
		sends_lock.Execute([&] () mutable {	sends.erase(ptr);	});
		
		return Complete();
	
	}
	
	
	bool Connection::Complete () noexcept {
	
		//	Decrement pending async count
		//	and return true if the socket
		//	should die
		return sends_lock.Execute([&] () mutable {
		
			return (
				((--pending)==0) &&
				is_shutdown
			);
			
		});
	
	}
	
	
	bool Connection::Kill () noexcept {
	
		disconnect();
		
		return sends_lock.Execute([&] () mutable {	return pending==0;	});
	
	}
	
	
	bool Connection::Dispatch () {
	
		return sends_lock.Execute([&] () mutable {
	
			auto retr=recv.Dispatch(socket);
			
			if (retr) ++pending;
			
			return retr;
			
		});
	
	}
	
	
	SmartPointer<SendHandle> Connection::Send (Vector<Byte> buffer) {
	
		//	Create a send handle for this send
		auto handle=SmartPointer<SendHandle>::Make(std::move(buffer));
		
		//	Lock and proceed
		sends_lock.Execute([&] () mutable {
		
			//	If already shutdown, fail at
			//	once
			if (is_shutdown) {
			
				handle->Fail();
				
				return;
				
			}
			
			//	Add to collection of handles
			auto pair=sends.emplace(
				static_cast<SendHandle *>(handle),
				handle
			);
			
			//	If an exception is thrown, we
			//	must remove from the collection
			try {
			
				//	SEND
				handle->Dispatch(socket);
				
				++pending;
			
			} catch (...) {
			
				sends.erase(pair.first);
				
				throw;
			
			}
		
		});
		
		return handle;
	
	}
	
	
	void Connection::Send (Word num) noexcept {
	
		sent+=num;
	
	}
	
	
	void Connection::Receive (Word num) noexcept {
	
		received+=num;
	
	}
	
	
	Word Connection::Pending () const noexcept {
	
		return sends_lock.Execute([&] () {	return sends.size();	});
	
	}


}

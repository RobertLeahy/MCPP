#include <network.hpp>
#include <utility>
#include <stdexcept>
#include <system_error>


#ifdef ENVIRONMENT_WINDOWS
#include <cstring>
#include <Ws2tcpip.h>
#else
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#endif


namespace MCPP {


	//	Default size of a receive buffer
	static const Word recv_buf_size=1024;


	//
	//	ENDPOINT
	//
	
	
	Endpoint::Endpoint (IPAddress ip, UInt16 port) noexcept : ip(ip), port(port) {	}
	
	
	IPAddress Endpoint::IP () const noexcept {
	
		return ip;
	
	}
	
	
	UInt16 Endpoint::Port () const noexcept {
	
		return port;
	
	}


	//
	//	SEND HANDLE SHARED
	//


	SendState SendHandle::State () const noexcept {
	
		lock.Acquire();
		SendState curr=state;
		lock.Release();
		
		return curr;
	
	}
	
	
	SendState SendHandle::Wait () const noexcept {
	
		lock.Acquire();
		while (!((state==SendState::Sent) || (state==SendState::Failed))) wait.Sleep(lock);
		SendState curr=state;
		lock.Release();
		
		return curr;
	
	}
	
	
	void SendHandle::AddCallback (SendCallback callback) {
	
		if (!callback) return;
	
		bool invoke=false;
		SendState curr;
		lock.Acquire();
		
		try {
		
			if ((state==SendState::Sent) || (state==SendState::Failed)) {
			
				curr=state;
				invoke=true;
			
			} else {
			
				callbacks.Add(std::move(callback));
			
			}
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
		
		if (invoke) callback(curr);
	
	}
	
	
	//
	//	CONNECTION SHARED
	//
	
	
	void Connection::Disconnect () noexcept {
	
		disconnect();
	
	}
	
	
	void Connection::Disconnect (const String & reason) noexcept {
	
		bool first=disconnect();
		
		if (first) try {
		
			this->reason=reason;
		
		} catch (...) {	}
	
	}
	
	
	void Connection::Disconnect (String && reason) noexcept {
	
		bool first=disconnect();
		
		if (first) this->reason=std::move(reason);
	
	}
	
	
	IPAddress Connection::IP () const noexcept {
	
		return endpoint.IP();
	
	}
	
	
	UInt16 Connection::Port () const noexcept {
	
		return endpoint.Port();
	
	}
	
	
	UInt64 Connection::Sent () const noexcept {
	
		return sent;
	
	}
	
	
	UInt64 Connection::Received () const noexcept {
	
		return received;
	
	}
	
	
	//
	//	CONNECTION HANDLER PORTABLE
	//
	
	
	inline void ConnectionHandler::disconnect (SmartPointer<Connection> conn) {
	
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
	
	
	inline void ConnectionHandler::kill (Connection * conn) noexcept {
	
		conn->Disconnect();
	
	}
	
	
	inline void ConnectionHandler::end_async () noexcept {
	
		async_lock.Acquire();
		--running_async;
		async_wait.WakeAll();
		async_lock.Release();
	
	}
	
	
	//
	//	MISC PORTABLE
	//
	
	
	static inline Endpoint get_endpoint (struct sockaddr_storage * addr) noexcept {
	
		return Endpoint(
			IPAddress(addr),
			(addr->ss_family==AF_INET6)
				?	reinterpret_cast<struct sockaddr_in6 *>(addr)->sin6_port
				:	reinterpret_cast<struct sockaddr_in *>(addr)->sin_port
		);
	
	}


}


#ifdef ENVIRONMENT_WINDOWS
#include "network_windows.cpp"
#else
#include "network_linux.cpp"
#endif

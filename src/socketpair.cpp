#include <socketpair.hpp>
#include <cstring>
#include <stdexcept>
#include <system_error>
#ifndef ENVIRONMENT_WINDOWS
#include <sys/socket.h>
#endif


namespace MCPP {


	#ifdef ENVIRONMENT_WINDOWS
	
	[[noreturn]]
	static void raise (int code) {
	
		throw std::system_error(
			std::error_code(
				code,
				std::system_category()
			)
		);
	
	}
	
	
	[[noreturn]]
	static void raise () {
	
		raise(WSAGetLastError());
	
	}
	
	
	static SOCKET socket () {
	
		auto retr=::socket(
			AF_INET,
			SOCK_STREAM,
			IPPROTO_TCP
		);
		if (retr==INVALID_SOCKET) raise();
		
		return retr;
	
	}
	
	
	SocketPair::SocketPair () {
	
		//	Ignored, for call to WSAStartup
		//	only (segfaults if not present)
		WSADATA dummy;
		
		//	Startup winsock
		auto result=WSAStartup(
			MAKEWORD(2,2),
			&dummy
		);
		if (result!=0) raise(result);
		
		try {
		
			//	Setup listening end
			
			auto listener=socket();
			
			try {
			
				//	Disable reuse address
				int reuse=0;
				if (setsockopt(
					listener,
					SOL_SOCKET,
					SO_REUSEADDR,
					reinterpret_cast<char *>(&reuse),
					sizeof(reuse)
				)==-1) raise();
				
				//	Bind to loopback on a random port and
				//	begin listening
				
				//	Address structures that we'll use
				union {
					struct sockaddr_in in_addr;
					struct sockaddr addr;
				};
				
				std::memset(
					&in_addr,
					0,
					sizeof(in_addr)
				);
				in_addr.sin_family=AF_INET;
				in_addr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
				
				if (
					(bind(
						listener,
						&addr,
						sizeof(in_addr)
					)==SOCKET_ERROR) ||
					(listen(
						listener,
						1
					)==SOCKET_ERROR)
				) raise();
				
				//	Retrieve port that was actually bound to
				int in_addr_size=sizeof(in_addr);
				if (getsockname(
					listener,
					&addr,
					&in_addr_size
				)==SOCKET_ERROR) raise();
				
				//	Now that the listener is listening, we
				//	create a socket to connect on
				pair[0]=socket();
				
				try {
				
					if (
						(connect(
							pair[0],
							&addr,
							sizeof(in_addr)
						)==SOCKET_ERROR) ||
						((pair[1]=accept(
							listener,
							nullptr,
							nullptr
						))==INVALID_SOCKET)
					) raise();
				
				} catch (...) {
				
					closesocket(pair[0]);
					
					throw;
				
				}
			
			} catch (...) {
			
				closesocket(listener);
				
				throw;
			
			}
			
			closesocket(listener);
		
		} catch (...) {
		
			WSACleanup();
			
			throw;
		
		}
	
	}
	
	
	SocketPair::~SocketPair () noexcept {
	
		//	Close both sockets
		for (auto & s : pair) closesocket(s);
	
		//	Cleanup winsock
		WSACleanup();
	
	}
	
	#else
	
	SocketPair::SocketPair () {
	
		if (socketpair(
			AF_LOCAL,
			SOCK_STREAM,
			0,
			pair
		)==-1) throw std::system_error(
			std::error_code(
				errno,
				std::system_category()
			)
		);
	
	}
	
	
	SocketPair::~SocketPair () noexcept {
	
		//	Close both sockets
		for (auto & s : pair) close(s);
	
	}
	
	#endif
	
	
	SocketPair::Type & SocketPair::operator [] (std::size_t i) noexcept {
	
		return pair[i];
	
	}
	
	
	const SocketPair::Type & SocketPair::operator [] (std::size_t i) const noexcept {
	
		return pair[i];
	
	}
	
	
	constexpr Word master=1;
	constexpr Word slave=0;
	
	
	#ifndef ENVIRONMENT_WINDOWS
	
	[[noreturn]]
	static void raise () {
	
		throw std::system_error(
			std::error_code(
				errno,
				std::system_category()
			)
		);
	
	}
	
	#endif
	
	
	void ControlSocket::send (Byte b) {
	
		//	Send this one byte over the master
		//	socket
		for (;;) switch (::send(pair[master],reinterpret_cast<char *>(&b),1,0)) {
		
			case 0:
				//	Nothing was sent, try again
				break;
				
			case 1:
				//	We're done
				return;
				
			default:
				//	ERROR
				raise();
		
		}
	
	}
	
	
	static bool would_block () noexcept {
	
		return
		#ifdef ENVIRONMENT_WINDOWS
		WSAGetLastError()==WSAEWOULDBLOCK
		#else
		(errno==EWOULDBLOCK)
		#if EWOULDBLOCK!=EAGAIN
		|| (errno==EAGAIN)
		#endif
		#endif
		;
	
	}
	
	
	static const char * end_of_stream="Unexpected end of stream";
	
	
	Nullable<Byte> ControlSocket::recv () {
	
		Byte b;
		//	Try and receive a byte over the slave
		//	socket (which is blocking)
		switch (::recv(pair[slave],reinterpret_cast<char *>(&b),1,0)) {
		
			case 0:
				//	UNEXPECTED END OF STREAM!!
				throw std::runtime_error(end_of_stream);
				
			case 1:
				//	Done
				return b;
				
			default:
				//	Error
				//
				//	If it's just an error because
				//	the operation would block, that's fine
				if (would_block()) return Nullable<Byte>{};
				//	Otherwise throw
				raise();
		
		}
	
	}
	
	
	#ifdef ENVIRONMENT_WINDOWS	
	
	static void set_blocking (SOCKET s, bool blocking) {
	
		u_long value=blocking ? 0 : 1;
		if (ioctlsocket(
			s,
			FIONBIO,
			&value
		)==SOCKET_ERROR) raise();
	
	}
	
	#else
	
	static void set_blocking (int s, bool blocking) {
	
		//	Get flags
		auto flags=fcntl(s,F_GETFL,0);
		if (flags==-1) raise();
		
		//	Set O_NONBLOCK flag as applicable
		if (blocking) flags&=~static_cast<decltype(flags)>(O_NONBLOCK);
		else flags|=O_NONBLOCK;
		
		//	Set flags back to file descriptor
		if (fcntl(s,F_SETFL,flags)==-1) raise();
	
	}
	
	#endif
	
	
	ControlSocket::ControlSocket () {
	
		//	Set the master end to blocking,
		//	the slave end to non-blocking
		set_blocking(pair[slave],false);
		set_blocking(pair[master],true);
	
	}
	
	
	int ControlSocket::Add (
		fd_set & set,
		int
		#ifndef ENVIRONMENT_WINDOWS
		nfds
		#endif
	) const noexcept {
	
		//	Add the slave socket to the set
		FD_SET(pair[slave],&set);
		
		//	On Windows nfds is ignored, so we
		//	unconditionally return zero.
		//
		//	On POSIX systems nfds is regarded,
		//	so we have to determine whether to
		//	return the original value or the
		//	file descriptor of the slave socket
		return
		#ifdef ENVIRONMENT_WINDOWS
		0
		#else
		(nfds>pair[slave]) ? nfds : (pair[slave]+1)
		#endif
		;
	
	}
	
	
	bool ControlSocket::Is (const fd_set & set) const noexcept {
	
		return FD_ISSET(pair[slave],&set);
	
	}
	
	
	void ControlSocket::Clear (fd_set & set) const noexcept {
	
		FD_CLR(pair[slave],&set);
	
	}


}

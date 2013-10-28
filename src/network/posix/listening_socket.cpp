#include <network.hpp>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <system_error>


namespace MCPP {


	namespace NetworkImpl {
	
	
		ListeningSocket::ListeningSocket (IPAddress ip, UInt16 port) : ip(ip), port(port) {
		
			//	Create a sockaddr_storage
			//	structure to tell the operating
			//	system what endpoint to bind
			//	this socket to
			struct sockaddr_storage addr;
			ip.ToOS(&addr,port);
		
			//	Create server socket
			socket=::socket(
				ip.IsV6() ? AF_INET6 : AF_INET,
				SOCK_STREAM,
				IPPROTO_TCP
			);
			
			//	We're now responsible for this
			//	socket
			try {
			
				//	Put the socket in non-blocking
				//	mode
				SetNonBlocking(socket);
				
				if (
					//	Bind to the endpoint
					(bind(
						socket,
						reinterpret_cast<struct sockaddr *>(&addr),
						sizeof(addr)
					)==-1) ||
					//	Begin listening
					(listen(
						socket,
						SOMAXCONN
					)==-1)
				) Raise();
			
			} catch (...) {
			
				close(socket);
				
				throw;
			
			}
		
		}
		
		
		void ListeningSocket::destroy () noexcept {
		
			if (socket!=-1) close(socket);
		
		}
		
		
		ListeningSocket::~ListeningSocket () noexcept {
		
			destroy();
		
		}
		
		
		ListeningSocket::ListeningSocket (ListeningSocket && other) noexcept : socket(other.socket) {
		
			other.socket=-1;
		
		}
		
		
		ListeningSocket & ListeningSocket::operator = (ListeningSocket && other) noexcept {
		
			//	Guard against self-assignment
			if (this!=&other) {
			
				destroy();
				
				socket=other.socket;
				other.socket=-1;
			
			}
			
			return *this;
		
		}
		
		
		static UInt16 get_port (struct sockaddr_storage * addr) noexcept {
		
			return (
				(addr->ss_family==AF_INET6)
					?	reinterpret_cast<struct sockaddr_in6 *>(addr)->sin6_port
					:	reinterpret_cast<struct sockaddr_in *>(addr)->sin_port
			);
		
		}
		
		
		Nullable<ListeningSocket::ConnectionType> ListeningSocket::Accept () {
		
			//	For the remote address
			struct sockaddr_storage addr;
			socklen_t addr_len=sizeof(addr);
			
			//	To be returned
			Nullable<ConnectionType> retr;
			
			//	Loop until we get a connection
			//	or the socket reports that it
			//	would block
			int fd;
			for (;;) {
			
				//	Attempt to accept
				fd=accept(
					socket,
					reinterpret_cast<struct sockaddr *>(&addr),
					&addr_len
				);
				
				//	Detect/handle errors
				if (fd==-1) {
				
					//	If the call failed simply
					//	because it would have blocked,
					//	just return
					if (WouldBlock()) return retr;
					
					//	If we're getting protocol errors
					//	from the connection passed through,
					//	filter those out and try again
					switch (errno) {
					
						case ENETDOWN:
						case EPROTO:
						case ENOPROTOOPT:
						case EHOSTDOWN:
						case ENONET:
						case EHOSTUNREACH:
						case EOPNOTSUPP:
						case ENETUNREACH:
							break;
						//	All other errors are critical
						//	and cause us to throw
						default:
							Raise();
					
					}
					
					//	At any rate, we didn't get a
					//	connection, try again
					continue;
				
				}
				
				//	Connection was accepted successfully
				
				//	Attempt to set new socket to non-blocking
				try {
				
					SetNonBlocking(fd);
				
				} catch (...) {
				
					//	Something went wrong, close the
					//	new connection and try again
					close(fd);
					
					continue;
				
				}
				
				break;
			
			}
			
			//	Create information about this new
			//	connection
			retr.Construct(
				fd,
				&addr,
				get_port(&addr),
				ip,
				port
			);
			
			return retr;
		
		}
		
		
		void ListeningSocket::Attach (Notifier & notifier) {
		
			notifier.Attach(socket);
			notifier.Update(socket,true,false);
		
		}
		
		
		int ListeningSocket::Get () const noexcept {
		
			return socket;
		
		}
	
	
	}


}

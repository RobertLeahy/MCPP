#include <network.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>


namespace MCPP {


	namespace NetworkImpl {
	
	
		ControlSocket::ControlSocket () {
			
			//	Create a pair of connected
			//	sockets for inter thread
			//	communication
			if (socketpair(
				AF_LOCAL,
				SOCK_STREAM,
				0,
				sockets
			)==-1) Raise();
			
			try {
			
				//	Make the master end blocking
				SetBlocking(sockets[0]);
				//	Make the slave end non-blocking
				SetNonBlocking(sockets[1]);
				
			} catch (...) {
			
				//	Make sure sockets get cleaned up
				//	before throw
				close(sockets[0]);
				close(sockets[1]);
			
				throw;
			
			}
		
		}
		
		
		ControlSocket::~ControlSocket () noexcept {
		
			close(sockets[0]);
			close(sockets[1]);
		
		}
		
		
		void ControlSocket::Get () {
		
			//	Loop until everything's been
			//	flushed out
			for (;;) {
			
				Byte b;
				auto result=recv(
					sockets[1],
					&b,
					1,
					0
				);
				
				//	If nothing was received, we're
				//	done
				if (result==0) return;
				
				//	Handle failure
				if (result<0) {
				
					//	If we failed simply because we'd
					//	block, we're done
					if (WouldBlock()) return;
					
					//	Otherwise throw
					Raise();
				
				}
				
			}
		
		}
		
		
		void ControlSocket::Put () {
		
			lock.Execute([&] () mutable {
			
				//	We just send one byte, just
				//	to wake the worker up
				Byte b=0;
				
				for (;;) {
				
					auto result=send(
						sockets[0],
						&b,
						1,
						0
					);
					
					if (result==1) return;
					
					if (result<0) {
					
						//	Being interrupt is NOT
						//	an error, try again
						if (errno==EINTR) continue;
						
						//	Throw
						Raise();
					
					}
				
				}
				
			});
		
		}
		
		
		int ControlSocket::Wait () const noexcept {
		
			return sockets[1];
		
		}
	
	
	}


}

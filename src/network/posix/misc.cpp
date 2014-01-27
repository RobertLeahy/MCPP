#include <network.hpp>
#include <algorithm>
#include <iterator>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


namespace MCPP {


	namespace NetworkImpl {
	
	
		void SetBlocking (FDType fd, bool blocking) {
		
			//	Retrieve the flags for this file
			//	descriptor
			auto flags=fcntl(fd,F_GETFL);
			
			//	Set the non-blocking flag appropriate
			if (blocking) flags&=~static_cast<decltype(flags)>(O_NONBLOCK);
			else flags|=O_NONBLOCK;
			
			//	Set the flags back to the file
			//	descriptor
			if (fcntl(fd,F_SETFL,flags)==-1) Raise();
		
		}
		
		
		FDType GetSocket (bool is_v6) {
		
			//	Make a socket for the appropriate
			//	address family
			auto socket=::socket(
				is_v6 ? AF_INET6 : AF_INET,
				SOCK_STREAM,
				IPPROTO_TCP
			);
			
			//	Make sure it's non-blocking
			SetBlocking(socket,false);
			
			return socket;
		
		}
		
		
		static UInt16 get_port (const struct sockaddr_storage * addr) noexcept {
		
			union {
				UInt16 retr;
				Byte buffer [sizeof(UInt16)];
			};
			retr=static_cast<UInt16>(
				(addr->ss_family==AF_INET6)
					?	reinterpret_cast<const sockaddr_in6 *>(addr)->sin6_port
					:	reinterpret_cast<const sockaddr_in *>(addr)->sin_port
			);
			
			if (!Endianness::IsBigEndian<UInt16>()) std::reverse(
				std::begin(buffer),
				std::end(buffer)
			);
			
			return retr;
		
		}
		
		
		Endpoint GetEndpoint (const struct sockaddr_storage * addr) noexcept {
		
			Endpoint ep;
			ep.IP=const_cast<struct sockaddr_storage *>(addr);
			ep.Port=get_port(addr);
			
			return ep;
			
		}
	
	
	}


}

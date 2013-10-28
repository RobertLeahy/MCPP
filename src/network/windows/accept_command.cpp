#include <network.hpp>
#include <mswsock.h>
#include <Ws2tcpip.h>
#include <system_error>


namespace MCPP {


	namespace NetworkImpl {
	
	
		AcceptCommand::AcceptCommand (SOCKET socket, SOCKET listening) noexcept
			:	IOCPCommand(NetworkCommand::Accept),
				socket(socket),
				listening(listening)
		{	}
		
		
		AcceptCommand::~AcceptCommand () noexcept {
		
			if (socket!=INVALID_SOCKET) closesocket(socket);
		
		}
		
		
		SOCKET AcceptCommand::Get () noexcept {
		
			//	Inherit
			if (setsockopt(
				socket,
				SOL_SOCKET,
				SO_UPDATE_ACCEPT_CONTEXT,
				reinterpret_cast<char *>(&listening),
				sizeof(listening)
			)==SOCKET_ERROR) {
			
				//	If inheriting failed, close
				//	the socket and return an invalid
				//	value to indicate failure
			
				closesocket(socket);
				
				socket=INVALID_SOCKET;
				
				return INVALID_SOCKET;
			
			}
		
			SOCKET retr=socket;
			socket=INVALID_SOCKET;
			
			return retr;
		
		}
		
		
		static UInt16 get_port (struct sockaddr_storage * addr) noexcept {
		
			return (
				(addr->ss_family==AF_INET6)
					?	reinterpret_cast<struct sockaddr_in6 *>(addr)->sin6_port
					:	reinterpret_cast<struct sockaddr_in *>(addr)->sin_port
			);
		
		}
		
		
		Tuple<IPAddress,UInt16,IPAddress,UInt16> AcceptCommand::GetEndpoints () {
		
			//	Attempt to get the address of the
			//	GetAcceptExSockaddrs function
			GUID guid=WSAID_GETACCEPTEXSOCKADDRS;
			LPFN_GETACCEPTEXSOCKADDRS get_accept_ex_sockaddrs;
			DWORD bytes;	//	Ignored
			if (WSAIoctl(
				socket,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&guid,
				sizeof(guid),
				&get_accept_ex_sockaddrs,
				sizeof(get_accept_ex_sockaddrs),
				&bytes,
				nullptr,
				nullptr
			)==SOCKET_ERROR) throw std::system_error(
				std::error_code(
					WSAGetLastError(),
					std::system_category()
				)
			);
			
			struct sockaddr_storage * remote;
			INT remote_size=sizeof(struct sockaddr_storage);
			struct sockaddr_storage * local;
			INT local_size=sizeof(struct sockaddr_storage);
			get_accept_ex_sockaddrs(
				buffer,
				0,
				sizeof(buffer)/2,
				sizeof(buffer)/2,
				reinterpret_cast<LPSOCKADDR *>(&local),
				&local_size,
				reinterpret_cast<LPSOCKADDR *>(&remote),
				&remote_size
			);
			
			return Tuple<IPAddress,UInt16,IPAddress,UInt16>(
				remote,
				get_port(remote),
				local,
				get_port(local)
			);
		
		}
		
		
		void AcceptCommand::Dispatch (SOCKET listening) {
		
			//	Attempt to get the address of the
			//	WSAAcceptEx function
			GUID guid=WSAID_ACCEPTEX;
			LPFN_ACCEPTEX accept_ex;
			DWORD bytes;	//	This is ignored
			if (WSAIoctl(
				socket,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&guid,
				sizeof(guid),
				&accept_ex,
				sizeof(accept_ex),
				&bytes,
				nullptr,
				nullptr
			)==SOCKET_ERROR) throw std::system_error(
				std::error_code(
					WSAGetLastError(),
					std::system_category()
				)
			);
			
			//	Dispatch asynchronous accept
			if (accept_ex(
				listening,
				socket,
				buffer,
				0,
				sizeof(buffer)/2,
				sizeof(buffer)/2,
				&num,
				reinterpret_cast<LPOVERLAPPED>(this)
			)==SOCKET_ERROR) {
			
				//	Error handling is a bit weird, since
				//	pending is a normal condition, but is
				//	reported as an error, so filter that
				//	out
			
				auto result=WSAGetLastError();
				
				if (result!=WSA_IO_PENDING) throw std::system_error(
					std::error_code(
						result,
						std::system_category()
					)
				);
			
			}
			
		}
		
		
		void AcceptCommand::Imbue (SOCKET socket) noexcept {
		
			this->socket=socket;
		
		}
	
	
	}


}

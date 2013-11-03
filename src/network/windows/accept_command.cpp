#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		AcceptCommand::AcceptCommand (IPAddress ip) noexcept : CompletionCommand(CommandType::Accept), socket(INVALID_SOCKET) {
		
			is_v6=ip.IsV6();
		
		}
		
		
		void AcceptCommand::cleanup () noexcept {
		
			//	Clean up socket this object
			//	contains unless it's already
			//	been cleaned up
			if (socket!=INVALID_SOCKET) {
			
				closesocket(socket);
				socket=INVALID_SOCKET;
			
			}
		
		}
		
		
		AcceptCommand::~AcceptCommand () noexcept {
		
			cleanup();
		
		}
		
		
		void AcceptCommand::Dispatch (SOCKET listening) {
		
			//	If there's already a socket in
			//	this object, clean it up
			cleanup();
			
			//	Create a new socket
			socket=MakeSocket(is_v6);
		
			//	Get the address of the AcceptEx function
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
			)==SOCKET_ERROR) RaiseWSA();
			
			//	Dispatch asynchronous accept
			if (!accept_ex(
				listening,
				socket,
				buffer,
				0,
				sizeof(buffer)/2,
				sizeof(buffer)/2,
				&num,
				reinterpret_cast<LPOVERLAPPED>(this)
			)) {
			
				//	Pending I/O is not an error
				auto result=WSAGetLastError();
				if (result!=ERROR_IO_PENDING) Raise(result);
			
			}
		
		}
		
		
		static UInt16 get_port (struct sockaddr_storage * addr) noexcept {
		
			return (
				(addr->ss_family==AF_INET6)
					?	reinterpret_cast<struct sockaddr_in6 *>(addr)->sin6_port
					:	reinterpret_cast<struct sockaddr_in *>(addr)->sin_port
			);
		
		}
		
		
		AcceptData AcceptCommand::Get () {
		
			//	Get the address of the
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
			)==SOCKET_ERROR) RaiseWSA();
			
			//	Get local and remote addresses
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
			
			//	Prepare structure to be returned
			
			//	Move socket from this structure to
			//	return structure
			AcceptData retr(socket);
			//	We're no longer responsible for this
			//	socket's lifetime
			socket=INVALID_SOCKET;
			
			//	IPs/ports
			retr.RemoteIP=IPAddress(remote);
			retr.RemotePort=get_port(remote);
			retr.LocalIP=IPAddress(local);
			retr.LocalPort=get_port(local);
			
			return retr;
		
		}
	
	
	}


}

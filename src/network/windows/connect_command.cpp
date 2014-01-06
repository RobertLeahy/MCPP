#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		ConnectCommand::ConnectCommand () noexcept : CompletionCommand(CommandType::Connect) {	}
		
		
		void ConnectCommand::Dispatch (SOCKET socket, IPAddress ip, UInt16 port) {
		
			//	Initialize the remote endpoint
			ip.ToOS(&addr,port);
		
			//	Create a bogus, local IP to
			//	bind to
			struct sockaddr_storage local_addr;
			IPAddress::Any(ip.IsV6()).ToOS(&local_addr,0);
			
			//	Bind to local address
			if (bind(
				socket,
				reinterpret_cast<struct sockaddr *>(&local_addr),
				sizeof(local_addr)
			)==SOCKET_ERROR) RaiseWSA();
			
			//	Get the address of ConnectEx
			GUID guid=WSAID_CONNECTEX;
			LPFN_CONNECTEX connect_ex;
			DWORD bytes;	//	Ignored
			if (WSAIoctl(
				socket,
				SIO_GET_EXTENSION_FUNCTION_POINTER,
				&guid,
				sizeof(guid),
				&connect_ex,
				sizeof(connect_ex),
				&bytes,
				nullptr,
				nullptr
			)==SOCKET_ERROR) RaiseWSA();
			
			//	Dispatch an asynchronous connect
			if (!connect_ex(
				socket,
				reinterpret_cast<struct sockaddr *>(&addr),
				sizeof(addr),
				nullptr,
				0,
				nullptr,
				reinterpret_cast<LPOVERLAPPED>(this)
			)) {
			
				//	Make sure I/O isn't pending -- that's
				//	fine
				auto result=WSAGetLastError();
				if (result!=ERROR_IO_PENDING) Raise(result);
			
			}
		
		}
	
	
	}


}

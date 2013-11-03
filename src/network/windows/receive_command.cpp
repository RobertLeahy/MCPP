#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		//	Minimum size of a receive buffer
		static const Word buffer_size=1024;
	
	
		ReceiveCommand::ReceiveCommand () noexcept : CompletionCommand(CommandType::Receive), flags(0) {	}
		
		
		DWORD ReceiveCommand::Dispatch (SOCKET socket) {
		
			//	Does the buffer need to be resized?
			//	If so, resize it
			if (Buffer.Count()==0) Buffer.SetCapacity(buffer_size);
			else if (Buffer.Count()==Buffer.Capacity()) Buffer.SetCapacity();
			
			//	Keep the WSABUF up to date
			buf.len=static_cast<u_long>(SafeWord(Buffer.Capacity()-Buffer.Count()));
			buf.buf=reinterpret_cast<char *>(Buffer.end());
		
			//	Dispatch receive
			if (WSARecv(
				socket,
				&buf,
				1,
				nullptr,
				&flags,
				reinterpret_cast<LPOVERLAPPED>(this),
				nullptr
			)==SOCKET_ERROR) {
			
				//	It's only an error if we
				//	actually failed
				auto result=WSAGetLastError();
				if (result!=WSA_IO_PENDING) return result;
			
			}
			
			return 0;
		
		}
	
	
	}


}

#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		SendCommand::SendCommand (Vector<Byte> buffer) noexcept : CompletionCommand(CommandType::Send), Buffer(std::move(buffer)) {	}
		
		
		DWORD SendCommand::Dispatch (SOCKET socket) noexcept {
		
			//	Prepare WSABUF structure
			buf.len=static_cast<u_long>(Buffer.Count());
			buf.buf=reinterpret_cast<char *>(Buffer.begin());
			
			//	Make WSASend call
			if (WSASend(
				socket,
				&buf,
				1,
				nullptr,
				0,
				reinterpret_cast<LPOVERLAPPED>(this),
				nullptr
			)==SOCKET_ERROR) {
			
				//	It's only an error if we actually
				//	failed
				auto result=WSAGetLastError();
				if (result!=WSA_IO_PENDING) return result;
			
			}
			
			return 0;
		
		}
	
	
	}


}

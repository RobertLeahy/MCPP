#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		//	Default size for a receive buffer
		static const Word default_size=256;
	
	
		ReceiveCommand::ReceiveCommand () noexcept : IOCPCommand(NetworkCommand::Receive), recv_flags(0) {	}
		
		
		bool ReceiveCommand::Dispatch (SOCKET socket) {
		
			//	Make space in the buffer if necessary
			if (Buffer.Capacity()==0) Buffer.SetCapacity(default_size);
			else if (Buffer.Capacity()==Buffer.Count()) Buffer.SetCapacity();
			
			//	Setup buffer
			b.len=static_cast<u_long>(SafeWord(Buffer.Capacity()-Buffer.Count()));
			b.buf=reinterpret_cast<char *>(Buffer.end());
		
			//	Dispatch asynchronous receive
			auto result=WSARecv(
				socket,
				&b,
				1,
				nullptr,
				&recv_flags,
				reinterpret_cast<LPWSAOVERLAPPED>(this),
				nullptr
			);
			
			//	Weird error checking for overlapped I/O
			return (!(
				(result==SOCKET_ERROR) &&
				(WSAGetLastError()!=WSA_IO_PENDING)
			));
		
		}
		
		
		bool ReceiveCommand::Complete (IOCPPacket packet) {
		
			if (!(
				packet.Result &&
				(packet.Num!=0)
			)) return false;
			
			//	Update buffer count
			Buffer.SetCount(Buffer.Count()+packet.Num);
			
			return true;
		
		}
	
	
	}


}

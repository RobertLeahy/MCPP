#include <network.hpp>
#include <system_error>
#include <utility>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	SendHandle::SendHandle (Vector<Byte> buffer) noexcept
		:	IOCPCommand(NetworkCommand::Send),
			buffer(std::move(buffer)),
			state(SendState::InProgress)
	{
	
		b.len=static_cast<u_long>(SafeWord(this->buffer.Count()));
		b.buf=reinterpret_cast<char *>(this->buffer.begin());
		
		sent=0;
	
	}
	
	
	bool SendHandle::Complete (IOCPPacket packet, ThreadPool & pool) {
	
		//	IF the completion port returned false, the
		//	send operation failed
		if (!packet.Result) return false;
		
		//	Update sent count
		sent+=packet.Num;
		
		//	If not all bytes where sent, the send
		//	operation failed
		if (packet.Num!=buffer.Count()) return false;
		
		//	Send was successful
		
		std::exception_ptr ex;
		lock.Execute([&] () mutable {
		
			//	Dispatch callbacks asynchronously
			for (auto & callback : callbacks) try {
			
				pool.Enqueue(
					std::move(callback),
					SendState::Succeeded
				);
			
			} catch (...) {
			
				//	If an exception is thrown, it's
				//	important we finish this, save
				//	it for later
				ex=std::current_exception();
			
			}
			
			//	Set state
			state=SendState::Succeeded;
			
			//	Wake waiting threads
			wait.WakeAll();
		
		});
		
		//	If an exception was thrown, rethrow
		if (ex) std::rethrow_exception(ex);
		
		return true;
	
	}
	
	
	void SendHandle::Dispatch (SOCKET socket) {
	
		if (WSASend(
			socket,
			&b,
			1,
			nullptr,
			0,
			reinterpret_cast<LPWSAOVERLAPPED>(this),
			nullptr
		)==SOCKET_ERROR) {
		
			//	Weird overlapped I/O error-checking
		
			auto result=WSAGetLastError();
			
			if (result!=WSA_IO_PENDING) throw std::system_error(
				std::error_code(
					result,
					std::system_category()
				)
			);
		
		}
	
	}


}

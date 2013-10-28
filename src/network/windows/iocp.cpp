#include <network.hpp>
#include <system_error>


namespace MCPP {


	namespace NetworkImpl {
	
	
		IOCP::IOCP () {
		
			if ((iocp=CreateIoCompletionPort(
				INVALID_HANDLE_VALUE,
				nullptr,
				0,
				0
			))==nullptr) throw std::system_error(
				std::error_code(
					GetLastError(),
					std::system_category()
				)
			);
		
		}
		
		
		void IOCP::destroy () noexcept {
		
			if (iocp!=nullptr) CloseHandle(iocp);
		
		}
		
		
		IOCP::~IOCP () noexcept {
		
			destroy();
		
		}
		
		
		void IOCP::Destroy () noexcept {
		
			destroy();
		
		}
		
		
		void IOCP::Attach (SOCKET socket, void * ptr) {
		
			if (CreateIoCompletionPort(
				reinterpret_cast<HANDLE>(socket),
				iocp,
				reinterpret_cast<ULONG_PTR>(ptr),
				0
			)!=iocp) throw std::system_error(
				std::error_code(
					GetLastError(),
					std::system_category()
				)
			);
		
		}
		
		
		void IOCP::Dispatch (IOCPCommand * command, void * ptr) {
		
			if (!PostQueuedCompletionStatus(
				iocp,
				0,
				reinterpret_cast<ULONG_PTR>(ptr),
				reinterpret_cast<LPOVERLAPPED>(command)
			)) throw std::system_error(
				std::error_code(
					GetLastError(),
					std::system_category()
				)
			);
		
		}
		
		
		IOCPPacket IOCP::Get () {
		
			IOCPPacket retr;
			
			DWORD num;
			retr.Result=GetQueuedCompletionStatus(
				iocp,
				&num,
				reinterpret_cast<PULONG_PTR>(&retr.Conn),
				reinterpret_cast<LPOVERLAPPED *>(&retr.Command),
				INFINITE
			);
			
			//	Error checking
			if (!retr.Result && (retr.Command==nullptr)) throw std::system_error(
				std::error_code(
					GetLastError(),
					std::system_category()
				)
			);
			
			//	Convert DWORD => Word
			retr.Num=static_cast<Word>(SafeInt<DWORD>(num));
			
			return retr;
		
		}
	
	
	}


}

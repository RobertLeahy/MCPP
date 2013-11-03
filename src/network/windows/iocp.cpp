#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		CompletionPort::CompletionPort () {
		
			//	Create the completion port
			if ((iocp=CreateIoCompletionPort(
				INVALID_HANDLE_VALUE,
				nullptr,
				0,
				0
			))==nullptr) Raise();
		
		}
		
		
		CompletionPort::~CompletionPort () noexcept {
		
			CloseHandle(iocp);
		
		}
		
		
		void CompletionPort::Attach (SOCKET socket, void * ptr) {
		
			//	Attempt to attach to the completion port
			if (CreateIoCompletionPort(
				reinterpret_cast<HANDLE>(socket),
				iocp,
				reinterpret_cast<ULONG_PTR>(ptr),
				0
			)==nullptr) Raise();
		
		}
		
		
		Packet CompletionPort::Get () {
		
			//	Attempt to dequeue
			Packet retr;
			retr.Result=GetQueuedCompletionStatus(
				iocp,
				&retr.Count,
				reinterpret_cast<PULONG_PTR>(&retr.Data),
				reinterpret_cast<LPOVERLAPPED *>(&retr.Command),
				INFINITE
			);
			
			if (retr.Result) {
			
				//	Both the dequeue and the I/O operation
				//	associated with the dequeued packet
				//	succeeded
				retr.Error=0;
			
			} else if (retr.Command==nullptr) {
			
				//	The dequeue itself failed
				Raise();
			
			} else {
			
				//	The dequeue succeeded, but the I/O
				//	operation associated with this packet
				//	failed
				retr.Error=GetLastError();
				retr.Count=0;
			
			}
			
			return retr;
		
		}
		
		
		void CompletionPort::Post () {
		
			if (!PostQueuedCompletionStatus(
				iocp,
				0,
				0,
				nullptr
			)) Raise();
		
		}
	
	
	}


}

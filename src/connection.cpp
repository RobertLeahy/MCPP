#include <connection_manager.hpp>


namespace MCPP {


	static const Word default_buf_size=256;


	Connection::Connection (
		Socket socket,
		const IPAddress & ip,
		UInt16 port
	)
		:	socket(std::move(socket)),
			ip(ip),
			port(port),
			recv(default_buf_size),
			recv_alt(default_buf_size),
			recv_task(false),
			send_msg_lock(nullptr),
			send_msg_signal(nullptr),
			saturated(false),
			disconnect_flag(false)
	{
	
		sent=0;
		received=0;
	
	}
	
	
	Connection::~Connection () noexcept {
	
		//	We must fail all pending waits
		//	on sends
		for (auto & tuple : send_queue) {
		
			auto handle=tuple.Item<1>();
			
			handle->lock.Acquire();
			handle->state=SendState::Failed;
			
			for (auto & callback : handle->callbacks) {
			
				try {
				
					callback(SendState::Failed);
				
				} catch (...) {	}
			
			}
			
			handle->wait.WakeAll();
			handle->lock.Release();
		
		}
	
	}

	
	SmartPointer<SendHandle> Connection::Send (Vector<Byte> buffer) {
	
		//	Create handle
		SmartPointer<SendHandle> handle(SmartPointer<SendHandle>::Make());
	
		//	Add data to the queue
	
		send_lock.Acquire();
		
		try {
		
			send_queue.EmplaceBack(
				std::move(buffer),
				handle
			);
		
		} catch (...) {
		
			send_lock.Release();
			
			throw;
		
		}
		
		send_lock.Release();
		
		//	Now notify send thread
		//	if applicable
		connected_lock.Acquire();
		if (send_msg_lock!=nullptr) {
		
			send_msg_lock->Acquire();
			send_msg_signal->WakeAll();
			send_msg_lock->Release();
			
		}
		connected_lock.Release();
		
		//	Return handle
		return handle;
	
	}
	
	
	void Connection::connect (Mutex * lock, CondVar * signal) noexcept {
	
		connected_lock.Acquire();
		send_msg_lock=lock;
		send_msg_signal=signal;
		connected_lock.Release();
	
	}
	
	
	void Connection::disconnect () noexcept {
	
		connected_lock.Acquire();
		send_msg_lock=nullptr;
		send_msg_signal=nullptr;
		connected_lock.Release();
	
	}
	
	
	const IPAddress & Connection::IP () const noexcept {
	
		return ip;
	
	}
	
	
	UInt16 Connection::Port () const noexcept {
	
		return port;
	
	}
	
	
	void Connection::Disconnect (const String & reason) noexcept {
	
		//	Attempt to update reason
		reason_lock.Acquire();
		
		//	This can't throw, so just eat
		//	exceptions and leave the reason
		//	empty
		try {
		
			this->reason=reason;
			
		} catch (...) {	}
		
		reason_lock.Release();
		
		//	Flag for disconnect
		disconnect_flag=true;
		
		//	Wake up send thread to disconnect
		connected_lock.Acquire();
		if (send_msg_lock!=nullptr) {
		
			send_msg_lock->Acquire();
			send_msg_signal->WakeAll();
			send_msg_lock->Release();
			
		}
		connected_lock.Release();
	
	}
	
	
	void Connection::Disconnect (String && reason) noexcept {
		
		//	Attempt to update reason
		reason_lock.Acquire();
		this->reason=std::move(reason);
		reason_lock.Release();
		
		//	Flag for disconnect
		disconnect_flag=true;
		
		//	Wake up send thread to disconnect
		connected_lock.Acquire();
		if (send_msg_lock!=nullptr) {
		
			send_msg_lock->Acquire();
			send_msg_signal->WakeAll();
			send_msg_lock->Release();
			
		}
		connected_lock.Release();
	
	}
	
	
	void Connection::Disconnect () noexcept {
	
		//	Flag for disconnect
		disconnect_flag=true;
		
		//	Wake up send thread to disconnect
		connected_lock.Acquire();
		if (send_msg_lock!=nullptr) {
		
			send_msg_lock->Acquire();
			send_msg_signal->WakeAll();
			send_msg_lock->Release();
		
		}
		connected_lock.Release();
	
	}
	
	
	UInt64 Connection::Sent () const noexcept {
	
		return sent;
	
	}
	
	
	UInt64 Connection::Received () const noexcept {
	
		return received;
	
	}
	

}

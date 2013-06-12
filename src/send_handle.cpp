#include <connection_manager.hpp>
#include <utility>


namespace MCPP {


	SendHandle::SendHandle () : state(SendState::Pending), callbacks(0) {	}
	
	
	SendState SendHandle::State () noexcept {
	
		lock.Acquire();
		
		SendState returnthis=state;
		
		lock.Release();
		
		return returnthis;
	
	}
	
	
	SendState SendHandle::Wait () noexcept {
	
		lock.Acquire();
	
		while (
			(state!=SendState::Sent) &&
			(state!=SendState::Failed)
		) wait.Sleep(lock);
		
		SendState returnthis=state;
		
		lock.Release();
		
		return returnthis;
	
	}
	
	
	Word SendHandle::Sent () noexcept {
	
		return sent;
	
	}
	
	
	void SendHandle::AddCallback (SendCallback callback) {
	
		bool execute=false;
		lock.Acquire();
		
		try {
		
			if (
				callback &&
				(
					(state==SendState::Sent) ||
					(state==SendState::Failed)
				)
			) execute=true;
			else callbacks.Add(std::move(callback));
			
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
		
		if (execute) callback(state);
	
	}


}

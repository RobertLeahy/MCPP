#include <connection_manager.hpp>


namespace MCPP {


	SendHandle::SendHandle () : state(SendState::Pending) {	}
	
	
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


}

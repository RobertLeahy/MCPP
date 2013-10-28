#include <network.hpp>
#include <utility>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	void SendHandle::Then (std::function<void (SendState)> callback) {
	
		lock.Execute([&] () mutable {	callbacks.Add(std::move(callback));	});
	
	}
	
	
	void SendHandle::Complete () noexcept {
	
		state=SendState::Succeeded;
	
	}
	
	
	SendState SendHandle::State () const noexcept {
	
		return lock.Execute([&] () {	return state;	});
	
	}
	
	
	SendState SendHandle::Wait () const noexcept {
	
		return lock.Execute([&] () {
		
			while (state==SendState::InProgress) wait.Sleep(lock);
			
			return state;
		
		});
	
	}
	
	
	void SendHandle::Fail () noexcept {
	
		lock.Execute([&] () mutable {
			
			for (auto & callback : callbacks) try {
			
				if (callback) callback(SendState::Failed);
			
			//	We don't care abouc exceptions
			} catch (...) {	}
			
			//	Set state
			state=SendState::Failed;
			
			//	Wake up waiting threads
			wait.WakeAll();
		
		});
	
	}
	
	
}

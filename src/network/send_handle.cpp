#include <network.hpp>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	void SendHandle::Fail () noexcept {
	
		lock.Execute([&] () mutable {
		
			state=SendState::Failed;
			
			for (auto & callback : callbacks) try {
			
				callback(state);
			
			//	Eat exceptions, we don't care about
			//	them, it's not our code executing
			} catch (...) {	}
			
			//	Wake up waiting threads
			wait.WakeAll();
		
		});
	
	}
	
	
	void SendHandle::Complete (ThreadPool & pool) {
	
		lock.Execute([&] () mutable {
		
			state=SendState::Sent;
			
			//	We MUST make an effort to fire all
			//	callbacks otherwise critical code
			//	may not execute, therefore we store
			//	exceptions to be rethrown later
			std::exception_ptr ex;
			for (auto & callback : callbacks) try {
			
				pool.Enqueue(
					std::move(callback),
					state
				);
			
			} catch (...) {
			
				ex=std::current_exception();
			
			}
			
			//	Wake up waiting threads
			wait.WakeAll();
			
			//	If there was an error, now is the
			//	time to rethrow it
			if (ex) std::rethrow_exception(ex);
		
		});
	
	}
	
	
	void SendHandle::SetState (SendState state) noexcept {
	
		this->state=state;
	
	}


	SendState SendHandle::Wait () const noexcept {
	
		return lock.Execute([&] () {
		
			while (state==SendState::Sending) wait.Sleep(lock);
			
			return state;
			
		});
	
	}
	
	
	void SendHandle::Then (std::function<void (SendState)> callback) {
	
		lock.Execute([&] () mutable {	callbacks.Add(std::move(callback));	});
	
	}
	
	
	SendState SendHandle::State () const noexcept {
	
		return lock.Execute([&] () {	return state;	});
	
	}


}

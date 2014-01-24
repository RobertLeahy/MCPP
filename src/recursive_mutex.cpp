#include <recursive_mutex.hpp>


namespace MCPP {


	void RecursiveMutex::Acquire () noexcept {
	
		//	Get this thread's ID
		auto id=Thread::ID();
	
		lock.Execute([&] () mutable {
		
			//	If the ID is not null, we may
			//	not be able to acquire at once
			if (!this->id.IsNull()) {
			
				//	If this thread is actually currently
				//	holding the lock, just increment the
				//	depth and return
				if (*(this->id)==id) {
				
					++depth;
					
					return;
				
				}
				
				//	Another thread is holding the lock,
				//	wait for it not to be
				do wait.Sleep(lock);
				while (!this->id.IsNull());
			
			}
			
			//	Acquire the lock
			this->id.Construct(id);
			depth=1;
		
		});
	
	}
	
	
	void RecursiveMutex::Release () noexcept {
		
		lock.Execute([&] () mutable {
		
			//	Decrement the depth counter, if
			//	this is the bottom, release the
			//	lock
			if ((--depth)==0) {
			
				id.Destroy();
				
				wait.Wake();
			
			}
		
		});
	
	}


}

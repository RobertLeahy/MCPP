#include <thread_pool.hpp>


namespace MCPP {


	ThreadPoolHandle::ThreadPoolHandle () noexcept : done(false), success(false), ptr(nullptr) {	}
	
	
	ThreadPoolHandle::~ThreadPoolHandle () noexcept {
	
		if (
			static_cast<bool>(cleanup) &&
			(ptr!=nullptr)
		) {
		
			try {
		
				cleanup(ptr);
				
			} catch (...) {	}
			
		}
	
	}
	
	
	bool ThreadPoolHandle::Completed () noexcept {
	
		lock.Acquire();
		
		bool returnthis=done;
		
		lock.Release();
		
		return returnthis;
	
	}
	
	
	bool ThreadPoolHandle::Success () noexcept {
	
		lock.Acquire();
		
		bool returnthis=success;
		
		lock.Release();
		
		return returnthis;
	
	}
	
	
	void ThreadPoolHandle::Wait () noexcept {
	
		lock.Acquire();
		
		try {
		
			while (!done) wait.Sleep(lock);
		
		} catch (...) {	}
		
		lock.Release();
	
	}


}

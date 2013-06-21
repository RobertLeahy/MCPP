#include <thread_pool.hpp>


namespace MCPP {


	ThreadPoolHandle::ThreadPoolHandle () noexcept {
	
		result=nullptr;
		queued=0;
		running=0;
		status=static_cast<Word>(ThreadPoolStatus::Queued);
	
	}
	
	
	ThreadPoolHandle::~ThreadPoolHandle () noexcept {
	
		if (cleanup) cleanup(result);
	
	}
	
	
	ThreadPoolStatus ThreadPoolHandle::Status () const noexcept {
	
		return static_cast<ThreadPoolStatus>(
			static_cast<Word>(status)
		);
	
	}
	
	
	bool ThreadPoolHandle::Completed () const noexcept {
	
		auto status=Status();
		
		return (status==ThreadPoolStatus::Success) || (status==ThreadPoolStatus::Error);
	
	}
	
	
	bool ThreadPoolHandle::Success () const noexcept {
	
		return Status()==ThreadPoolStatus::Success;
	
	}
	
	
	UInt64 ThreadPoolHandle::Queued () const noexcept {
	
		UInt64 returnthis;
	
		lock.Acquire();
		
		try {
		
			if (Status()==ThreadPoolStatus::Queued) returnthis=timer.ElapsedNanoseconds();
			else returnthis=queued;
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
	
		return returnthis;
	
	}
	
	
	UInt64 ThreadPoolHandle::Running () const noexcept {
	
		UInt64 returnthis;
		
		lock.Acquire();
		
		try {
		
			if (Status()==ThreadPoolStatus::Running) returnthis=timer.ElapsedNanoseconds();
			else returnthis=running;
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
		
		return returnthis;
	
	}
	
	
	void ThreadPoolHandle::Wait () const noexcept {
	
		lock.Acquire();
		
		for (;;) {
		
			auto status=Status();
			
			if (
				(status==ThreadPoolStatus::Success) ||
				(status==ThreadPoolStatus::Error)
			) break;
			
			wait.Sleep(lock);
		
		}
		
		lock.Release();
	
	}


}

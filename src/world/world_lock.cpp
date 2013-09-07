#include <world/world.hpp>


namespace MCPP {


	WorldLock::WorldLock (ThreadPool & pool, std::function<void ()> panic) noexcept : count(0), pool(pool), panic(std::move(panic)) {	}
	
	
	void WorldLock::acquire (std::function<void (bool)> callback) {
	
		//	Which thread is this?
		auto id=Thread::ID();
		
		lock.Acquire();
		
		if (
			//	We can acquire immediately
			//	because no thread currently
			//	holds the lock
			(count==0) ||
			//	We can acquire recursively because
			//	this thread currently holds the
			//	lock
			(
				!this->id.IsNull() &&
				(*(this->id)==id)
			)
		) {
		
			//	Acquire
			
			this->id.Construct(id);
			++count;
			
			lock.Release();
			
			//	If this lock is asynchronous
			//	invoke the callback at once
			if (callback) try {
			
				callback(
					//	We've already set the
					//	thread, no need for the
					//	callback to acquire the
					//	lock and do so again
					false
				);
				
			//	Had this callback actually been
			//	invoked asynchronously -- i.e.
			//	at some time later -- the caller
			//	would not receive exceptions from
			//	it, so we simply eat them
			} catch (...) {	}
		
		} else {
		
			//	Enqueue or block
			
			bool sync=!callback;
			
			try {
			
				//	Add to list of pending
				//	tasks/threads
				list.Add(std::move(callback));
				
			} catch (...) {
			
				lock.Release();
				
				throw;
			
			}
			
			if (sync) {
			
				//	If synchronous, wait until
				//	we can acquire
			
				while (count!=0) wait.Sleep(lock);
				
				//	Acquire
				this->id.Construct(id);
				++count;
			
			}
			
			lock.Release();
		
		}
	
	}
	
	
	void WorldLock::Acquire () {
	
		acquire(std::function<void (bool)>());
	
	}
	
	
	void WorldLock::Release () {
	
		auto id=Thread::ID();
	
		lock.Acquire();
		
		if (
			//	Is the lock held?
			!this->id.IsNull() &&
			//	Do we hold it?
			(*(this->id)==id) &&
			//	Are we releasing or just
			//	winding back through recursively-
			//	acquired locks?
			((--count)==0)
		) {
		
			//	Actually releasing
			
			//	Make sure we null this out,
			//	otherwise if this thread reenters
			//	the lock before an asynchronous
			//	callback takes ownership of it,
			//	two threads could have the lock
			this->id.Destroy();
			
			if (list.Count()!=0) {
			
				//	There are waiting tasks/threads
				
				auto callback=std::move(list[0]);
				list.Delete(0);
				
				if (callback) {
					
					//	Dispatch an asynchronous task
					
					//	Acquire the lock on behalf
					//	of the callback we're about
					//	to dispatch
					++count;
					
					try {
					
						pool.Enqueue(
							std::move(callback),
							//	We don't know which thread
							//	will run the callback, so it
							//	must set its own thread ID
							//	when it begins executing
							true
						);
					
					} catch (...) {
					
						lock.Release();
					
						panic();
						
						throw;
					
					}
					
				} else {
				
					//	Release a waiting thread
					
					wait.Wake();
				
				}
			
			}
		
		}
		
		lock.Release();
	
	}
	
	
	bool WorldLock::Transfer () noexcept {
	
		auto id=Thread::ID();
	
		lock.Acquire();
		
		bool retr=false;
		//	Are we holding the lock?
		if (
			!this->id.IsNull() &&
			(*(this->id)==id)
		) {
		
			//	YES, release and transfer
			//	out
			
			retr=true;
			
			this->id.Destroy();
			
			//	When we transfer out, we lose
			//	all layers of recursion
			count=1;
		
		}
		
		lock.Release();
		
		return retr;
	
	}
	
	
	void WorldLock::Resume () noexcept {
	
		auto id=Thread::ID();
		
		lock.Acquire();
		
		this->id.Construct(id);
		
		lock.Release();
	
	}


}

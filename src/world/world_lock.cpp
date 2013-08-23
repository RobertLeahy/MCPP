#include <world/world.hpp>
#include <stdexcept>


namespace MCPP {


	static const ASCIIChar * release_error="One or more locks could not be released";


	WorldLock::WorldLock (ThreadPool & pool) noexcept : pool(pool) {	}
	
	
	void WorldLock::acquire (SmartPointer<WorldLockInfo> info) {
	
		lock.Acquire();
		
		//	See if we can acquire by
		//	checking to see if this lock
		//	contends
		
		bool can_acquire=true;
		
		for (const auto & i : held)
		if (i->Request.DoesContendWith(info->Request)) {
		
			can_acquire=false;
			
			break;
		
		}
		
		if (can_acquire)
		for (const auto & i : pending)
		if (i->Request.DoesContendWith(info->Request)) {
		
			can_acquire=false;
			
			break;
		
		}
		
		if (can_acquire) {
		
			try {
			
				//	Attempt to insert into
				//	list of held locks
				held.Add(info);
				
				try {
				
					//	Attempt to dispatch async
					//	callback if it exists
					info->Wake(pool);
				
				} catch (...) {
				
					held.Delete(held.Count()-1);
					
					throw;
				
				}
			
			} catch (...) {
			
				lock.Release();
				
				throw;
			
			}
			
			lock.Release();
		
		} else {
		
			try {
			
				//	Attempt to insert into list
				//	of pending locks
				pending.Add(info);
			
			} catch (...) {
			
				lock.Release();
				
				throw;
			
			}
			
			lock.Release();
			
			//	Wait (if necessary) for lock
			//	to be acquired
			info->Wait();
		
		}
	
	}
	
	
	inline void WorldLock::release (const void * ptr) {
	
		lock.Acquire();
		
		//	Attempt to find lock
		
		bool found=false;
		
		for (Word i=0;i<held.Count();++i)
		if (reinterpret_cast<const WorldLockInfo *>(ptr)==static_cast<WorldLockInfo *>(held[i])) {
		
			//	Remove the lock
			held.Delete(i);
		
			found=true;
			
			break;
		
		}
		
		//	Return if the pointer is
		//	bogus
		if (!found) {
		
			lock.Release();
		
			return;
			
		}
		
		//	Attempt to acquire pending
		//	locks
		bool error=false;
		for (Word i=0;i<pending.Count();) {
		
			bool can_acquire=true;
		
			//	Check with all held locks to
			//	see if this lock contends
			for (const auto & h : held)
			if (h->Request.DoesContendWith(pending[i]->Request)) {
			
				can_acquire=false;
				
				break;
			
			}
			
			//	Check with all preceding pending
			//	locks to see if this lock contends
			//	(this avoids starvation)
			if (can_acquire)
			for (Word n=0;n<i;++n)
			if (pending[n]->Request.DoesContendWith(pending[i]->Request)) {
			
				can_acquire=false;
				
				break;
			
			}
			
			if (!can_acquire) {
			
				++i;
				
				continue;
			
			}
			
			//	Can acquire
			
			//	Remove from pending
			auto curr=std::move(pending[i]);
			pending.Delete(i);
			
			//	Attempt to add to list of
			//	held locks
			try {
			
				held.Add(curr);
				
				//	Attempt to awaken waiting
				//	thread/dispatch async
				//	callback
				try {
				
					curr->Wake(pool);
				
				} catch (...) {
				
					held.Delete(held.Count()-1);
					
					throw;
				
				}
			
			} catch (...) {
			
				error=true;
			
			}
		
		}
		
		lock.Release();
		
		if (error) throw std::runtime_error(release_error);
	
	}
	
	
	void WorldLock::upgrade (
		const void * ptr,
		const WorldLockRequest & request,
		Nullable<std::function<void (const void *)>> callback
	) {
	
		lock.Acquire();
		
		//	Attempt to find lock
		
		Nullable<Word> index;
		
		for (Word i=0;i<held.Count();++i)
		if (reinterpret_cast<const WorldLockInfo *>(ptr)==static_cast<WorldLockInfo *>(held[i])) {
		
			index=i;
			
			break;
		
		}
		
		//	Abort if the pointer we
		//	were passed is garbage
		if (index.IsNull()) {
		
			lock.Release();
			
			return;
		
		}
		
		//	Upgrade
		try {
		
			held[*index]->Request.Merge(request);
		
		} catch (...) {
		
			lock.Release();
			
			try {
			
				release(ptr);
			
			} catch (...) {	}
			
			throw;
		
		}
		
		//	Set new wait method
		if (callback.IsNull()) held[*index]->Synchronous();
		else held[*index]->Asynchronous(std::move(*callback));
		
		//	Test to make sure new lock
		//	is compatible with currently
		//	held locks
		
		bool can_acquire=true;
		
		for (Word i=0;i<held.Count();++i)
		if (
			(i!=*index) &&
			held[i]->Request.DoesContendWith(held[*index]->Request)
		) {
		
			can_acquire=false;
			
			break;
		
		}
		
		//	If we can acquire at once, fire
		//	the callback (if necessary)
		if (can_acquire) {
		
			try {
			
				held[*index]->Wake(pool);
			
			} catch (...) {
			
				lock.Release();
				
				try {
				
					release(ptr);
				
				} catch (...) {	}
				
				throw;
			
			}
			
			lock.Release();
		
		//	If we cannot acquire, put this lock
		//	at the front of the pending queue and
		//	wait (if necessary)
		} else {
		
			auto wait=held[*index];
		
			try {
			
				pending.Insert(
					wait,
					0
				);
			
			} catch (...) {
			
				lock.Release();
				
				try {
				
					release(ptr);
				
				} catch (...) {	}
				
				throw;
			
			}
			
			held.Delete(*index);
			
			lock.Release();
			
			wait->Wait();
		
		}
	
	}
	
	
	const void * WorldLock::Acquire (WorldLockRequest request) {
	
		auto handle=SmartPointer<WorldLockInfo>::Make(std::move(request));
		
		const void * retr=static_cast<WorldLockInfo *>(handle);
		
		acquire(std::move(handle));
		
		return retr;
	
	}
	
	
	void WorldLock::Release (const void * handle) {
	
		release(handle);
	
	}
	
	
	void WorldLock::Upgrade (const void * handle, const WorldLockRequest & request) {
	
		upgrade(
			handle,
			request,
			Nullable<std::function<void (const void *)>>()
		);
	
	}


}

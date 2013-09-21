#include <world/world.hpp>


namespace MCPP {


	WorldLock::WorldLock (
		ThreadPool & pool,
		std::function<void ()> panic
	) noexcept
		:	num(0),
			exclusive(false),
			pool(pool),
			panic(std::move(panic))
	{	}
	
	
	static inline void fire (const std::function<void (bool)> & callback, bool pass) noexcept {
	
		if (callback) try {
		
			callback(pass);
		
		} catch (...) {	}
	
	}
	
	
	inline void WorldLock::acquire_impl (const std::function<void (bool)> & callback, decltype(Thread::ID()) id, bool ex) {
	
		locks.emplace(
			id,
			1
		);
	
		exclusive=ex;
		++num;
		
		fire(callback,false);
	
	}
	
	
	void WorldLock::acquire (std::function<void (bool)> callback, bool ex) {
	
		auto id=Thread::ID();
		bool sync=!callback;
	
		lock.Execute([&] () mutable {
		
			//	Can we acquire immediately or
			//	do we have to wait?
			
			//	If there are no locks, acquire
			//	at once
			if (num==0) {
			
				acquire_impl(callback,id,ex);
				
				return;
			
			}
			
			//	If this thread currently holds
			//	the lock, we simply recursively
			//	acquire
			auto iter=locks.find(id);
			if (iter!=locks.end()) {
			
				//	Increment recursion count
				++iter->second;
				
				fire(callback,false);
				
				return;
			
			}
			
			//	If there are no pending locks,
			//	non-exclusive locks may acquire
			//	provided there is not an exclusive
			//	lock currently held.
			//
			//	Exclusive locks can acquire only if
			//	there is no lock currently held.
			if (
				(pending.Count()==0) &&
				(
					ex
						?	(num==0)
						:	!exclusive
				)
			) {
			
				acquire_impl(callback,id,ex);
				
				return;
			
			}
			
			//	We must block/enqueue
			
			std::atomic<bool> woken;
			woken=false;
			
			pending.EmplaceBack(
				std::move(callback),
				ex,
				&woken,
				id
			);
			
			//	If synchronous, wait to be
			//	woken up
			if (sync) do (ex ? ex_wait : wait).Sleep(lock);
			while (!woken);
		
		});
	
	}
	
	
	inline bool WorldLock::can_acquire (const wl_tuple & t) noexcept {
	
		//	No locks, can always acquire
		if (num==0) return true;
		
		//	Exclusive held, can never acquire
		if (exclusive) return false;
		
		//	This lock is exclusive, and as per
		//	above there is at least one held
		//	lock, therefore cannot acquire
		return !t.Item<1>();
	
	}
	
	
	void WorldLock::release () {
	
		auto id=Thread::ID();
		
		lock.Execute([&] () {
		
			//	Look up the lock this thread
			//	holds
			auto iter=locks.find(id);
			
			//	Short-circuit out if this thread
			//	doesn't actually hold a lock, or
			//	if we're just winding back through
			//	a recursive acquisition
			if (
				(iter==locks.end()) ||
				((--iter->second)!=0)
			) return;
			
			//	Release
			locks.erase(iter);
			exclusive=false;
			--num;
			
			//	Flush out pending requests
			while (
				(pending.Count()!=0) &&
				can_acquire(pending[0])
			) {
			
				//	Extract lock request
				auto & t=pending[0];
				auto callback=std::move(t.Item<0>());
				exclusive=t.Item<1>();
				auto * wake=t.Item<2>();
				auto id=t.Item<3>();
				pending.Delete(0);
				
				//	Maintain counter
				++num;
				
				try {
				
					if (callback) {
					
						//	Asynchronous
						
						pool.Enqueue(
							std::move(callback),
							true
						);
					
					} else {
					
						//	Synchronous
						
						//	Insert lock
						locks.emplace(
							id,
							1
						);
						
						//	Release waiting thread
						*wake=true;
						(exclusive ? ex_wait : wait).WakeAll();
					
					}
					
				} catch (...) {
				
					//	The lock may be in an inconsistent
					//	state!
					try {	if (panic) panic();	} catch (...) {	}
					
					//	Attempt recovery as best we can
					--num;
					exclusive=false;
					
					throw;
				
				}
			
			}
		
		});
	
	}
	
	
	void WorldLock::Acquire () {
	
		acquire(
			std::function<void (bool)>(),
			false
		);
	
	}
	
	
	void WorldLock::AcquireExclusive () {
	
		acquire(
			std::function<void (bool)>(),
			true
		);
	
	}
	
	
	bool WorldLock::Transfer () noexcept {
	
		auto id=Thread::ID();
	
		return lock.Execute([&] () {
		
			return locks.erase(id)!=0;
		
		});
	
	}
	
	
	void WorldLock::Resume () {
	
		auto id=Thread::ID();
		
		lock.Execute([&] () {
		
			locks.emplace(
				id,
				1
			);
		
		});
	
	}
	
	
	void WorldLock::Release () {
	
		release();
	
	}


}

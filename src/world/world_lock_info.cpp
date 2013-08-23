#include <world/world.hpp>


namespace MCPP {


	WorldLockInfo::WorldLockInfo (WorldLockRequest request) noexcept
		:	sync(true),
			wake(false),
			Request(std::move(request))
	{	}
	
	
	WorldLockInfo::WorldLockInfo (std::function<void (const void *)> callback, WorldLockRequest request) noexcept
		:	sync(false),
			callback(std::move(callback)),
			Request(std::move(request))
	{	}
	
	
	void WorldLockInfo::Wait () noexcept {
	
		if (sync) {
		
			lock.Acquire();
			while (!wake) wait.Sleep(lock);
			wake=false;
			lock.Release();
		
		}
	
	}
	
	
	void WorldLockInfo::Wake (ThreadPool & pool) {
	
		if (sync) {
		
			lock.Acquire();
			wake=true;
			wait.Wake();
			lock.Release();
		
		} else {
		
			if (callback) pool.Enqueue(
				std::move(callback),
				this
			);
		
		}
	
	}
	
	
	void WorldLockInfo::Asynchronous (std::function<void (const void *)> callback) noexcept {
	
		sync=false;
		this->callback=std::move(callback);
	
	}
	
	
	void WorldLockInfo::Synchronous () noexcept {
	
		sync=true;
	
	}


}

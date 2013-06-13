#include <thread_pool.hpp>
#include <utility>


namespace MCPP {


	static const char * thread_pool_startup_error="Supplied thread initializer threw exception";


	void ThreadPool::pool_func (void * ptr) noexcept {
	
		reinterpret_cast<ThreadPool *>(ptr)->pool_func_impl();
	
	}
	
	
	void ThreadPool::pool_func_impl () noexcept {
	
		//	Wait until startup is done
		queue_lock.Acquire();
		while (!(stop || begin)) queue_sleep.Sleep(queue_lock);
		queue_lock.Release();
		
		//	Startup failed?  Return
		if (stop) return;
		
		//	Fire startup handler
		bool initialized=false;
		try {
		
			if (startup) startup();
			
			initialized=true;
		
		} catch (...) {
		
			//	Init failed, signal shutdown
			stop=true;
		
		}
		
		//	Wait for other threads
		sync->Enter();
		
		//	If we failed, end
		if (stop) goto end;
	
		//	Begin work
		for (;;) {
	
			queue_lock.Acquire();
			
			//	Wait for something to do
			while (
				(queue.Count()==0) &&
				!stop
			) queue_sleep.Sleep(queue_lock);
			
			//	Die if appropriate
			if (stop) {
			
				queue_lock.Release();
				
				break;
			
			}
			
			//	Dequeue a task
			Tuple<
				CallbackType,
				SmartPointer<ThreadPoolHandle>
			> task(std::move(queue[0]));
			
			queue.Delete(0);
			
			//	Wake up parent thread
			//	if it's waiting for us
			queue_wait.WakeAll();
			
			queue_lock.Release();
			
			ThreadPoolHandle & handle=*(task.Item<1>());
			
			//	Execute task
			try {
			
				task.Item<0>()(&(handle.ptr),&(handle.cleanup));
				
				//	Success
				handle.lock.Acquire();
				
				handle.success=true;
				handle.done=true;
				
				handle.wait.WakeAll();
				
				handle.lock.Release();
			
			} catch (...) {
			
				//	Error
				handle.lock.Acquire();
				
				handle.success=false;
				handle.done=true;
				
				handle.wait.WakeAll();
				
				handle.lock.Release();
			
			}
			
		}
		
		//	DIE
		
		end:
		
		//	Call cleanup
		if (initialized && shutdown) {
		
			try {
			
				shutdown();
			
			} catch (...) {	}
		
		}
	
	}


	ThreadPool::ThreadPool (Word num, ThreadPoolStartup startup, ThreadPoolShutdown shutdown) : pool((num==0) ? 1 : num), startup(std::move(startup)), shutdown(std::move(shutdown)), stop(false), begin(false) {
	
		num=(num==0) ? 1 : num;
	
		//	Spawn
		try {
		
			for (Word i=0;i<num;++i) pool.EmplaceBack(
				pool_func,
				this
			);
			
			//	Prepare the barrier
			sync.Construct(Word(SafeWord(num)+SafeWord(1)));
			
			//	Release the threads
			queue_lock.Acquire();
			begin=true;
			queue_sleep.WakeAll();
			queue_lock.Release();
			
			//	Wait for threads to finish calling
			//	startup handlers
			sync->Enter();
			
		} catch (...) {
		
			//	Failed spawning, abort!
			
			queue_lock.Acquire();
			
			stop=true;
			
			queue_sleep.WakeAll();
			
			queue_lock.Release();
			
			for (Thread & t : pool) t.Join();
			
			throw;
		
		}
		
		//	Did init fail?
		if (stop) {
		
			//	Yes, they should all die in
			//	short order
			for (Thread & t : pool) t.Join();
			
			throw std::runtime_error(thread_pool_startup_error);
		
		}
		
		//	We're good, all threads initialized and running
	
	}
	
	
	ThreadPool::~ThreadPool () noexcept {
	
		//	Wait for all pending tasks
		//	to be flushed out
		queue_lock.Acquire();
		
		while (queue.Count()!=0) queue_wait.Sleep(queue_lock);
		
		//	Order stop
		stop=true;
		queue_sleep.WakeAll();
		
		queue_lock.Release();
		
		//	Wait for all threads to die
		for (Thread & t : pool) t.Join();
	
	}
	
	
	SmartPointer<ThreadPoolHandle> ThreadPool::Enqueue (const CallbackType & func) {
	
		//	Create handle
		SmartPointer<ThreadPoolHandle> handle=SmartPointer<ThreadPoolHandle>::Make();
		
		//	Enqueue task
		queue_lock.Acquire();
		
		try {
		
			queue.EmplaceBack(
				func,
				handle
			);
			
			//	Wake up a worker to handle
			//	this
			queue_sleep.Wake();
		
		} catch (...) {
		
			queue_lock.Release();
			
			throw;
		
		}
		
		queue_lock.Release();
		
		return handle;
	
	}


}

#include <thread_pool.hpp>
#include <stdexcept>


namespace MCPP {


	static const char * init_error="Error running initializers in thread pool";


	void ThreadPool::worker_func (Word i) noexcept {
	
		//	Wait for startup to complete
		queue_lock.Acquire();
		while (!startup_complete) queue_wait.Sleep(queue_lock);
		//	This would indicate an error
		if (stop) {
		
			queue_lock.Release();
			
			return;
		
		}
		queue_lock.Release();
		
		//	Call init function
		bool init_success=true;
		try {
		
			if (init) init();
		
		} catch (...) {
		
			init_success=false;
		
		}
		
		queue_lock.Acquire();
		//	Tell other threads to die
		//	if we failed to initialize
		if (!init_success) stop=true;
		//	If we're the last thread in
		//	wake the other threads up
		if (++init_count==workers.Count()) queue_wait.WakeAll();
		//	Otherwise wait for them to all
		//	finish
		else while (init_count!=workers.Count()) queue_wait.Sleep(queue_lock);
		//	Get value of stop to determine whether
		//	we need to abort
		bool abort=stop;
		queue_lock.Release();
		
		//	Cleanup and abort if necessary
		if (abort) {
		
			//	Only cleanup if we actually initialized
			if (init_success) try {	if (cleanup) cleanup();	} catch (...) {	}
			
			return;
		
		}
		
		//	BEGIN
		
		//	Enter worker loop
		try {
		
			for (;;) {
			
				//	The current task
				Tuple<
					SmartPointer<ThreadPoolHandle>,
					std::function<void ()>
				> curr;
			
				//	Lock queue
				queue_lock.Acquire();
				
				try {
				
					auto time=timer.ElapsedNanoseconds();
					for (;;time=timer.ElapsedNanoseconds()) {
					
						//	Break out immediately if
						//	there are things to be done
						//	or if we must stop
						if (
							stop ||
							(queue.Count()!=0)
						) break;
					
						if (scheduled.Count()==0) {
					
							//	There are no scheduled tasks,
							//	wait until we are woken up
							
							queue_wait.Sleep(queue_lock);
							
						} else {
							
							//	There is a scheduled task at some
							//	point in the future, wait until
							//	woken up or until that time arrives,
							//	whichever come first
							
							UInt64 next=(
								//	Next scheduled task is in the future
								(scheduled[0].Item<1>()>time)
									?	scheduled[0].Item<1>()-time
									:	0
							);
							
							//	Break out if the next scheduled
							//	callback is less than a millisecond
							//	away
							if (next<1000000) break;
							
							//	Sleep
							queue_wait.Sleep(
								queue_lock,
								static_cast<Word>(next/1000000)
							);
						
						}
						
					}
					
					//	Stop if requested
					if (stop) {
					
						//	Make sure lock gets released
						queue_lock.Release();
						
						//	Exit
						break;
					
					}
					
					//	Queue up all scheduled tasks
					//	that are due to be run
					bool enqueued=false;
					while (
						(scheduled.Count()!=0) &&
						(
							(scheduled[0].Item<1>()<=time) ||
							((scheduled[0].Item<1>()-time)<1000000)
						)
					) {
					
						queue.Emplace(
							0,
							std::move(scheduled[0].Item<0>()),
							std::move(scheduled[0].Item<2>())
						);
						
						scheduled.Delete(0);
						
						enqueued=true;
					
					}
					
					//	Get a task
					curr=std::move(queue[0]);
					queue.Delete(0);
					
					if (enqueued) queue_wait.WakeAll();
					
				} catch (...) {
				
					queue_lock.Release();
					
					throw;
				
				}
				
				//	Done with the lock
				queue_lock.Release();
				
				auto & handle=curr.Item<0>();
				auto & func=curr.Item<1>();
				
				//	Execute task
				
				try {
				
					//	Finalize time the task
					//	spent queued and flag
					//	the task as running
					handle->lock.Acquire();
					
					try {
					
						handle->queued=handle->timer.ElapsedNanoseconds();
						handle->timer.Reset();
						
						handle->status=static_cast<Word>(ThreadPoolStatus::Running);
					
					} catch (...) {
					
						handle->lock.Release();
						
						throw;
					
					}
					
					handle->lock.Release();
					
					//	Flag set if task fails
					bool failed=false;
					
					//	We are now running a task
					Timer timer=Timer::CreateAndStart();
					++running;
					
					try {
					
						//	Execute
						func();
					
					} catch (...) {
					
						//	Task failed
						failed=true;
					
					}
					
					//	Done
					
					//	This thread is no longer
					//	running a task
					auto elapsed=timer.ElapsedNanoseconds();
					workers[i].Item<0>()+=elapsed;
					++workers[i].Item<1>();
					--running;
					if (failed) ++workers[i].Item<2>();
					
					//	Finalize time the task
					//	spent running and flag
					//	the task as completed
					handle->lock.Acquire();
					
					try {
					
						handle->running=handle->timer.ElapsedNanoseconds();
						
						handle->status=static_cast<Word>(
							failed
								?	ThreadPoolStatus::Error
								:	ThreadPoolStatus::Success
						);
						
						//	Wake up observers
						handle->wait.WakeAll();
					
					} catch (...) {
					
						handle->lock.Release();
						
						throw;
					
					}
					
					handle->lock.Release();
				
				} catch (...) {
				
					//	Make sure everyone waiting
					//	on this task is released
					handle->lock.Acquire();
					handle->status=static_cast<Word>(ThreadPoolStatus::Error);
					handle->wait.WakeAll();
					handle->lock.Release();
					
					//	Propagate, we shouldn't be
					//	having exceptions thrown
					throw;
				
				}
			
			}
		
		} catch (...) {
		
			//	PANIC
			try {	if (panic) panic();	} catch (...) {	}
		
		}
		
		//	Cleanup
		try {	if (cleanup) cleanup();	} catch (...) {	}
	
	}
	
	
	ThreadPool::ThreadPool (Word num_workers, std::function<void ()> panic, std::function<void ()> init, std::function<void ()> cleanup)
		:	panic(panic),
			stop(false),
			startup_complete(false),
			init_count(0),
			timer(Timer::CreateAndStart()),
			init(init),
			cleanup(cleanup)
	{
	
		//	Initialize atomics
		running=0;
	
		//	Allocate space for workers
		workers=decltype(workers)(num_workers);
		
		Word i=0;
		try {
	
			//	Start workers
			for (;i<num_workers;++i) {
			
				workers.EmplaceBack();
				
				workers[i].Item<0>()=0;
				workers[i].Item<1>()=0;
				workers[i].Item<2>()=0;
				workers[i].Item<3>()=Thread([=] () {	worker_func(i);	});
			
			}
			
		} catch (...) {
		
			//	KILL
			queue_lock.Acquire();
			stop=true;
			startup_complete=true;
			queue_wait.WakeAll();
			queue_lock.Release();
			
			for (Word n=0;n<i;++n) {
			
				workers[n].Item<3>().Join();
			
			}
			
			throw;
		
		}
		
		//	Tell workers to begin
		queue_lock.Acquire();
		startup_complete=true;
		queue_wait.WakeAll();
		//	Wait on initialization
		while (init_count!=workers.Count()) queue_wait.Sleep(queue_lock);
		queue_lock.Release();
		
		//	Initialization failed
		if (stop) {
		
			//	Wait for workers to end
			for (i=0;i<num_workers;++i) workers[i].Item<3>().Join();
			
			//	Throw
			throw std::runtime_error(init_error);
			
		}
	
	}
	
	
	ThreadPool::~ThreadPool () noexcept {
	
		//	Notify threads that it's time
		//	to shut down
		queue_lock.Acquire();
		stop=true;
		queue_wait.WakeAll();
		queue_lock.Release();
		
		//	Wait for all threads to end
		for (auto & t : workers) t.Item<3>().Join();
		
		//	End all pending tasks
		for (auto & t : queue) {
		
			auto & handle=t.Item<0>();
			
			handle->lock.Acquire();
			handle->status=static_cast<Word>(ThreadPoolStatus::Error);
			handle->wait.WakeAll();
			handle->lock.Release();
		
		}
		
		//	End all scheduled tasks
		for (auto & t : scheduled) {
		
			auto & handle=t.Item<0>();
			
			handle->lock.Acquire();
			handle->status=static_cast<Word>(ThreadPoolStatus::Error);
			handle->wait.WakeAll();
			handle->lock.Release();
		
		}
	
	}
	
	
	ThreadPoolWorkerInfo::ThreadPoolWorkerInfo (UInt64 running, UInt64 task_count, UInt64 failed) noexcept
		:	Running(running), TaskCount(task_count), Failed(failed)
	{	}
	
	
	ThreadPoolInfo ThreadPool::GetInfo () const {
	
		ThreadPoolInfo info;
		
		info.WorkerInfo=Vector<ThreadPoolWorkerInfo>(workers.Count());
		
		for (auto & t : workers) {
		
			info.WorkerInfo.EmplaceBack(
				static_cast<UInt64>(t.Item<0>()),
				static_cast<UInt64>(t.Item<1>()),
				static_cast<UInt64>(t.Item<2>())
			);
		
		}
		
		info.Running=running;
		
		queue_lock.Acquire();
		info.Queued=queue.Count();
		info.Scheduled=scheduled.Count();
		info.ElapsedNanoseconds=timer.ElapsedNanoseconds();
		queue_lock.Release();
		
		return info;
	
	}
	
	
	Word ThreadPool::Count () const noexcept {
	
		return workers.Count();
	
	}


}

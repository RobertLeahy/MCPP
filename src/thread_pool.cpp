#include <thread_pool.hpp>
#include <cstdlib>


namespace MCPP {


	static const char * thread_pool_error="Thread pool shutdown";
	
	
	const char * ThreadPoolError::what () const noexcept {
	
		return thread_pool_error;
	
	}


	ThreadPool::Worker::Worker () noexcept {
	
		Running=0;
		TaskCount=0;
		Failed=0;
	
	}


	ThreadPool::Task::~Task () noexcept {	}
	
	
	void ThreadPool::do_panic () noexcept {
	
		//	Invoke panic callback if applicable
		if (panic) try {
		
			panic(std::current_exception());
		
		} catch (...) {	}
		
		//	We're panicking -- we can't do anything
		//	sane if this function returns, therefore...
		std::abort();
	
	}
	
	
	void ThreadPool::do_cleanup () noexcept {
	
		if (cleanup) try {
		
			cleanup();
		
		} catch (...) {	}
	
	}
	
	
	bool ThreadPool::thread_startup () noexcept {
	
		//	Notify constructor and wait for other
		//	threads
		return lock.Execute([&] () mutable {
		
			//	If another thread throw, fail immediately
			if (ex) return false;
			
			//	If this is the last thread, wake the other
			//	threads
			auto num=workers.Count()+1;	//	Add one for scheduler
			if ((++started)==num) {
			
				wait.WakeAll();
				
				return true;
			
			}
			
			//	We're not the last thread, we have to wait
			//	for all other threads to complete (or for any
			//	one thread to fail)
			do wait.Sleep(lock);
			while (!(ex || (started==num)));
			
			//	Only proceed if none of the other threads
			//	threw
			return !ex;
		
		});
	
	}
	
	
	bool ThreadPool::should_startup () const noexcept {
	
		return lock.Execute([&] () mutable {	return !stop;	});
	
	}
	
	
	void ThreadPool::worker_func (Word i) noexcept {
	
		//	Confirm startup
		if (!(
			should_startup() &&
			worker_startup()
		)) return;
	
		try {
		
			worker(workers[i]);
		
		} catch (...) {
		
			do_cleanup();
			
			do_panic();
			
			return;
		
		}
		
		do_cleanup();
	
	}
	
	
	bool ThreadPool::worker_startup () noexcept {
	
		//	Invoke init routine if applicable
		if (init) try {
		
			init();
		
		} catch (...) {
		
			//	Tell constructor we failed
			lock.Execute([&] () mutable {
			
				//	Store exception that was thrown, unless
				//	there's already a stored exception
				if (!ex) ex=std::current_exception();
				
				wait.WakeAll();
			
			});
			
			//	Fail out
			return false;
		
		}
		
		//	Wait for other threads
		if (!thread_startup()) {
		
			do_cleanup();
			
			return false;
		
		}
		
		return true;
	
	}
	
	
	void ThreadPool::worker (Worker & self) {
	
		//	Loop until told to stop
		for (;;) {
		
			//	Lock and check
			auto ptr=lock.Execute([&] () mutable {
			
				//	Wait for something to happen
				while (!stop && (queue.Count()==0)) wait.Sleep(lock);
				
				//	If we should stop, do so at once
				if (stop) return std::unique_ptr<Task>();
				
				//	Otherwise there's a task to attend to
				auto retr=std::move(queue[0]);
				queue.Delete(0);
				return retr;
			
			});
			
			//	If a null pointer was returned, that
			//	means the stop command was given
			if (!ptr) break;
			
			//	Execute the task
			Timer timer=Timer::CreateAndStart();
			++running;
			if (!(*ptr)()) ++self.Failed;
			--running;
			self.Running+=timer.ElapsedNanoseconds();
			++self.TaskCount;
		
		}
	
	}
	
	
	void ThreadPool::scheduler_func () noexcept {
	
		//	Confirm startup
		if (!(
			should_startup() &&
			thread_startup()
		)) return;
	
		try {
		
			scheduler_inner();
		
		} catch (...) {
		
			do_panic();
			
		}
	
	}
	
	
	void ThreadPool::scheduler_inner () {
	
		//	Loop until told to stop
		for (;;) {
		
			auto task=scheduled_lock.Execute([&] () mutable {
			
				//	Wait for there to be something to do
				while (!scheduler_stop && (scheduled.Count()==0)) scheduled_wait.Sleep(scheduled_lock);
				
				//	If we're to stop, do so at once
				if (scheduler_stop) return ScheduledTask{};
				
				//	Wait until it's time to dequeue the first
				//	task
				UInt64 elapsed;
				while (
					!scheduler_stop &&
					((elapsed=timer.ElapsedMilliseconds())<scheduled[0].When)
				) scheduled_wait.Sleep(
					scheduled_lock,
					scheduled[0].When-elapsed
				);
				
				//	If we're to stop, do it at once
				if (scheduler_stop) return ScheduledTask{};
				
				//	It's time
				auto retr=std::move(scheduled[0]);
				scheduled.Delete(0);
				return retr;
				
			});
			
			//	If no task was dequeued, we stop
			//	immediately
			if (!task.What) break;
			
			//	Enqueue the task
			lock.Execute([&] () mutable {
			
				queue.Add(std::move(task.What));
				
				wait.Wake();
			
			});
		
		}
	
	}
	
	
	ThreadPool::ThreadPool (
		Word num_workers,
		std::function<void (std::exception_ptr)> panic,
		std::function<void ()> init,
		std::function<void ()> cleanup
	)	:	timer(Timer::CreateAndStart()),
			started(0),
			stop(false),
			scheduler_stop(false),
			init(std::move(init)),
			cleanup(std::move(cleanup)),
			panic(std::move(panic))
	{
	
		//	Initialize statistic
		running=0;
		
		//	Normalize worker count
		if (num_workers==0) num_workers=1;
		else if (num_workers==std::numeric_limits<Word>::max()) --num_workers;
		
		//	Allocate enough space for control blocks
		workers=Vector<Worker>(num_workers);
		
		//	Create control blocks
		for (Word i=0;i<num_workers;++i) workers.EmplaceBack();
		
		//	Spawn threads
		Word i=0;
		bool s=false;
		try {
		
			//	Lock prevents threads from checking stop
			//	variable until every single thread has been
			//	spawned
			lock.Execute([&] () mutable {
			
				try {
			
					//	Spawn workers
					for (;i<num_workers;++i) workers[i].T=Thread([this,i] () mutable {	worker_func(i);	});
					
					//	Spawn scheduler
					scheduler=Thread([this] () mutable {	scheduler_func();	});
					s=true;
					
				} catch (...) {
				
					//	Give shutdown command
					stop=true;
					
					throw;
				
				}
				
				//	Wait
				while (!(ex || (started==(num_workers+1)))) wait.Sleep(lock);
				
				//	Check and rethrow if worker encountered
				//	error
				if (ex) std::rethrow_exception(std::move(ex));
			
			});
		
		} catch (...) {
		
			//	Wait for threads to exit
			if (s) scheduler.Join();
			while ((i--)>0) workers[i].T.Join();
			
			//	Rethrow
			throw;
		
		}
	
	}
	
	
	ThreadPool::~ThreadPool () noexcept {
	
		//	Issue shutdown commands
		
		lock.Execute([&] () mutable {
		
			stop=true;
			
			wait.WakeAll();
		
		});
		
		scheduled_lock.Execute([&] () mutable {
		
			scheduler_stop=true;
			
			scheduled_wait.WakeAll();
		
		});
		
		//	Wait for workers to shutdown
		for (auto & control : workers) control.T.Join();
		scheduler.Join();
	
	}
	
	
	ThreadPoolInfo ThreadPool::GetInfo () const {
	
		ThreadPoolInfo retr;
		retr.Running=running;
		
		lock.Execute([&] () {	retr.Queued=queue.Count();	});
		
		scheduled_lock.Execute([&] () {
		
			retr.Scheduled=scheduled.Count();
			
			retr.ElapsedNanoseconds=timer.ElapsedNanoseconds();
		
		});
		
		for (auto & control : workers) retr.WorkerInfo.Add(
			ThreadPoolWorkerInfo{
				control.Running,
				control.TaskCount,
				control.Failed
			}
		);
		
		return retr;
	
	}
	
	
	Word ThreadPool::Count () const noexcept {
	
		return workers.Count();
	
	}


}

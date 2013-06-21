/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <functional>
#include <type_traits>
#include <limits>
#include <new>
#include <utility>


namespace MCPP {


	/**
	 *	\cond
	 */
	 
	 
	class ThreadPool;
	
	
	/**
	 *	\endcond
	 */
	 
	 
	/**
	 *
	 */
	enum class ThreadPoolStatus : Word {
	
		Waiting,	/**<	The task was scheduled for a future time which has not yet arrived.*/
		Queued,		/**<	The task is ready to run waiting for a thread in the pool to be available.	*/
		Running,	/**<	The task is running on a thread in the thread pool.	*/
		Success,	/**<	The task has completed successfully.	*/
		Error		/**<	The task has completed unsuccessfully.	*/
	
	};
	 
	 
	/**
	 *	A handle upon which a thread pool
	 *	task may be waited, and from which
	 *	the result of that task may be
	 *	retrieved.
	 */
	class ThreadPoolHandle {
	
	
		friend class ThreadPool;
	
	
		private:
		
		
			//	The lock which protects
			//	the non-atomic elements of
			//	the handle
			mutable Mutex lock;
			//	A condvar upon which observers
			//	may wait for state changes
			mutable CondVar wait;
			//	A timer to be used in tracking
			//	the amount of time this task's
			//	various stages take
			mutable Timer timer;
			//	A pointer to the result, or
			//	the null pointer if the task
			//	has not yet completed
			void * result;
			//	The function which shall be called
			//	to clear up the result pointer
			std::function<void (void *)> cleanup;
			//	The amount of time the thread spent
			//	queued and ready to run
			std::atomic<UInt64> queued;
			//	The amount of time the thread spent
			//	actually running
			std::atomic<UInt64> running;
			//	The status of the task this
			//	handle represents
			std::atomic<Word> status;
			
		
		public:
		
		
			/**
			 *	\cond
			 */
			 
			 
			ThreadPoolHandle () noexcept;
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	Cleans up the thread pool handle.
			 */
			~ThreadPoolHandle () noexcept;
			
			
			/**
			 *	Retrieves the status of the underlying
			 *	task.
			 *
			 *	\return
			 *		The status of the task.  If this
			 *		status is not ThreadPoolStatus::Success
			 *		or ThreadPoolStatus::Error it is
			 *		liable to change at anytime, perhaps
			 *		even before this function returns.
			 */
			ThreadPoolStatus Status () const noexcept;
			/**
			 *	Determines whether the task has completed
			 *	or not.
			 *
			 *	\return
			 *		\em true if the task is finished,
			 *		\em false otherwise.
			 */
			bool Completed () const noexcept;
			/**
			 *	Determines whether the task has finished
			 *	successfully or not.
			 *
			 *	\return
			 *		\em true if the task finished successfully,
			 *		\em false otherwise.
			 */
			bool Success () const noexcept;
			/**
			 *	Determines the number of nanoseconds the
			 *	task spent or has spent queued.
			 *
			 *	\return
			 *		The amount of time the thread has been
			 *		or was queued in nanoseconds.
			 */
			UInt64 Queued () const noexcept;
			/**
			 *	Determines the number of nanoseconds the
			 *	task spent or has spent running.
			 *
			 *	\return
			 *		The amount of time the thread has been
			 *		or was running in nanoseconds.
			 */
			UInt64 Running () const noexcept;
			/**
			 *	Waits until the task completes.
			 *
			 *	When this function returns the task
			 *	is guaranteed to have either succeeded
			 *	or failed.
			 */
			void Wait () const noexcept;
			/**
			 *	Fetches the result of the task.
			 *
			 *	Calling this function when the task has
			 *	not completed will cause it to wait
			 *	until the task has completed.
			 *
			 *	If the task failed this shall return
			 *	\em nullptr.
			 *
			 *	\tparam T
			 *		The type the caller expects the
			 *		result to be.
			 *
			 *	\return
			 *		A pointer to the result.
			 */
			template <typename T>
			T * Result () noexcept {
			
				Wait();
				
				return reinterpret_cast<T *>(result);
			
			}
			/**
			 *	Fetches the result of the task.
			 *
			 *	Calling this function when the task has
			 *	not completed will cause it to wait
			 *	until the task has completed.
			 *
			 *	If the task failed this shall return
			 *	\em nullptr.
			 *
			 *	\tparam T
			 *		The type the caller expects the
			 *		result to be.
			 *
			 *	\return
			 *		A pointer to the result.
			 */
			template <typename T>
			const T * Result () const noexcept {
			
				Wait();
				
				return reinterpret_cast<const T *>(result);
			
			}
	
	
	};
	
	
	/**
	 *	Contains information about a particular
	 *	thread pool worker at a particular moment
	 *	in time.
	 */
	class ThreadPoolWorkerInfo {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			ThreadPoolWorkerInfo (UInt64, UInt64, UInt64) noexcept;
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	The number of nanoseconds this
			 *	particular thread has spent running
			 *	tasks.
			 */
			UInt64 Running;
			/**
			 *	The number of tasks this particular
			 *	thread has executed.
			 */
			UInt64 TaskCount;
			/**
			 *	The number of tasks this particular
			 *	thread has executed which failed.
			 */
			UInt64 Failed;
	
	
	};
	
	
	/**
	 *	Contains a snapshot of information about
	 *	a thread pool at a specific moment in
	 *	time.
	 */
	class ThreadPoolInfo {
	
	
		public:
		
		
			/**
			 *	The number of workers running
			 *	a task at the moment this
			 *	structure was generated.
			 */
			Word Running;
			/**
			 *	The number of tasks queued
			 *	at the moment this structure
			 *	was generated.
			 */
			Word Queued;
			/**
			 *	The number of tasks scheduled
			 *	for execution at some point
			 *	in the future at the moment
			 *	this structure was generated.
			 */
			Word Scheduled;
			/**
			 *	The number of nanoseconds since
			 *	the thread pool was started.
			 */
			UInt64 ElapsedNanoseconds;
			/**
			 *	Information about each of the workers
			 *	in the thread pool
			 */
			Vector<ThreadPoolWorkerInfo> WorkerInfo;
	
	
	};
	
	
	/**
	 *	A pool of threads which may be used
	 *	to execute asynchronous tasks.
	 */
	class ThreadPool {
	
	
		private:
		
		
			//	A function which will be invoked
			//	if something goes seriously wrong
			//	in the worker threads
			std::function<void ()> panic;
			
			
			//	When the pool must shut down, this
			//	flag will be set.
			//
			//	Then each of the threads shall be woken
			//	up, see the flag set, and end in turn.
			bool stop;
			
			
			//	During startup the threads must wait
			//	to begin until we're sure all threads
			//	have started.
			//
			//	To establish this, they watch this flag
			bool startup_complete;
			
			
			//	After they initially start up threads
			//	perform their initialization functions
			//	and increment this count as they do so
			Word init_count;
			
			
			//	This timer counts the number of nanoseconds
			//	since the pool was started
			//
			//	It is guarded by the queue lock
			mutable Timer timer;
			
			
			//	Workers
			
			//	The pool
			Vector<
				Tuple<
					//	The number of nanoseconds
					//	this particular thread has
					//	spent running
					std::atomic<UInt64>,
					//	The number of tasks this
					//	particular thread has executed
					std::atomic<UInt64>,
					//	The number of tasks this
					//	particular thread has executed
					//	which have failed
					std::atomic<UInt64>,
					//	The thread itself
					Thread
				>
			> workers;
			
			//	The number of workers currently running
			std::atomic<Word> running;
			
			//	Queued tasks
			//
			//	A vector was chosen as the queue
			//	is expected to be relatively short,
			//	and since it does not have the overhead
			//	of constant memory allocations/deallocations
			Vector<
				Tuple<
					//	The task's handle
					SmartPointer<ThreadPoolHandle>,
					//	The task
					std::function<void ()>
				>
			> queue;
			//	Lock which ensures
			//	mutually exclusive
			//	access to the queue
			mutable Mutex queue_lock;
			//	The condvar on which
			//	workers wait while they
			//	have no task to execute
			CondVar queue_wait;
			
			//	Scheduled tasks
			Vector<
				Tuple<
					//	The task's handle
					SmartPointer<ThreadPoolHandle>,
					//	When the task is to be
					//	executed
					UInt64,
					//	The task
					std::function<void ()>
				>
			> scheduled;
			
			
			//	Cleanup and init functions
			
			std::function<void ()> init;
			std::function<void ()> cleanup;
			
			
			//	Worker function
			
			void worker_func (Word) noexcept;
			
			
			//	Wraps callbacks which
			//	return void
			template <typename T, typename... Args>
			static auto wrap (
				SmartPointer<ThreadPoolHandle> & handle,
				T && func,
				Args &&... args
			) -> typename std::enable_if<
				std::is_same<
					decltype(func(std::forward<Args>(args)...)),
					void
				>::value,
				std::function<void ()>
			>::type {
			
				return std::bind(
					std::forward<T>(func),
					std::forward<Args>(args)...
				);
			
			}
			
			
			template <typename return_type, typename T, typename... Args>
			static inline std::function<void ()> wrap_impl (
				SmartPointer<ThreadPoolHandle> & handle,
				T && func,
				Args &&... args
			) {
			
				//	Cleanup function
				handle->cleanup=[] (void * ptr) {
				
					if (ptr!=nullptr) {
					
						//	Destroy the object
						reinterpret_cast<return_type *>(ptr)->~return_type();
						
						//	Clean up the memory
						Memory::Free(ptr);
					
					}
				
				};
				
				//	Create an invocable target
				//	that takes no arguments
				std::function<return_type ()> bound(
					std::bind(
						std::forward<T>(func),
						std::forward<Args>(args)...
					)
				);
				//	If we capture the smart pointer
				//	itself then we'll have that
				//	object with a reference to itself
				//	so it'll never get clean up
				void ** ptr=&(handle->result);
				
				//	Build a function that wraps that
				return [=] () {
				
					//	Create somewhere for the
					//	object returned to live
					*ptr=Memory::Allocate<return_type>();
					
					//	Place the returned object there.
					//
					//	This will invoke move semantics
					//	so this will work even for objects
					//	which cannot be copied, also this
					//	avoids default construction and
					//	move assignment, so literally the
					//	only requirement is move constructibility,
					//	which is required to return from
					//	a function anyway.
					new (*ptr) return_type (bound());
				
				};
			
			}
			
			
			//	Wraps callbacks which return
			//	not-void
			template <typename T, typename... Args>
			static auto wrap (
				SmartPointer<ThreadPoolHandle> & handle,
				T && func,
				Args &&... args
			) -> typename std::enable_if<
				!std::is_same<
					decltype(func(std::forward<Args>(args)...)),
					void
				>::value,
				std::function<void ()>
			>::type {
			
				//	GCC segfaults if a typedef this, and GCC
				//	doesn't allow parameter packs to be captured
				//	in a lambda, so here's a workaround...
				return wrap_impl<
					decltype(func(std::forward<Args>(args)...))
				>(
					handle,
					std::forward<T>(func),
					std::forward<Args>(args)...
				);
				
			}
			
			
		public:
		
		
			ThreadPool () = delete;
			ThreadPool (const ThreadPool &) = delete;
			ThreadPool (ThreadPool &&) = delete;
			ThreadPool & operator = (const ThreadPool &) = delete;
			ThreadPool & operator = (ThreadPool &&) = delete;
		
		
			/**
			 *	Creates a new thread pool.
			 *
			 *	\param [in] num_workers
			 *		The number of worker threads the
			 *		pool shall spawn.
			 *	\param [in] panic
			 *		A function which shall be called
			 *		if something goes wrong in one of
			 *		the worker threads.
			 */
			ThreadPool (
				Word num_workers,
				std::function<void ()> panic=std::function<void ()>(),
				std::function<void ()> init=std::function<void ()>(),
				std::function<void ()> cleanup=std::function<void ()>()
			);
			/**
			 *	Cleans up the thread pool, stopping all
			 *	workers.
			 */
			~ThreadPool () noexcept;
			
			
			/**
			 *	Enqueues a task for execution in the thread
			 *	pool.
			 *
			 *	\tparam T
			 *		The type of the callback to be executed.
			 *	\tparam Args
			 *		The types of the arguments to be passed
			 *		to the callback of type \em T.
			 *
			 *	\param [in] func
			 *		A callback of type \em T which shall be
			 *		invoked in a thread pool worker thread.
			 *	\param [in,out] args
			 *		The arguments which shall be passed to
			 *		\em func when it is invoked.
			 *
			 *	\return
			 *		A handle which may be used to monitor
			 *		the task and to retrieve its result.
			 */
			template <typename T, typename... Args>
			auto Enqueue (T && func, Args &&... args) -> typename std::enable_if<
				!std::numeric_limits<
					typename std::decay<T>::type
				>::is_integer,
				SmartPointer<ThreadPoolHandle>
			>::type {
			
				//	Create a handle
				auto handle=SmartPointer<ThreadPoolHandle>::Make();
				
				//	Initialize
				handle->status=static_cast<Word>(ThreadPoolStatus::Queued);
				handle->timer=Timer::CreateAndStart();
				
				//	Add to queue
				queue_lock.Acquire();
				
				try {
				
					queue.EmplaceBack(
						handle,
						wrap(
							handle,
							std::forward<T>(func),
							std::forward<Args>(args)...
						)
					);
					
					//	Wake up a worker
					queue_wait.Wake();
				
				} catch (...) {
				
					queue_lock.Release();
					
					throw;
				
				}
				
				queue_lock.Release();
				
				return handle;
			
			}
			
			
			template <typename T, typename... Args>
			SmartPointer<ThreadPoolHandle> Enqueue (Word milliseconds, T && func, Args &&... args) {
			
				//	If it's less than a milliseconds away
				//	just enqueue it
				if (milliseconds==0) return Enqueue(
					std::forward<T>(func),
					std::forward<Args>(args)...
				);
			
				//	Create a handle
				auto handle=SmartPointer<ThreadPoolHandle>::Make();
				
				//	Initialize
				handle->status=static_cast<Word>(ThreadPoolStatus::Waiting);
				
				//	Add to scheduled tasks
				queue_lock.Acquire();
				
				try {
				
					scheduled.EmplaceBack(
						handle,
						timer.ElapsedNanoseconds()+(static_cast<UInt64>(milliseconds)*1000000),
						wrap(
							handle,
							std::forward<T>(func),
							std::forward<Args>(args)...
						)
					);
					
					//	Wake up a worker
					queue_wait.Wake();
				
				} catch (...) {
				
					queue_lock.Release();
					
					throw;
				
				}
				
				queue_lock.Release();
				
				return handle;
			
			}
			
			
			/**
			 *	Retrieves information about this thread
			 *	pool.
			 *
			 *	\return
			 *		A ThreadPoolInfo object populated with
			 *		information about this thread pool that
			 *		was current as of the invocation.
			 */
			ThreadPoolInfo GetInfo () const;
	
	
	};


}

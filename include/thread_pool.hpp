/**
 *	\file
 */
 
 
#pragma once


#include <functional>
#include <stdexcept>


/**
 *	\cond
 */


namespace MCPP {


	class ThreadPool;
	class ThreadPoolHandle;
	
	
	/**
	 *	A callback which may optionally be passed
	 *	to the constructor of a thread pool and
	 *	shall be called inside all threads before
	 *	they start executing callbacks.
	 */
	typedef std::function <void ()> ThreadPoolStartup;
	/**
	 *	A callback which may optionally be passed
	 *	to the constructor of a thread pool and
	 *	shall be called inside all threads before
	 *	they shutdown.
	 */
	typedef std::function <void ()> ThreadPoolShutdown;


}


/**
 *	\endcond
 */


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	A handle to a task executing in a
	 *	thread pool.  Allows the task's
	 *	execution to be tracked, and the
	 *	result to be retrieved.
	 */
	class ThreadPoolHandle {
	
	
		friend class ThreadPool;
	
		
		private:
		
		
			Mutex lock;
			CondVar wait;
			bool done;
			bool success;
			void * ptr;
			std::function<void (void *)> cleanup;
			
			
		public:
		
		
			ThreadPoolHandle (const ThreadPoolHandle &) = delete;
			ThreadPoolHandle (ThreadPoolHandle &&) = delete;
			ThreadPoolHandle & operator = (const ThreadPoolHandle &) = delete;
			ThreadPoolHandle & operator = (ThreadPoolHandle &&) = delete;
		
		
			/**
			 *	Creates a new thread pool handle.
			 */
			ThreadPoolHandle () noexcept;
			
			
			/**
			 *	Cleans up a thread pool handle.
			 */
			~ThreadPoolHandle () noexcept;
			
			
			/**
			 *	Determines whether the task is completed.
			 *
			 *	\return
			 *		\em true if the task in complete,
			 *		\em false otherwise.
			 */
			bool Completed () noexcept;
			/**
			 *	Determines whether the task succeeded.
			 *
			 *	\return
			 *		\em true if the task completed without
			 *		throwing an exception, \em false otherwise.
			 */
			bool Success () noexcept;
			/**
			 *	Fetches the result.
			 *
			 *	Calling this function when Completed has
			 *	not returned \em true is unsafe.
			 *
			 *	\tparam T
			 *		The type the caller expects the
			 *		result to be.
			 *
			 *	\return
			 *		A pointer to the result.
			 */
			template <typename T>
			T * Result () noexcept;
			/**
			 *	Waits for the task to complete.
			 */
			void Wait () noexcept;
		
	
	};
	
	
	template <typename T>
	T * ThreadPoolHandle::Result () noexcept {
	
		return reinterpret_cast<T *>(ptr);
	
	}


	
	class ThreadPool {
	
	
		public:
		
		
			/**
			 *	The type of callback the thread pool
			 *	expects.
			 */
			typedef std::function<void (void **, std::function<void (void *)> *)> CallbackType;
	
	
		private:
		
		
			//	Thread pool
			Vector<Thread> pool;
			
			
			//	Task queue
			Vector<
				Tuple<
					CallbackType,
					SmartPointer<ThreadPoolHandle>
				>
			> queue;
			Mutex queue_lock;
			CondVar queue_sleep;
			CondVar queue_wait;
			
			
			//	Startup/shutdown
			ThreadPoolStartup startup;
			ThreadPoolShutdown shutdown;
			
			
			//	Pool control
			volatile bool stop;
			volatile bool begin;
			Nullable<Barrier> sync;
			
			
			static void pool_func (void *) noexcept;
			void pool_func_impl () noexcept;
			
			
			template <typename T>
			static
			typename std::enable_if<!std::is_same<T,void>::value,CallbackType>::type
			wrap_factory (
				std::function<T ()> && func
			) {
			
				return [func] (void ** ptr, std::function<void (void *)> * cleanup) -> void {
				
					*cleanup=[] (void * ptr) -> void {
					
						reinterpret_cast<T *>(ptr)->~T();
						
						Memory::Free(ptr);
					
					};
					
					*ptr=Memory::Allocate<T>();
					
					try {
					
						new (*ptr) T (func());
					
					} catch (...) {
					
						Memory::Free(*ptr);
						
						*ptr=nullptr;
						
						throw;
					
					}
				
				};
			
			}
			
			
			template <typename T>
			static
			typename std::enable_if<std::is_same<T,void>::value,CallbackType>::type
			wrap_factory (
				std::function<T ()> && func
			) {
			
				return [func] (void ** ptr, std::function<void (void *)> *) -> void {	func();	};
			
			}
			
			
		public:
		
		
			/**
			 *	Wraps an arbitrary function with arbitrary
			 *	arguments such that it may be executed by
			 *	a thread pool.
			 *
			 *	\tparam T
			 *		The type of the invocable target.
			 *	\tparam Args
			 *		The types of the arguments to forward
			 *		to \em func.
			 *
			 *	\param [in] func
			 *		An invocable target which shall be called
			 *		in the thread pool.
			 *	\param [in] args
			 *		The arguments to pass to \em func.
			 *
			 *	\return
			 *		An object suitable to be passed to
			 *		ThreadPool::Enqueue.
			 */
			template <typename T, typename... Args>
			static CallbackType Wrap (T && func, Args &&... args) {
			
				return wrap_factory<decltype(func(std::forward<Args>(args)...))>(
					std::bind(
						std::forward<T>(func),
						std::forward<Args>(args)...
					)
				);
			
			}
		
		
			ThreadPool () = delete;
			ThreadPool (const ThreadPool &) = delete;
			ThreadPool (ThreadPool &&) = delete;
			ThreadPool & operator = (const ThreadPool &) = delete;
			ThreadPool & operator = (ThreadPool &&) = delete;
			
			
			/**
			 *	Creates a thread pool with a given number
			 *	of workers.
			 *
			 *	\param [in] num
			 *		The number of workers the thread pool
			 *		shall have.
			 *	\param [in] startup
			 *		A callback function to be called before
			 *		each worker begins work.  This shall be
			 *		guaranteed to have been called in
			 *		each thread before the constructor returns,
			 *		if it fails (i.e.\ throws an exception) the
			 *		constructor is guaranteed to throw, each
			 *		worker thread is guaranteed to have been
			 *		shutdown, and all supplied \em shutdown
			 *		functions are guaranteed to be called
			 *		in threads where \em startup did not
			 *		fail (i.e.\ throw).
			 *	\param [in] shutdown
			 *		A callback function to be called before
			 *		each worker shuts down.
			 */
			ThreadPool (Word num, const ThreadPoolStartup & startup=ThreadPoolStartup(), const ThreadPoolShutdown & shutdown=ThreadPoolShutdown());
			/**
			 *	Shuts down a thread pool.
			 */
			~ThreadPool () noexcept;
			
			
			/**
			 *	Enqueues a task for the thread pool
			 *	to execute.
			 *
			 *	\param [in] func
			 *		The task that the thread pool shall
			 *		execute.
			 *
			 *	\return
			 *		A ThreadPoolHandle which allows
			 *		the task to be waited on and
			 *		monitored.
			 */
			SmartPointer<ThreadPoolHandle> Enqueue (const CallbackType & func);
			/**
			 *	Enqueues a task for the thread pool
			 *	to execute.
			 *
			 *	\tparam T
			 *		The type of the invocable target.
			 *	\tparam Args
			 *		The types of the arguments to forward
			 *		to \em func.
			 *
			 *	\param [in] func
			 *		An invocable target which shall be called
			 *		in the thread pool.
			 *	\param [in] args
			 *		The arguments to pass to \em func.
			 *
			 *	\return
			 *		A ThreadPoolHandle which allows
			 *		the task to be waited on and
			 *		monitored.
			 */
			template <typename T, typename... Args>
			auto Enqueue (T && func, Args &&... args) -> typename std::enable_if<
				!std::is_same<
					CallbackType,
					typename std::remove_reference<
						typename std::decay<T>::type
					>::type
				>::value,
				SmartPointer<ThreadPoolHandle>
			>::type {
			
				return Enqueue(
					Wrap(
						std::forward<T>(func),
						std::forward<Args>(args)...
					)
				);
			
			}
	
	
	};


}

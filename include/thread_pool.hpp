/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <promise.hpp>
#include <safeint.hpp>
#include <atomic>
#include <exception>
#include <functional>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>


namespace MCPP {


	/**
	 *	Thrown from a promise that could not be fulfilled
	 *	because the thread pool running the task shutdown.
	 */
	class ThreadPoolError : public std::exception {
	
		
		public:
		
		
			virtual const char * what () const noexcept override;
		
	
	};
	
	
	/**
	 *	Contains information about a particular
	 *	thread pool worker at a particular moment
	 *	in time.
	 */
	class ThreadPoolWorkerInfo {
	
	
		public:
		
		
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
	 *	Manages a pool of worker threads, using them to
	 *	execute asynchronous tasks.
	 */
	class ThreadPool {
	
	
		private:
		
		
			class Worker {
			
			
				public:
				
				
					std::atomic<UInt64> Running;
					std::atomic<UInt64> TaskCount;
					std::atomic<UInt64> Failed;
					Thread T;
					
					
					Worker () noexcept;
			
			
			};
			
			
			//	Workers
			Vector<Worker> workers;
			std::atomic<Word> running;
			
			
			class Task {
			
			
				public:
				
				
					virtual ~Task () noexcept;
					virtual bool operator () () noexcept = 0;
			
			
			};
			
			
			template <typename T>
			class WrappedTask : public Task {
			
			
				public:
				
				
					typedef std::function<T ()> Type;
			
			
				private:
				
				
					Promise<T> promise;
					Nullable<Type> callback;
					
					
				public:
				
				
					WrappedTask (Type callback) noexcept : callback(std::move(callback)) {	}
					
					
					~WrappedTask () noexcept {
					
						if (!callback.IsNull()) promise.Fail(std::make_exception_ptr(ThreadPoolError{}));
					
					}
					
					
					virtual bool operator () () noexcept override {
					
						bool retr=true;
						try {
						
							promise.Execute([&] () mutable {	return (*callback)();	});
							
						} catch (...) {
						
							retr=false;
						
						}
						
						callback.Destroy();
						
						return retr;
					
					}
					
					
					Promise<T> Get () const noexcept {
					
						return promise;
					
					}
			
			
			};
			
			
			//	Task queue
			Vector<std::unique_ptr<Task>> queue;
			mutable Mutex lock;
			mutable CondVar wait;
			
			
			class ScheduledTask {
			
			
				public:
				
				
					UInt64 When;
					std::unique_ptr<Task> What;
			
			
			};
			
			
			//	Scheduler thread
			Thread scheduler;
			//	Scheduled tasks
			Vector<ScheduledTask> scheduled;
			mutable Mutex scheduled_lock;
			mutable CondVar scheduled_wait;
			mutable Timer timer;
			
			
			//	Coordinates startup and shutdown
			Word started;
			bool stop;
			bool scheduler_stop;
			std::exception_ptr ex;
			
			
			//	Cleanup and init functions
			std::function<void ()> init;
			std::function<void ()> cleanup;
			
			
			//	Panic function
			std::function<void (std::exception_ptr)> panic;
			
			
			template <typename T, typename... Args>
			static auto wrap (T && callback, Args &&... args) -> Tuple<
				std::unique_ptr<Task>,
				Promise<decltype(
					callback(std::forward<Args>(args)...)
				)>
			> {
			
				typedef decltype(callback(std::forward<Args>(args)...)) type;
				
				auto ptr=std::unique_ptr<Task>(
					new WrappedTask<type>(
						std::bind(
							std::forward<T>(callback),
							std::forward<Args>(args)...
						)
					)
				);
				
				auto promise=reinterpret_cast<WrappedTask<type> *>(ptr.get())->Get();
				
				return Tuple<std::unique_ptr<Task>,Promise<type>>(
					std::move(ptr),
					std::move(promise)
				);
			
			}
			
			
			[[noreturn]]
			void do_panic () noexcept;
			void do_cleanup () noexcept;
			bool thread_startup () noexcept;
			bool should_startup () const noexcept;
			
			
			//	Worker functions
			void worker_func (Word) noexcept;
			bool worker_startup () noexcept;
			void worker (Worker &);
			
			
			//	Scheduler functions
			void scheduler_func () noexcept;
			void scheduler_inner ();
			
			
		public:
		
		
			ThreadPool () = delete;
			ThreadPool (const ThreadPool &) = delete;
			ThreadPool (ThreadPool &&) = delete;
			ThreadPool & operator = (const ThreadPool &) = delete;
			ThreadPool & operator = (ThreadPool &&) = delete;
		
		
			/**
			 *	Creates and starts a new thread pool.
			 *
			 *	\param [in] num_workers
			 *		The number of worker threads the thread
			 *		pool shall create and maintain.
			 *	\param [in] panic
			 *		A callback the thread pool shall invoke
			 *		if something goes wrong in one of its
			 *		worker threads.  Optional.  Defaults to
			 *		std::abort.
			 *	\param [in] init
			 *		A function each worker thread shall invoke
			 *		before it begins work.
			 *	\param [in] cleanup
			 *		A function each worker thread shall invoke
			 *		before it shuts down.
			 */
			ThreadPool (
				Word num_workers,
				std::function<void (std::exception_ptr)> panic=std::function<void (std::exception_ptr)>(),
				std::function<void ()> init=std::function<void ()>(),
				std::function<void ()> cleanup=std::function<void ()>()
			);
			/**
			 *	Stops and cleans up a thread pool.
			 */
			~ThreadPool () noexcept;
		
		
			/**
			 *	Dispatches a task to run as soon as possible in
			 *	the thread pool.
			 *
			 *	\param [in] callback
			 *		A callback which shall be invoked by a
			 *		thread pool worker thread.
			 *	\param [in] args
			 *		Arguments which shall be forwarded through
			 *		to \em callback.
			 *
			 *	\return
			 *		A promise of the result of invoking \em callback
			 *		with \em args.
			 */
			template <typename T, typename... Args>
			auto Enqueue (T && callback, Args &&... args) -> typename std::enable_if<
				!std::numeric_limits<
					typename std::decay<T>::type
				>::is_integer,
				Promise<decltype(
					callback(std::forward<Args>(args)...)
				)>
			>::type {
			
				auto t=wrap(
					std::forward<T>(callback),
					std::forward<Args>(args)...
				);
				
				lock.Execute([&] () mutable {
				
					queue.Add(
						std::move(
							t.template Item<0>()
						)
					);
					
					wait.Wake();
					
				});
				
				return std::move(t.template Item<1>());
			
			}
			
			
			/**
			 *	Dispatches a task to run in the thread pool after some
			 *	delay.
			 *
			 *	\param [in] when
			 *		The number of milliseconds that shall be waited from
			 *		this point in time until \em callback is executed in
			 *		the thread pool.
			 *	\param [in] callback
			 *		A callback which shall be invoked by a thread pool
			 *		worker thread.
			 *	\param [in] args
			 *		Arguments which shall be forwarded through to
			 *		\em callback.
			 *
			 *	\return
			 *		A promise of the result of invoking \em callback with
			 *		\em args.
			 */
			template <typename T, typename... Args>
			auto Enqueue (Word when, T && callback, Args &&... args) -> Promise<decltype(
				callback(std::forward<Args>(args)...)
			)> {
			
				auto t=wrap(
					std::forward<T>(callback),
					std::forward<Args>(args)...
				);
				
				scheduled_lock.Execute([&] () mutable {
					
					UInt64 w=static_cast<UInt64>(
						SafeInt<UInt64>(timer.ElapsedMilliseconds())+
						SafeInt<UInt64>(SafeWord(when))
					);
					
					Word i=0;
					for (;i<scheduled.Count();++i) if (w<scheduled[i].When) break;
					scheduled.Emplace(
						i,
						ScheduledTask{
							w,
							std::move(
								t.template Item<0>()
							)
						}
					);
					
					scheduled_wait.WakeAll();
				
				});
				
				return std::move(t.template Item<1>());
			
			}
			
			
			/**
			 *	Gets information and statistics about the thread pool.
			 *
			 *	\return
			 *		A structure containing information and statistics
			 *		about the thread pool.
			 */
			ThreadPoolInfo GetInfo () const;
			
			
			/**
			 *	Determines how many workers the thread pool is maintaining.
			 *
			 *	\return
			 *		The number of worker threads managed by this thread
			 *		pool.
			 */
			Word Count () const noexcept;
	
	
	};


}

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <thread_pool.hpp>
#include <functional>
#include <utility>


namespace MCPP {


	/**
	 *	Manages the dispatch of tasks through
	 *	a thread pool, ensuring that no more
	 *	than a certain number are ever running
	 *	or queued simultaneously.
	 */
	class ConcurrencyManager {
	
	
		private:
		
		
			ThreadPool & pool;
			Word max;
			Vector<std::function<void ()>> pending;
			Word running;
			Mutex lock;
			std::function<void ()> panic;
			
			
			inline void next () noexcept {
			
				lock.Execute([&] () {
				
					if (pending.Count()==0) {
					
						--running;
					
					} else {
					
						std::function<void ()> callback=std::move(pending[0]);
						pending.Delete(0);
					
						try {
						
							pool.Enqueue(std::move(callback));
						
						} catch (...) {
						
							--running;
						
							try {
							
								panic();
							
							} catch (...) {	}
						
						}
					
					}
				
				});
			
			}
		
		
		public:
		
		
			ConcurrencyManager () = delete;
			ConcurrencyManager (const ConcurrencyManager &) = delete;
			ConcurrencyManager (ConcurrencyManager &&) = delete;
			ConcurrencyManager & operator = (const ConcurrencyManager &) = delete;
			ConcurrencyManager & operator = (ConcurrencyManager &&) = delete;
			
			
			/**
			 *	Creates a new concurrency manager.
			 *
			 *	\param [in] pool
			 *		A reference to the thread pool
			 *		that the concurrency manager will
			 *		use to dispatch asynchronous tasks.
			 *	\param [in] max
			 *		The maximum number of simultaneous
			 *		tasks managed by this concurrency
			 *		manager which may be queued or
			 *		running simultaneously.
			 *	\param [in] panic
			 *		A callback which shall be invoked
			 *		if an error occurs in the concurrency
			 *		manager's internals.  Defaults to
			 *		nothing.
			 */
			ConcurrencyManager (ThreadPool & pool, Word max, std::function<void ()> panic=std::function<void ()>()) noexcept;
			
			
			/**
			 *	Enqueues a task to be run at the next
			 *	opportunity.
			 *
			 *	Tasks are dispatched in the order that
			 *	they are enqueued until the maximum
			 *	allowable number are queued or running,
			 *	at which point they remain queued inside
			 *	the concurrency manager until another
			 *	task managed by the concurrency manager
			 *	completes.
			 *
			 *	\tparam T
			 *		The type of callback which shall
			 *		be asynchronously invoked.
			 *	\tparam Args
			 *		The type of arguments to pass through
			 *		to the callback of type \em T.
			 *
			 *	\param [in] callback
			 *		The callback which shall be asynchronously
			 *		invoked.
			 *	\param [in] args
			 *		The arguments to pass through to \em callback.
			 */
			template <typename T, typename... Args>
			void Enqueue (T && callback, Args &&... args) {
			
				auto bound=std::bind(
					std::forward<T>(callback),
					std::forward<Args>(args)...
				);
				
				Tuple<decltype(bound)> t(std::move(bound));
				
				std::function<void ()> wrapped=std::bind(
					[this] (decltype(t) t) mutable {
					
						try {
						
							t.template Item<0>()();
						
						} catch (...) {
						
							next();
							
							throw;
						
						}
						
						next();
					
					},
					std::move(t)
				);
				
				lock.Execute([&] () mutable {
				
					if (running<max) {
					
						pool.Enqueue(std::move(wrapped));
						
						++running;
					
					} else {
					
						pending.Add(std::move(wrapped));
					
					}
				
				});
			
			}
			
			
			/**
			 *	Retrieves the maximum number of tasks
			 *	this concurrency manager will allow to
			 *	execute simultaneously.
			 *
			 *	\return
			 *		The maximum number of simultaneous
			 *		tasks this manager will allow.
			 */
			Word Maximum () const noexcept;
	
	
	};


}

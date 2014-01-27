/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>
#include <scope_guard.hpp>
#include <functional>
#include <memory>
#include <utility>
 
 
namespace MCPP {


	/**
	 *	A thread safe pool of arbitrary resources
	 *	which are created lazily and maintained in
	 *	the pool up to some maximum.
	 *
	 *	\tparam T
	 *		The type of resource the pool will manage.
	 */
	template <typename T>
	class Pool {
	
	
		public:
		
		
			/**
			 *	The type that the creation functor should
			 *	return.
			 */
			typedef std::unique_ptr<T> Type;
			/**
			 *	The type of functor which will be called to
			 *	acquire resources as they're needed.
			 */
			typedef std::function<Type ()> CreateType;
			
			
		private:
		
		
			class Entry {
			
			
				private:
				
				
					Type ptr;
					
					
				public:
					
					
					Type Acquire () noexcept {
					
						return ptr ? std::move(ptr) : Type{};
					
					}
					
					
					bool Release (Type & ptr) noexcept {
					
						if (this->ptr) return false;
						
						this->ptr=std::move(ptr);
						
						return true;
					
					}
					
					
					operator bool () const noexcept {
					
						return static_cast<bool>(ptr);
					
					}
			
			
			};
			
			
			Vector<Entry> vec;
			mutable Mutex lock;
			mutable CondVar wait;
			Word max;
			CreateType create;
			
			
			bool is_max () const noexcept {
			
				if (max==0) return false;
				
				return vec.Count()==max;
			
			}
			
			
			Type acquire () {
			
				auto ptr=lock.Execute([&] () {
				
					Type retr;
					bool first=true;
					do {
					
						if (first) first=false;
						else wait.Sleep(lock);
					
						for (auto & entry : vec) if (retr=entry.Acquire()) return retr;
					
					} while (
						is_max() &&
						!retr
					);
					
					vec.EmplaceBack();
					
					return Type{};
				
				});
				
				if (ptr) return ptr;
				
				try {
				
					ptr=create();
					
				} catch (...) {
				
					lock.Execute([&] () {
					
						for (Word i=0;i<vec.Count();++i) if (!vec[i]) {
						
							vec.Delete(i);
							
							wait.Wake();
							
							break;
						
						}
					
					});
					
					throw;
				
				}
				
				return ptr;
			
			}
			
			
			void release (Type ptr) noexcept {
			
				lock.Execute([&] () {
				
					for (auto & entry : vec) if (entry.Release(ptr)) break;
					
					wait.Wake();
				
				});
			
			}
		
		
		public:
		
		
			class Resource;
			
			
			friend class Resource;
		
		
			/**
			 *	Wraps a pool managed resource in an
			 *	RAII container which will automatically
			 *	release the object back into the pool
			 *	when it goes out of scope.
			 *
			 *	Allowing instances of this class to persist
			 *	beyond the lifetime of the Pool they're
			 *	associated with results in undefined
			 *	behaviour.
			 */
			class Resource {
			
			
				friend class Pool;
			
			
				private:
				
				
					Type ptr;
					Pool * pool;
					
					
					void destroy () {
					
						if (ptr) pool->release(std::move(ptr));
					
					}
					
					
					Resource (Type ptr, Pool & pool) noexcept : ptr(std::move(ptr)), pool(&pool) {	}
					
					
				public:
				
				
					Resource () = delete;
					Resource (const Resource &) = delete;
					Resource & operator = (const Resource &) = delete;
					Resource (Resource &&) = default;
					Resource & operator = (Resource && other) noexcept {
					
						if (&other!=this) {
						
							destroy();
							
							ptr=std::move(other.ptr);
							pool=other.pool;
						
						}
						
						return *this;
					
					}
					
					
					/**
					 *	Releases the resource back to the
					 *	pool.
					 */
					~Resource () noexcept {
					
						destroy();
					
					}
					
					
					/**
					 *	Obtains a reference to the resource.
					 *
					 *	\return
					 *		A reference to the resource.
					 */
					T & Get () noexcept {
					
						return *ptr;
					
					}
					
					
					/**
					 *	Obtains a reference to the resource.
					 *
					 *	\return
					 *		A reference to the resource.
					 */
					operator T & () noexcept {
					
						return *ptr;
					
					}
			
			
			};
		
		
			Pool () = delete;
		
		
			/**
			 *	Creates a new resource pool.
			 *
			 *	\param [in] create
			 *		The functor that will be invoked when
			 *		the pool needs to acquire another
			 *		resource.
			 *	\param [in] max
			 *		The maximum number of resources the
			 *		pool may have at once.  When the pool
			 *		is managing the maximum number of resources,
			 *		and they're all in use, incoming threads
			 *		block until a resource is available for
			 *		them.  Defaults to zero, which is interpreted
			 *		as unlimited.
			 */
			Pool (CreateType create, Word max=0) noexcept : max(max), create(std::move(create)) {	}
		
		
			/**
			 *	Acquires a resource from the pool, performs
			 *	some action, and then releases it to the pool.
			 *
			 *	\tparam Callback
			 *		The type of a functor to invoke.
			 *	\tparam Args
			 *		The types of arguments to forward through
			 *		to the functor of type \em Callback.
			 *
			 *	\param [in] callback
			 *		A callback to invoke while a resource is
			 *		held.  The first argument shall be a
			 *		mutable lvalue reference to a resource of
			 *		type \em T, and any further arguments shall
			 *		be of types \em Args.
			 *	\param [in] args
			 *		Arguments of types \em Args to forward through
			 *		to \em callback as its second argument onwards.
			 *
			 *	\return
			 *		Whatever \em callback returned.
			 */
			template <typename Callback, typename... Args>
			auto Execute (Callback && callback, Args &&... args) -> decltype(callback(std::declval<T &>(),std::forward<Args>(args)...)) {
			
				auto ptr=acquire();
				auto guard=AtExit([&] () {	release(std::move(ptr));	});
				
				return callback(*ptr,std::forward<Args>(args)...);
			
			}
			
			
			/**
			 *	Retrieves a resource from the pool, wrapping
			 *	it in an RAII wrapper so that it will automatically
			 *	be released when it goes out of scope.
			 *
			 *	\return
			 *		The resource, wrapped in an object which will
			 *		automatically release it back to the pool when
			 *		it goes out of scope.
			 */
			Resource Get () {
			
				return Resource(acquire(),*this);
			
			}
			
			
			/**
			 *	Determines the number of resources that the
			 *	pool is actually managing.
			 *
			 *	\return
			 *		The number of resources the pool is
			 *		managing.
			 */
			Word Count () const noexcept {
			
				return lock.Execute([&] () {	return vec.Count();	});
			
			}
			
			
			/**
			 *	Determines the maximum number of resources that
			 *	the pool can manage.
			 *
			 *	\return
			 *		The maximum number of resources the pool
			 *		can manage.
			 */
			Word Maximum () const noexcept {
			
				return max;
			
			}
	
	
	};


}
 
/** 
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <exception>
#include <functional>
#include <type_traits>
#include <utility>


namespace MCPP {


	template <typename...>
	class PolicyFunctor;
	
	
	template <typename T, typename... Args>
	class PolicyFunctor<T (Args...)> {
	
	
		public:
		
		
			typedef std::function<T (Args...)> CallbackType;
			typedef std::function<T (CallbackType, Args...)> PolicyType;
			
			
		private:
		
		
			CallbackType callback;
			PolicyType policy;
			
			
		public:
		
		
			PolicyFunctor (PolicyType policy) noexcept : policy(std::move(policy)) {	}
			
			
			T operator () (Args... args) {
			
				if (callback) return policy ? policy(callback,std::move(args)...) : callback(std::move(args)...);
				
				throw std::bad_function_call{};
			
			}
			
			
			bool Set (CallbackType callback) noexcept {
			
				if (this->callback) return false;
				
				this->callback=std::move(callback);
				
				return true;
			
			}
			
			
			operator bool () const noexcept {
			
				return static_cast<bool>(callback);
			
			}
	
	
	};
	
	
	/**
	 *	\cond
	 */
	

	//	TODO: Remove this code when GCC bug with
	//	"sorry, unimplemented: mangling nontype_argument_pack"
	//	is fixed	
	
	
	template <typename>
	class Promise;
	

	namespace PromiseImpl {
	
	
		template <typename TFrom, typename TTo>
		class ThenFunctor {
		
		
			private:
			
			
				Promise<TTo> to;
				std::function<TTo (Promise<TFrom>)> callback;
				
				
			public:
			
			
				ThenFunctor (
					Promise<TTo> to,
					std::function<TTo (Promise<TFrom>)> callback
				) noexcept : to(std::move(to)), callback(std::move(callback)) {	}
				
				
				void operator () (Promise<TFrom> p) {
				
					to.Execute([&] () mutable {	return callback(std::move(p));	});
				
				}
		
		
		};
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Encapsulates a promised, future return
	 *	value, returned instead of the value itself
	 *	from an asynchronous function.
	 *
	 *	\tparam T
	 *		The type of result that the asynchronous
	 *		function will return.
	 */
	template <typename T>
	class Promise {
	
	
		private:
		
		
			class Indirect {
			
			
				public:
				
				
					typedef PolicyFunctor<void (Promise)> PolicyType;
				
				
					mutable Mutex Lock;
					mutable CondVar Wait;
					Nullable<PolicyType> Callback;
					std::exception_ptr Error;
					Nullable<T> Item;
					
					
					Indirect (typename PolicyType::PolicyType policy) noexcept {
					
						Callback.Construct(std::move(policy));
					
					}
			
			
			};
			
			
			SmartPointer<Indirect> i;
			
			
			bool is_done () const noexcept {
			
				return !i->Item.IsNull() || i->Error;
			
			}
			
			
			void fire_callback () noexcept {
			
				auto & callback=i->Callback;
				
				if (
					callback.IsNull() ||
					!(*callback)
				) return;
				
				try {
				
					(*callback)(*this);
					
				} catch (...) {	}
				
				callback.Destroy();
			
			}
			
			
			void complete () noexcept {
			
				fire_callback();
				
				i->Wait.WakeAll();
			
			}
			
			
		public:
		
		
			/**
			 *	The type of the functor which defines the
			 *	execution policy for the promise.
			 */
			typedef typename Indirect::PolicyType::PolicyType PolicyType;
		
		
			/**
			 *	Creates a new promise.
			 *
			 *	\param [in] policy
			 *		A policy which dictates how the
			 *		promise's follow up function (if
			 *		there is one) is executed.  Normally
			 *		the promise' follow up function is
			 *		executed synchronously (if present)
			 *		when the promise is fulfilled.  Passing
			 *		in a policy functor allows a custom
			 *		execution policy to be specified as the
			 *		functor becomes responsible for invoking
			 *		the follow up function however it sees
			 *		fit.  Optional.
			 */
			Promise (PolicyType policy=PolicyType{}) : i(SmartPointer<Indirect>::Make(std::move(policy))) {	}
			
			
			/**
			 *	Retrieves the promised value.
			 *
			 *	Not thread safe.
			 *
			 *	Calling this function before the promise
			 *	has been fulfilled results in undefined
			 *	behaviour.
			 *
			 *	Calling this function multiple times
			 *	results in undefined behaviour.
			 *
			 *	\return
			 *		The promised value.
			 */
			T Get () {
			
				if (i->Error) std::rethrow_exception(std::move(i->Error));
				
				return std::move(*(i->Item));
			
			}
			
			
			/**
			 *	Waits for the promised value.
			 *
			 *	Not thread safe.
			 *
			 *	Calling this function multiple times
			 *	results in undefined behaviour.
			 *
			 *	\return
			 *		The promised value.
			 */
			T Wait () {
			
				i->Lock.Execute([&] () mutable {	while (!is_done()) i->Wait.Sleep(i->Lock);	});
				
				return Get();
			
			}
			
			
			/**
			 *	Completes the promise.
			 *
			 *	Calling this function multiple times results
			 *	in undefined behaviour.
			 *
			 *	\param [in] obj
			 *		The object which fulfilles the promise.
			 */
			void Complete (T obj) noexcept(std::is_nothrow_move_constructible<T>::value) {
			
				i->Lock.Execute([&] () mutable {
				
					i->Item.Construct(std::move(obj));
					
					complete();
				
				});
			
			}
			
			
			/**
			 *	Breaks the promise.
			 *
			 *	Calling this function multiple times results
			 *	in undefined behaviour.
			 *
			 *	\param [in] ex
			 *		An exception_ptr object which points to
			 *		the exception which caused this promise
			 *		to be broken.
			 */
			void Fail (std::exception_ptr ex) noexcept {
			
				i->Lock.Execute([&] () mutable {
				
					i->Error=std::move(ex);
					
					complete();
				
				});
			
			}
			
			
			/**
			 *	Provides a convenient wrapper for asynchronous
			 *	functions.  Invokes an inner function with arbitrary
			 *	arguments of arbitrary types, and then either
			 *	completes the promise with its return value, or
			 *	breaks the promise if it throws.
			 *
			 *	\tparam Callback
			 *		The type of functor to invoke.
			 *	\tparam Args
			 *		The types of arguments to forward through to
			 *		the functor.
			 *
			 *	\param [in] callback
			 *		The functor to invoke.
			 *	\param [in] args
			 *		The arguments to forward through to \em callback.
			 */
			template <typename Callback, typename... Args>
			void Execute (Callback && callback, Args &&... args) noexcept(
				noexcept(callback(std::forward<Args>(args)...))
			) {
			
				try {
				
					Complete(callback(std::forward<Args>(args)...));
					
				} catch (...) {
				
					Fail(std::current_exception());
					
					throw;
				
				}
			
			}
			
			
			/**
			 *	Specifies a function that shall be called in accordance
			 *	with the execution policy once the promise has been
			 *	fulfilled.
			 *
			 *	\tparam Callback
			 *		The type of functor to invoke.
			 *	\tparam Args
			 *		The types of arguments to forward through to
			 *		the functor.
			 *
			 *	\param [in] callback
			 *		The functor to invoke.
			 *	\param [in] args
			 *		The arguments to forward through to \em callback.
			 *
			 *	\return
			 *		A promise for the results of \em callback.
			 */
			template <typename Callback, typename... Args>
			auto Then (Callback && callback, Args &&... args) -> Promise<decltype(
				callback(std::declval<Promise>(),std::forward<Args>(args)...)
			)> {
			
				typedef decltype(callback(*this,std::forward<Args>(args)...)) type;
				
				Promise<type> promise;
				
				i->Lock.Acquire();
				
				try {
				
					if (
						i->Callback.IsNull() ||
						*(i->Callback)
					) throw std::exception{};
					
					i->Callback->Set(
						PromiseImpl::ThenFunctor<
							T,
							type
						>(
							promise,
							std::bind(
								std::forward<Callback>(callback),
								std::placeholders::_1,
								std::forward<Args>(args)...
							)
						)
					);
					
					//	TODO: Use this code when GCC bug with
					//	"sorry, unimplemented: mangling nontype_argument_pack"
					//	is fixed
					/*#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wpedantic"
					i->Callback->Set([
						promise,
						wrapped=std::bind(
							std::forward<Callback>(callback),
							std::placeholders::_1,
							std::forward<Args>(args)...
						)
					] (Promise p) mutable {
					
						promise.Execute(std::move(wrapped),std::move(p));
					
					});
					#pragma GCC diagnostic pop*/
					
					if (is_done()) fire_callback();
				
				} catch (...) {
				
					i->Lock.Release();
					
					throw;
				
				}
				
				i->Lock.Release();
				
				return promise;
			
			}
			
			
	};
	
	
	/**
	 *	\cond
	 */
	
	
	template <>
	class Promise<void> {
	
	
		private:
		
		
			class Indirect {
			
			
				public:
				
				
					typedef PolicyFunctor<void (Promise)> PolicyType;
					
					
					mutable Mutex Lock;
					mutable CondVar Wait;
					Nullable<PolicyType> Callback;
					std::exception_ptr Error;
					bool Done;
					
					
					Indirect (typename PolicyType::PolicyType policy) noexcept : Done(false) {
					
						Callback.Construct(std::move(policy));
					
					}
			
			
			};
			
			
			SmartPointer<Indirect> i;
			
			
			bool is_done () const noexcept {
			
				return i->Done;
			
			}
			
			
			void fire_callback () noexcept {
			
				auto & callback=i->Callback;
				
				if (
					callback.IsNull() ||
					!(*callback)
				) return;
				
				try {
				
					(*callback)(*this);
				
				} catch (...) {	}
				
				callback.Destroy();
			
			}
			
			
			void complete () noexcept {
			
				i->Done=true;
			
				fire_callback();
				
				i->Wait.WakeAll();
			
			}
			
			
		public:
		
		
			typedef typename Indirect::PolicyType::PolicyType PolicyType;
			
			
			Promise (PolicyType policy=PolicyType{}) : i(SmartPointer<Indirect>::Make(std::move(policy))) {	}
			
			
			void Get () {
			
				if (i->Error) std::rethrow_exception(std::move(i->Error));
			
			}
			
			
			void Wait () {
			
				i->Lock.Execute([&] () mutable {	while (!is_done()) i->Wait.Sleep(i->Lock);	});
				
				Get();
			
			}
			
			
			void Complete () noexcept {
			
				i->Lock.Execute([&] () mutable {
				
					i->Done=true;
					
					complete();
				
				});
			
			}
			
			
			void Fail (std::exception_ptr ex) noexcept {
			
				i->Lock.Execute([&] () mutable {
				
					i->Error=std::move(ex);
					
					complete();
				
				});
			
			}
			
			
			template <typename Callback, typename... Args>
			void Execute (Callback && callback, Args &&... args) noexcept(
				noexcept(callback(std::forward<Args>(args)...))
			) {
			
				try {
				
					callback(std::forward<Args>(args)...);
					
					Complete();
					
				} catch (...) {
				
					Fail(std::current_exception());
					
					throw;
				
				}
			
			}
			
			
			template <typename Callback, typename... Args>
			auto Then (Callback && callback, Args &&... args) -> Promise<decltype(
				callback(std::declval<Promise>(),std::forward<Args>(args)...)
			)> {
			
				typedef decltype(callback(*this,std::forward<Args>(args)...)) type;
				
				Promise<type> promise;
				
				i->Lock.Acquire();
				
				try {
				
					if (
						i->Callback.IsNull() ||
						*(i->Callback)
					) throw std::exception{};
					
					i->Callback->Set(
						PromiseImpl::ThenFunctor<
							void,
							type
						>(
							promise,
							std::bind(
								std::forward<Callback>(callback),
								std::placeholders::_1,
								std::forward<Args>(args)...
							)
						)
					);
					
					//	TODO: Use this code when GCC bug with
					//	"sorry, unimplemented: mangling nontype_argument_pack"
					//	is fixed
					/*#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wpedantic"
					i->Callback->Set([
						promise,
						wrapped=std::bind(
							std::forward<Callback>(callback),
							std::placeholders::_1,
							std::forward<Args>(args)...
						)
					] (Promise p) mutable {
					
						promise.Execute(std::move(wrapped),std::move(p));
					
					});
					#pragma GCC diagnostic pop*/
					
					if (is_done()) fire_callback();
				
				} catch (...) {
				
					i->Lock.Release();
					
					throw;
				
				}
				
				i->Lock.Release();
				
				return promise;
			
			}
	
	
	};
	
	
	/**
	 *	\endcond
	 */


}

	
/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <type_traits>
#include <utility>


namespace MCPP {


	/**
	 *	Implements a mutex that may be recursively
	 *	acquired and released.
	 */
	class RecursiveMutex {
	
	
		private:
		
		
			Mutex lock;
			CondVar wait;
			Nullable<decltype(Thread::ID())> id;
			Word depth;
			
			
		public:
		
		
			void Acquire () noexcept;
			void Release () noexcept;
			
			
			template <typename T, typename... Args>
			auto Execute (T && callback, Args &&... args) noexcept(
				noexcept(callback(std::forward<Args>(args)...))
			) -> typename std::enable_if<
				std::is_same<
					decltype(callback(std::forward<Args>(args)...)),
					void
				>::value
			>::type {
			
				Acquire();
				
				try {
				
					callback(std::forward<Args>(args)...);
				
				} catch (...) {
				
					Release();
					
					throw;
				
				}
				
				Release();
			
			}
			
			
			template <typename T, typename... Args>
			auto Execute (T && callback, Args &&... args) noexcept (
				noexcept(callback(std::forward<Args>(args)...)) &&
				std::is_nothrow_move_constructible<
					decltype(callback(std::forward<Args>(args)...))
				>::value
			) -> typename std::enable_if<
				!std::is_same<
					decltype(callback(std::forward<Args>(args)...)),
					void
				>::value,
				decltype(callback(std::forward<Args>(args)...))
			>::type {
			
				Nullable<decltype(callback(std::forward<Args>(args)...))> retr;
				
				Acquire();
				
				try {
				
					retr.Construct(callback(std::forward<Args>(args)...));
				
				} catch (...) {
				
					Release();
					
					throw;
				
				}
				
				Release();
				
				return std::move(*retr);
			
			}
	
	
	};


}

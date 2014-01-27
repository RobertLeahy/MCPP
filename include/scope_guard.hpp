/**
 *	\file
 */


#pragma once


#include <functional>
#include <type_traits>
#include <utility>


namespace MCPP {


	/**
	 *	Executes a certain action when the
	 *	enclosing scope exits.
	 */
	template <typename T>
	class ScopeGuard {
	
		
		private:
			
			
			bool engaged;
			T callback;
			
			
			void destroy () noexcept {
			
				if (engaged) {
					
					try {
						
						callback();
						
					} catch (...) {	}
					
					engaged=false;
					
				}
				
			}
			
			
		public:
			
			
			ScopeGuard () = delete;
			ScopeGuard (const ScopeGuard &) = delete;
			ScopeGuard & operator = (const ScopeGuard &) = delete;
			
			
			/**
			 *	Creates a new scope guard.
			 *
			 *	\param [in] callback
			 *		The callback to invoke when the
			 *		enclosing scope exits.
			 */
			ScopeGuard (T callback) noexcept(std::is_nothrow_move_constructible<T>::value)
				:	engaged(true),
					callback(std::move(callback))
			{	}
			
			
			/**
			 *	Destroys the scope guard.  If it's
			 *	engaged its callback is executed.
			 */
			~ScopeGuard () noexcept {
				
				destroy();
				
			}
			
			
			/**
			 *	Initializes a new scope guard by
			 *	moving and disengaging another.
			 *
			 *	After this operation, the moved
			 *	scope guard will be disengaged and
			 *	will no longer guard its enclosing
			 *	scope.
			 *
			 *	\param [in] other
			 *		The scope guard to move.
			 */
			ScopeGuard (ScopeGuard && other) noexcept(std::is_nothrow_move_constructible<T>::value)
				:	engaged(other.engaged),
					callback(std::move(other.callback))
			{
			
				other.engaged=false;
				
			}
			
			
			/**
			 *	Replaces this scope guard with another
			 *	scope guard.  If this scope guard is
			 *	engaged, its callback will be fired.
			 *
			 *	\param [in] other
			 *		The scope guard with which to replace
			 *		this one.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			ScopeGuard & operator = (ScopeGuard && other) noexcept(std::is_nothrow_move_constructible<T>::value) {
			
				destroy();
				
				if (&other!=this) {
					
					//	Do this first in case it throws
					//	we haven't erroneously disengaged
					//	the other guard
					engaged=std::move(other.engaged);
					
					engaged=other.engaged;
					other.engaged=false;
					
				}
				
				return *this;
				
			}


			/**
			 *	Disengages this scope guard, preventing
			 *	its callback from firing.
			 */
			void Disengage () noexcept {
			
				engaged=false;
			
			}
		
		
	};
	
	
	/**
	 *	Wraps a callback in a ScopeGuard.
	 *
	 *	\tparam T
	 *		The callback to wrap.
	 *
	 *	\param [in] callback
	 *		The callback to wrap.
	 *
	 *	\return
	 *		A scope guard which will execute
	 *		\em callback when it goes out of
	 *		scope.  Note that if this return
	 *		value is ignored the scope guard
	 *		will go out of scope immediately,
	 *		rather than at the end of the
	 *		enclosing scope.
	 */
	template <typename T>
	ScopeGuard<typename std::decay<T>::type> AtExit (T && callback) noexcept(
		std::is_nothrow_move_constructible<typename std::decay<T>::type>::value
	) {
	
		return ScopeGuard<typename std::decay<T>::type>(std::forward<T>(callback));
		
	}
	
	
	/**
	 *	Wraps a callback in a ScopeGuard.
	 *
	 *	\tparam T
	 *		The callback to wrap.
	 *	\tparam Args
	 *		The type of arguments to pass
	 *		through to the callback of type
	 *		\em T when it is invoked.
	 *
	 *	\param [in] callback
	 *		The callback to wrap.
	 *	\param [in] args
	 *		The arguments to pass through to
	 *		\em callback when it is invoked.
	 *
	 *	\return
	 *		A scope guard which will execute
	 *		\em callback when it goes out of
	 *		scope.  Note that if this return
	 *		value is ignored the scope guard
	 *		will go out of scope immediately,
	 *		rather than at the end of the
	 *		enclosing scope.
	 */
	template <typename T, typename... Args>
	auto AtExit (T && callback, Args &&... args) noexcept(
		noexcept(
			std::bind(
				std::forward<T>(callback),
				std::forward<Args>(args)...
			)
		) &&
		std::is_nothrow_move_constructible<
			decltype(
				std::bind(
					std::forward<T>(callback),
					std::forward<Args>(args)...
				)
			)
		>::value
	) -> ScopeGuard<
		decltype(
			std::bind(
				std::forward<T>(callback),
				std::forward<Args>(args)...
			)
		)
	> {
	
		return AtExit(
			std::bind(
				std::forward<T>(callback),
				std::forward<Args>(args)...
			)
		);
		
	}
	
	
}
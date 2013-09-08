/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <atomic>
#include <functional>


namespace MCPP {


	/**
	 *	\cond
	 */


	class MultiScopeGuardIndirect {
	
	
		private:
		
		
			std::atomic<Word> count;
	
	
		public:
		
		
			MultiScopeGuardIndirect () = delete;
			MultiScopeGuardIndirect (const MultiScopeGuardIndirect &) = delete;
			MultiScopeGuardIndirect (MultiScopeGuardIndirect &&) = delete;
			MultiScopeGuardIndirect & operator = (const MultiScopeGuardIndirect &) = delete;
			MultiScopeGuardIndirect & operator = (MultiScopeGuardIndirect &&) = delete;
		
		
			std::function<void ()> ExitAll;
			std::function<void ()> ExitEach;
			std::function<void ()> Panic;
			
			
			MultiScopeGuardIndirect (
				std::function<void ()>,
				std::function<void ()>,
				std::function<void ()>
			) noexcept;
			
			
			void Acquire () noexcept;
			bool Release () noexcept;
	
	
	};
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	A reference counted scope guard.
	 *
	 *	A scope guard fires a callback whenever
	 *	it goes out of scope.
	 *
	 *	This reference counted scope guard
	 *	additionally fires a callback once
	 *	every reference to it has gone out
	 *	of scope.
	 */
	class MultiScopeGuard {
	
	
		private:
		
		
			MultiScopeGuardIndirect * indirect;
			
			
			inline void destroy () noexcept;
			
			
		public:
		
		
			/**
			 *	Creates a new MultiScopeGuard
			 *	with no associated callbacks.
			 */
			MultiScopeGuard () noexcept;
			/**
			 *	Creates a new MultiScopeGuard.
			 *
			 *	\param [in] all
			 *		A callback which shall be
			 *		invoked once every reference
			 *		to this MultiScopeGuard goes
			 *		out of scope.
			 */
			MultiScopeGuard (std::function<void ()> all);
			/**
			 *	Creates a new MultiScopeGuard.
			 *
			 *	\param [in] all
			 *		A callback which shall be
			 *		invoked once every reference
			 *		to this MultiScopeGuard goes
			 *		out of scope.
			 *	\param [in] each
			 *		A callback which shall be
			 *		invoked each time a reference
			 *		to this MultiScopeGuard goes
			 *		out of scope.
			 */
			MultiScopeGuard (std::function<void ()> all, std::function<void ()> each);
			/**
			 *	Creates a new MultiScopeGuard.
			 *
			 *	\param [in] all
			 *		A callback which shall be
			 *		invoked once every reference
			 *		to this MultiScopeGuard goes
			 *		out of scope.
			 *	\param [in] each
			 *		A callback which shall be
			 *		invoked each time a reference
			 *		to this MultiScopeGuard goes
			 *		out of scope.
			 *	\param [in] panic
			 *		A callback which shall be
			 *		invoked when and if \em all
			 *		and/or \em when throw.
			 */
			MultiScopeGuard (std::function<void ()> all, std::function<void ()> each, std::function<void ()> panic);
			/**
			 *	Copies a MultiScopeGuard, incrementing
			 *	its reference count.
			 *
			 *	\param [in] other
			 *		The MultiScopeGuard to copy.
			 */
			MultiScopeGuard (const MultiScopeGuard & other) noexcept;
			/**
			 *	Moves a MultiScopeGuard, causing it to
			 *	stop guarding this scope and guard the
			 *	destination scope.
			 *
			 *	\param [in] other
			 *		The MultiScopeGuard to move.
			 */
			MultiScopeGuard (MultiScopeGuard && other) noexcept;
			/**
			 *	Overwrites this MultiScopeGuard with
			 *	a copy of another MultiScopeGuard.
			 *
			 *	The MultiScopeGuard will immediately
			 *	go out of scope, firing all appropriate
			 *	callbacks, and will then be replaced with
			 *	a copy of the target MultiScopeGuard,
			 *	causing it to guard its enclosing scope
			 *	with the callbacks contained therein.
			 *
			 *	If \em other is a reference to this object,
			 *	nothing happens.
			 *
			 *	\param [in] other
			 *		The MultiScopeGuard to copy.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			MultiScopeGuard & operator = (const MultiScopeGuard & other) noexcept;
			/**
			 *	Overwrites this MultiScopeGuard with
			 *	another MultiScopeGuard.
			 *
			 *	The MultiScopeGuard will immediately
			 *	go out of scope, firing all appropriate
			 *	callbacks, and will then be replaced with
			 *	a the target MultiScopeGuard, causing it
			 *	to guard its enclosing scope with the
			 *	callbacks contained therein.
			 *
			 *	If \em other is a reference to this object,
			 *	nothing happens.
			 *
			 *	\param [in] other
			 *		The MultiScopeGuard to move.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			MultiScopeGuard & operator = (MultiScopeGuard && other) noexcept;
			/**
			 *	Destroys this MultiScopeGuard, causing it
			 *	to go out of scope.
			 */
			~MultiScopeGuard () noexcept;
	
	
	};


}

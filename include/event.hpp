/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <functional>
#include <type_traits>
 
 
namespace MCPP {


	/**
	 *	\cond
	 */
	 
	 
	template <typename T, typename... Args>
	class Event;
	
	
	/**
	 *	\endcond
	 */
	 
	
	/**
	 *	An event to which subscribers may attach
	 *	to handle the event, and which provides
	 *	facilities for aggregating and filtering
	 *	callback results.
	 *
	 *	\tparam T
	 *		The return type of the callbacks to
	 *		be encapsulated.
	 *	\tparam Args
	 *		A parameter pack specifying the arguments
	 *		which the callbacks shall accept.
	 */
	template <typename T, typename... Args>
	class Event<T (Args...)> {
	
		
		private:
		
		
			Vector<std::function<T (Args...)>> callbacks;
			bool does_throw;
			
			
		public:
		
		
			/**
			 *	Creates a new event with no callbacks
			 *	attached.
			 *
			 *	\param [in] does_throw
			 *		\em false if exceptions thrown from
			 *		callbacks should be silently ignored,
			 *		\em true otherwise.  Defaults to
			 *		\em true.
			 */
			Event (bool does_throw=true) noexcept;
			
			
			/**
			 *	Adds a callback to an event.
			 *
			 *	This is not thread safe.
			 *
			 *	\param [in] func
			 *		A function to add as a callback
			 *		to this event.
			 */
			void Add (const std::function<T (Args...)> & func);
			/**
			 *	Adds a callback to an event.
			 *
			 *	This is not thread safe.
			 *
			 *	\param [in] func
			 *		A function to add as a callback
			 *		to this event.
			 */
			void Add (std::function<T (Args...)> && func);
			
			
			/**
			 *	Executes the stored callbacks.
			 *
			 *	\param [in] args
			 *		Parameters of types \em Args
			 *		which shall be passed through
			 *		to the callbacks.
			 */
			template <typename T1=T>
			typename std::enable_if<
				std::is_same<T1,T>::value && std::is_same<T,void>::value
			>::type operator () (Args... args);
			/**
			 *	Executes the stored callbacks.
			 *
			 *	\param [in] args
			 *		Parameters of types \em Args
			 *		which shall be passed through
			 *		to the callbacks.
			 *
			 *	\return
			 *		\em true if none of the stored
			 *		callbacks returns \em false,
			 *		\em false otherwise.
			 */
			template <typename T1=T>
			typename std::enable_if<
				std::is_same<bool,T1>::value && std::is_same<T,T1>::value,
				bool
			>::type operator () (Args... args);
			/**
			 *	Executes the stored callbacks.
			 *
			 *	\param [in] args
			 *		Parameters of types \em Args
			 *		which shall be passed through
			 *		to the callbacks.
			 *
			 *	\return
			 *		A vector of nullable results of
			 *		calling each callback.  If a function
			 *		threw an exception which was silently
			 *		ignored the result shall be nulled.
			 */
			template <typename T1=T>
			typename std::enable_if<
				!std::is_same<bool,T>::value && std::is_same<T,T1>::value && !std::is_same<T,void>::value,
				Vector<Nullable<T>>
			>::type operator () (Args... args);
			/**
			 *	Executes the stored callbacks in search
			 *	of a particuler result.
			 *
			 *	\tparam expect
			 *		\em false if the function should return
			 *		when the value returned from a callback
			 *		is not \em val, \em true otherwise.
			 *		Defaults to \em false.
			 *
			 *	\param [in] val
			 *		The value to compare the callback results
			 *		to.
			 *	\param [in] args
			 *		Parameters of types \em Args which shall
			 *		be passed through to the callbacks.
			 *
			 *	\return
			 *		A nullable type which shall be \em null if
			 *		none of the callbacks failed the expectation,
			 *		and which shall contain the returned value
			 *		otherwise.
			 */
			/*template <bool expect=false, typename T1=T>
			typename std::enable_if<
				std::is_same<T1,T>::value && !std::is_same<T,void>::value,
				Nullable<T>
			>::type operator () (const T & val, Args... args);*/
		
	
	};
	
	
	template <typename T, typename... Args>
	Event<T (Args...)>::Event (bool does_throw) noexcept : callbacks(0), does_throw(does_throw) {	}
	
	
	template <typename T, typename... Args>
	void Event<T (Args...)>::Add (const std::function<T (Args...)> & func) {
	
		callbacks.Add(func);
	
	}
	
	
	template <typename T, typename... Args>
	void Event<T (Args...)>::Add (std::function<T (Args...)> && func) {
	
		callbacks.Add(std::move(func));
	
	}
	
	
	template <typename T, typename...Args>
	template <typename T1>
	typename std::enable_if<
		std::is_same<T1,T>::value && std::is_same<T,void>::value
	>::type Event<T (Args...)>::operator () (Args... args) {
	
		for (std::function<T (Args...)> & func : callbacks) {
		
			try {
			
				func(args...);
			
			} catch (...) {
			
				if (does_throw) throw;
			
			}
		
		}
	
	}
	
	
	template <typename T, typename... Args>
	template <typename T1>
	typename std::enable_if<
		std::is_same<bool,T1>::value && std::is_same<T,T1>::value,
		bool
	>::type Event<T (Args...)>::operator () (Args... args) {
	
		try {
	
			for (std::function<T (Args...)> & func : callbacks) {
			
				if (!func(args...)) return false;
			
			}
			
		} catch (...) {
		
			if (does_throw) throw;
			
			return false;
		
		}
		
		return true;
	
	}
	
	
	template <typename T, typename... Args>
	template <typename T1>
	typename std::enable_if<
		!std::is_same<bool,T>::value && std::is_same<T,T1>::value && !std::is_same<T,void>::value,
		Vector<Nullable<T>>
	>::type Event<T (Args...)>::operator () (Args... args) {
	
		Vector<Nullable<T>> returnthis(callbacks.Count());
		
		for (std::function<T (Args...)> & func : callbacks) {
		
			try {
			
				returnthis.Add(func(args...));
			
			} catch (...) {
			
				if (does_throw) throw;
				
				returnthis.EmplaceBack();
			
			}
		
		}
		
		return returnthis;
	
	}
	
	
	/*template <typename T, typename... Args>
	template <bool expect, typename T1>
	typename std::enable_if<
		std::is_same<T1,T>::value && !std::is_same<T,void>::value,
		Nullable<T>
	>::type Event<T (Args...)>::operator () (const T & val, Args... args) {
	
		for (std::function<T (Args...)> & func : callbacks) {
		
			Nullable<T> returnthis;
		
			try {
			
				returnthis=func(args...);
				
			} catch (...) {
			
				if (does_throw) throw;
				
				continue;
			
			}
			
			if ((*returnthis==val)!=expect) return returnthis;
		
		}
		
		return Nullable<T>();
	
	}*/


}
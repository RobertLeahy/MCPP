/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <limits>


#ifdef ENVIRONMENT_WINDOWS
#include <windows.h>
#endif


namespace MCPP {


	/**
	 *	Implements an interruptible wrapper
	 *	for the BSD select API.
	 */
	class Select {


		private:
		
		
			#ifdef ENVIRONMENT_WINDOWS
			WSAEVENT wait;
			#endif
			
			
			SOCKET extract_socket (Socket & s) {
			
				return *reinterpret_cast<SOCKET>(&s);
			
			}
			
			
		public:
		
		
			/**
			 *	Returns the maximum number of sockets
			 *	that can be waited on.
			 */
			static Word Max () noexcept;
		
		
			/**
			 *	Creates a new interruptible select
			 *	wrapper.
			 */
			Select ();
			
			
			/**
			 *	Interrupts a pending select call, or
			 *	the next select call which occurs.
			 */
			void Interrupt ();
			
			
			template <typename T>
			void Wait (T && begin, T && end, Word milliseconds=std::numeric_limits<Word>::max()) {
			
				SafeWord diff(static_cast<Word>(end-begin));
				
				diff+=1;
				
				Word vec_len=Word(diff);
				Vector<WSAEVENT> events(vec_len);
				
				for (Word i=0;i<(vec_len-1);++i) {
				
					
				
				}
			
			}


	};


}

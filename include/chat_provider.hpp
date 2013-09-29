/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>



namespace MCPP {


	/**
	 *	Provides the ability for a front-end
	 *	to send messages through chat.
	 */
	class ChatProvider {
	
	
		public:
		
		
			/**
			 *	Sends a message through chat.
			 *
			 *	\param [in] message
			 *		The message to send through
			 *		chat.
			 */
			virtual void Send (const String & message) = 0;
	
	
	};


}

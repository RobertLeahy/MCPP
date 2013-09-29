/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	Specifies an interface to which
	 *	command interpreters must conform,
	 *	allowing the front-end to issue
	 *	commands and receive the results
	 *	thereof.
	 */
	class CommandInterpreter {
	
	
		public:
		
		
			/**
			 *	Executes a command and obtains the
			 *	result thereof.
			 *
			 *	\param [in] command
			 *		The text of the command to
			 *		execute.
			 *
			 *	\return
			 *		A nullable string which contains
			 *		the result of the command, or which
			 *		is nulled if the command returned
			 *		no result.
			 */
			virtual Nullable<String> operator () (const String & command) = 0;
	
	
	};


}
 
/**
 *	\file
 */
 

#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	The base of the object from which
	 *	all modules which the server
	 *	dynamically loads are expected
	 *	to derive.
	 */
	class Module {
	
	
		protected:
		
		
			Module () noexcept;
			
			
		public:
		
		
			Module (const Module &) = delete;
			Module (Module &&) = delete;
			Module & operator = (const Module &) = delete;
			Module & operator = (Module &&) = delete;
			
			
			/**
			 *	When overriden in a derived class,
			 *	cleans this object up.
			 */
			virtual ~Module () noexcept;
			
			
			/**
			 *	When overriden in a derived class,
			 *	returns a number which the server
			 *	shall use to decide in what order
			 *	to install this module.
			 *
			 *	Lower numbers are loaded first.
			 *
			 *	Modules that supply dependencies (other
			 *	than simply being linked against and
			 *	in the same address space) for other
			 *	modules should set a low number.
			 *
			 *	Provided MCPP modules all specify 0
			 *	so that they are loaded first and
			 *	may be easily overridden.
			 *
			 *	No guarantees are made about the state
			 *	of the server when this function is
			 *	called.  <B>Do not attempt to access
			 *	the server, just return a constant
			 *	number, do nothing else.</B>
			 *
			 *	\return
			 *		The priority this module should
			 *		be given in being installed.
			 */
			virtual Word Priority () const noexcept = 0;
			
			
			/**
			 *	When overriden in a derived class,
			 *	hooks the module into the server.
			 *
			 *	When this function is called the server
			 *	is guaranteed to be properly initialized
			 *	with the exception of networking components.
			 */
			virtual void Install () = 0;
			
			
			/**
			 *	When overriden in a derived class,
			 *	returns the name that this module shall
			 *	be known by.
			 *
			 *	\return
			 *		A reference to a string which gives
			 *		the name by which this module shall
			 *		be known.  As it is a reference that
			 *		string must be statically allocated
			 *		in some way.
			 */
			virtual const String & Name () const noexcept = 0;
	
	
	};


} 

/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	Determines the number of simultaneous
	 *	threads of executing the hardware this
	 *	program is running on may execute.
	 *
	 *	\return
	 *		The number of concurrent threads
	 *		that the hardware of this computer
	 *		may execute.
	 */
	Word HardwareConcurrency () noexcept;


}

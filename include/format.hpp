/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	Formats a triplet of version numbers
	 *	into a descriptive string.
	 *
	 *	\param [in] major
	 *		The major version number.
	 *	\param [in] minor
	 *		The minor version number.
	 *	\param [in] patch
	 *		The patch number.
	 *
	 *	\return
	 *		A formatted string.
	 */
	String FormatVersion (Word major, Word minor, Word patch);


}

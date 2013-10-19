/**
 *	\file
 */


#pragma once
 
 
#include <rleahylib/rleahylib.hpp>


namespace Base64 {


	/**
	 *	Encodes a buffer of bytes as
	 *	a base 64 string.
	 *
	 *	\param [in] buffer
	 *		A buffer of bytes.
	 *
	 *	\return
	 *		The base 64 encoding of the
	 *		string.
	 */
	String Encode (const Vector<Byte> & buffer);	


}

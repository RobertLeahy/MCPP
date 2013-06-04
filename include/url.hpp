/**
 *	\file
 */
 
 
#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	Contains tools for dealing with
	 *	URLs.
	 */
	namespace URL {


		/**
		 *	URL encodes a given string.
		 *
		 *	\param [in] encode
		 *		The string to be encoded.
		 *
		 *	\return
		 *		The encoded string.
		 */
		String Encode (const String & encode);
		/**
		 *	URL decodes a given string.
		 *
		 *	\param [in] decode
		 *		The string to be decoded.
		 *
		 *	\return
		 *		The decoded string.
		 */
		String Decode (const String & decode);
		
		
	}


}

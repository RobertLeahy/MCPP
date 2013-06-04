/**
 *	\file
 */


#include <rleahylib/rleahylib.hpp>
#include <evp.h>


namespace MCPP {


	/**
	 *	Implements the SHA-1 hashing
	 *	algorithm.
	 */
	class SHA1 {
	
	
		private:
		
		
			EVP_MD_CTX sha;
			
			
		public:
		
		
			/**
			 *	Creates a new digest by copying
			 *	another digest.
			 *
			 *	\param [in] other
			 *		The digest to copy.
			 */
			SHA1 (const SHA1 & other);
			/**
			 *	Copies another digest into this
			 *	digest.
			 *
			 *	\param [in] other
			 *		The digest to copy.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			SHA1 & operator = (const SHA1 & other);
		
		
			/**
			 *	Initializes a new SHA-1 hash.
			 */
			SHA1 ();
			/**
			 *	Cleans up the digest.
			 */
			~SHA1 () noexcept;
			
			
			/**
			 *	Hashes a given amount of data,
			 *	adding it to the digest.
			 *
			 *	\param [in] data
			 *		The data to hash.
			 */
			void Update (const Vector<Byte> & data);
			/**
			 *	Returns the message digest and
			 *	reinitializes the object so it
			 *	may be used to compute another
			 *	digest.
			 *
			 *	\return
			 *		The completed digest.
			 */
			Vector<Byte> Complete ();
			/**
			 *	Returns the message digest
			 *	formatted as a hex digest and
			 *	reinitializes the object so it
			 *	may be used to compute another
			 *	digest.
			 *
			 *	\return
			 *		The hex digest string.
			 */
			String HexDigest ();
	
	
	};


}

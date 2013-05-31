/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	class RSAKey {
	
	
		private:
		
		
			//	RSA key pair from OpenSSL
			void * key;
			
			
		public:
		
		
			/**
			 *	Creates a new public/private key pair
			 *	suitable for use with the Minecraft
			 *	protocol.
			 *
			 *	\param [in] seed
			 *		An optional string which shall
			 *		be used to seed the OpenSSL
			 *		pseudo random number generator.
			 */
			RSAKey (const String & seed=String());
			/**
			 *	Cleans up this public/private key pair.
			 */
			~RSAKey () noexcept;
			
			
			/**
			 *	Encrypts a buffer of bytes using the private
			 *	key such that it may be decrypted using
			 *	the public key.
			 *
			 *	\param [in] buffer
			 *		A buffer of unencrypted bytes.
			 *
			 *	\return
			 *		A buffer of encrypted bytes.
			 */
			Vector<Byte> PrivateEncrypt (const Vector<Byte> & buffer) const;
			/**
			 *	Recovers a message encrypted with the
			 *	public key.
			 *
			 *	\param [in] buffer
			 *		A buffer of encrypted bytes.
			 *
			 *	\return
			 *		A buffer of unencrypted bytes.
			 */
			Vector<Byte> PrivateDecrypt (const Vector<Byte> & buffer) const;
			/**
			 *	Encrypts a buffer of bytes using the public
			 *	key such that it may be decrypted using
			 *	the private key.
			 *
			 *	\param [in] buffer
			 *		A buffer of unencrypted bytes.
			 *
			 *	\return
			 *		A buffer of encrypted bytes.
			 */
			Vector<Byte> PublicEncrypt (const Vector<Byte> & buffer) const;
			/**
			 *	Recovers a message encrypted with the
			 *	private key.
			 *
			 *	\param [in] buffer
			 *		A buffer of encrypted bytes.
			 *
			 *	\return
			 *		A buffer of unencrypted bytes.
			 */
			Vector<Byte> PublicDecrypt (const Vector<Byte> & buffer) const;
	
	
	};


}
 
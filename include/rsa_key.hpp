/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	Encapsulates an RSA public/private
	 *	key pair.
	 */
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
			 *	Creates a new public key from a
			 *	SubjectPublicKeyInfo, i.e. an
			 *	ASN.1 DER-encoded public key.
			 *
			 *	Note that functions that depend
			 *	on the private key will fail after
			 *	the object has been constructed
			 *	in this way.
			 *
			 *	\param [in] pub_key
			 *		The ASN.1 DER-encoded public
			 *		key from which to construct
			 *		this object.
			 */
			RSAKey (const Vector<Byte> & pub_key);
			/**
			 *	Cleans up this public/private key pair.
			 */
			~RSAKey () noexcept;
			
			
			RSAKey (const RSAKey &) = delete;
			RSAKey (RSAKey &&) = delete;
			RSAKey & operator = (const RSAKey &) = delete;
			RSAKey & operator = (RSAKey &&) = delete;
			
			
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
			
			
			/**
			 *	Retrieves the ASN.1 DER-encoded representation
			 *	of the public key.
			 *
			 *	\return
			 *		The ASN.1, DER-encoded representation of
			 *		the public key.
			 */
			Vector<Byte> PublicKey () const;
	
	
	};


}
 
/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>
#include <evp.h>


namespace MCPP {


	/**
	 *	Provides AES/CFB8 encryption and
	 *	decryption using 128-bit keys.
	 */
	class AES128CFB8 {
	
	
		private:
		
		
			Mutex encrypt_lock;
			EVP_CIPHER_CTX encrypt;
			Mutex decrypt_lock;
			EVP_CIPHER_CTX decrypt;
			
			
		public:
		
		
			AES128CFB8 () = delete;
			AES128CFB8 (const AES128CFB8 &) = delete;
			AES128CFB8 (AES128CFB8 &&) = delete;
			AES128CFB8 & operator = (const AES128CFB8 &) = delete;
			AES128CFB8 & operator = (AES128CFB8 &&) = delete;
		
		
			/**
			 *	Creates a new encryptor/decryptor
			 *	which encrypts and decrypts using
			 *	the given key and initialization
			 *	vector, both of which must be
			 *	at least 16 bytes long.
			 *
			 *	If either the key or initialization
			 *	vector is longer than 16 bytes,
			 *	bytes past the 16th are simply
			 *	ignored.
			 *
			 *	\param [in] key
			 *		The encryption key.
			 *	\param [in] iv
			 *		The initialization vector.
			 */
			AES128CFB8 (const Vector<Byte> & key, const Vector<Byte> & iv);
			/**
			 *	Cleans up this object, freeing all
			 *	used resources and erasing sensitive
			 *	encryption information from memory.
			 */
			~AES128CFB8 () noexcept;
			
			
			/**
			 *	Acquires the necessary lock to encrypt
			 *	in a threadsafe manner.
			 *
			 *	This lock should be held until the data
			 *	is dispatched over the wire or queued
			 *	for such dispatch otherwise the decryptor
			 *	at the other end will not be in sync
			 *	and will not be able to decrypt.
			 */
			void BeginEncrypt () noexcept;
			/**
			 *	Releases the encryption lock.
			 *
			 *	Every call to BeginEncrypt must have
			 *	a matching EndEncrypt call.
			 */
			void EndEncrypt () noexcept;
			/**
			 *	Encrypts a given segment of cleartext.
			 *
			 *	\param [in] cleartext
			 *		The cleartext to be encrypted.
			 *
			 *	\return
			 *		The ciphertext which corresponds to
			 *		\em cleartext.
			 */
			Vector<Byte> Encrypt (const Vector<Byte> & cleartext);
			
			
			/**
			 *	Acquires the necessary lock to
			 *	decrypt in a threadsafe manner.
			 */
			void BeginDecrypt () noexcept;
			/**
			 *	Releases the decryption lock.
			 */
			void EndDecrypt () noexcept;
			/**
			 *	Decrypts a given segment of ciphertext.
			 *
			 *	\param [in] ciphertext
			 *		The ciphertext to be decrypted.
			 *
			 *	\return
			 *		The cleartext which corresponds to
			 *		\em ciphertext.
			 */
			Vector<Byte> Decrypt (const Vector<Byte> & ciphertext);
			/**
			 *	Extracts ciphertext from one buffer and places
			 *	plaintext in another buffer.
			 *
			 *	\param [in,out] ciphertext
			 *		The buffer from which ciphertext shall be
			 *		extracted.
			 *	\param [in,out] plaintext
			 *		The buffer into which plaintext shall be
			 *		appended.
			 */
			void Decrypt (Vector<Byte> * ciphertext, Vector<Byte> * plaintext);
	
	
	};


}

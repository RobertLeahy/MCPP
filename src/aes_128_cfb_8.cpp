#include <aes_128_cfb_8.hpp>
#include <stdexcept>
#include <openssl/aes.h>
#include <openssl/err.h>


namespace MCPP {


	static const char * insufficient_key_size="Encryption key is not long enough";
	static const char * insufficient_iv_size="Initialization vector is not long enough";


	AES128CFB8::AES128CFB8 (const Vector<Byte> & key, const Vector<Byte> & iv) {
	
		//	Insufficient key or IV bytes
		if (key.Count()<16) throw std::invalid_argument(insufficient_key_size);
		if (iv.Count()<16) throw std::invalid_argument(insufficient_iv_size);
	
		//	Initialize cipher contexts
		EVP_CIPHER_CTX_init(&encrypt);
		EVP_CIPHER_CTX_init(&decrypt);
		
		try {
		
			//	Setup cipher types, load in IVs and
			//	keys, use default engine
			if (
				(EVP_EncryptInit_ex(
					&encrypt,
					EVP_aes_128_cfb8(),
					nullptr,
					reinterpret_cast<unsigned char *>(
						static_cast<Byte *>(
							const_cast<Vector<Byte> &>(
								key
							)
						)
					),
					reinterpret_cast<unsigned char *>(
						static_cast<Byte *>(
							const_cast<Vector<Byte> &>(
								iv
							)
						)
					)
				)==0) ||
				(EVP_DecryptInit_ex(
					&decrypt,
					EVP_aes_128_cfb8(),
					nullptr,
					reinterpret_cast<unsigned char *>(
						static_cast<Byte *>(
							const_cast<Vector<Byte> &>(
								key
							)
						)
					),
					reinterpret_cast<unsigned char *>(
						static_cast<Byte *>(
							const_cast<Vector<Byte> &>(
								iv
							)
						)
					)
				)==0)
			) throw std::runtime_error(
				ERR_error_string(
					ERR_get_error(),
					nullptr
				)
			);
		
		} catch (...) {
		
			EVP_CIPHER_CTX_cleanup(&encrypt);
			EVP_CIPHER_CTX_cleanup(&decrypt);
			
			throw;
		
		}
	
	}
	
	
	AES128CFB8::~AES128CFB8 () noexcept {
	
		EVP_CIPHER_CTX_cleanup(&encrypt);
		EVP_CIPHER_CTX_cleanup(&decrypt);
	
	}
	
	
	void AES128CFB8::BeginEncrypt () noexcept {
	
		encrypt_lock.Acquire();
	
	}
	
	
	void AES128CFB8::EndEncrypt () noexcept {
	
		encrypt_lock.Release();
	
	}
	
	
	Vector<Byte> AES128CFB8::Encrypt (const Vector<Byte> & cleartext) {
	
		//	CFB8 always outputs the
		//	same number of bytes
		//	as it takes as input
		Vector<Byte> buffer(cleartext.Count());
		
		//	Short-circuit out if
		//	there's no data to be
		//	encrypted
		if (cleartext.Count()==0) return buffer;
		
		//	Convert the length of the
		//	cleartext into an integer
		//	format acceptable for
		//	OpenSSL
		int len=int(SafeWord(cleartext.Count()));
		
		//	Encrypt
		if (EVP_EncryptUpdate(
			&encrypt,
			reinterpret_cast<unsigned char *>(
				static_cast<Byte *>(
					buffer
				)
			),
			&len,
			reinterpret_cast<unsigned char *>(
				static_cast<Byte *>(
					const_cast<Vector<Byte> &>(
						cleartext
					)
				)
			),
			len
		)==0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);

		//	Update buffer's count
		buffer.SetCount(cleartext.Count());
		
		return buffer;
	
	}
	
	
	Vector<Byte> AES128CFB8::Decrypt (const Vector<Byte> & ciphertext) {
	
		//	CFB8 always outputs the
		//	same number of bytes as it
		//	takes as input
		Vector<Byte> buffer(ciphertext.Count());
		
		//	Short-circuit out if
		//	there's no data to be
		//	decrypted
		if (ciphertext.Count()==0) return buffer;
		
		//	Convert the length of
		//	the ciphertext into an
		//	integer format acceptable
		//	for OpenSSL
		int len=int(SafeWord(ciphertext.Count()));
		
		//	Decrypt
		if (EVP_DecryptUpdate(
			&decrypt,
			reinterpret_cast<unsigned char *>(
				static_cast<Byte *>(
					buffer
				)
			),
			&len,
			reinterpret_cast<unsigned char *>(
				static_cast<Byte *>(
					const_cast<Vector<Byte> &>(
						ciphertext
					)
				)
			),
			len
		)==0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		//	Update buffer's count
		buffer.SetCount(ciphertext.Count());
		
		return buffer;
	
	}
	
	
	void AES128CFB8::Decrypt (Vector<Byte> * ciphertext, Vector<Byte> * plaintext) {
	
		//	Null checks
		if (
			(ciphertext==nullptr) ||
			(plaintext==nullptr)
		) throw std::out_of_range(NullPointerError);
		
		//	If there's no ciphertext, short-circuit
		//	out
		if (ciphertext->Count()==0) return;
	
		//	Pre-allocate enough space in
		//	plaintext
		Word plaintext_capacity=Word(
			SafeWord(ciphertext->Count())+
			SafeWord(plaintext->Count())
		);
		plaintext->SetCapacity(plaintext_capacity);
		
		//	Convert the length of the ciphertext
		//	into an integer format acceptable
		//	for OpenSSL
		int len=int(SafeWord(ciphertext->Count()));
		
		//	Decrypt
		if (EVP_DecryptUpdate(
			&decrypt,
			reinterpret_cast<unsigned char *>(
				static_cast<Byte *>(
					*plaintext
				)+plaintext->Count()
			),
			&len,
			reinterpret_cast<unsigned char *>(
				static_cast<Byte *>(
					*ciphertext
				)
			),
			len
		)==0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		//	Update buffer's count
		plaintext->SetCount(plaintext_capacity);
		
		//	Clear ciphertext buffer
		ciphertext->Delete(0,ciphertext->Count());
	
	}


}

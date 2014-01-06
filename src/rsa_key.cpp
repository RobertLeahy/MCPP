#include <rsa_key.hpp>
#include <stdexcept>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


namespace MCPP {


	//	Error messages
	static const char * insufficient_randomness="Insufficient randomness";
	
	
	//	Constants
	static const int num=1024;	//	Number of bits in the key
	static const unsigned long e=65537;	//	Public exponent of key


	RSAKey::RSAKey (const String & seed) {
	
		//	Convert the seed to bytes
		Vector<Byte> buffer=UTF8().Encode(seed);
		int count=int(SafeWord(buffer.Count()));
		
		//	Seed with bytes from seed string
		RAND_seed(
			static_cast<Byte *>(buffer),
			count
		);
		
		//	Check to make sure PRNG is
		//	ready
		if (RAND_status()==0) throw std::runtime_error(insufficient_randomness);
		
		//	Create key pair
		if ((key=RSA_generate_key(num,e,nullptr,nullptr))==nullptr) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
	
	}
	
	
	RSAKey::RSAKey (const Vector<Byte> & pub_key) {
	
		//	Allocate an RSA structure
		if ((key=RSA_new())==nullptr) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		//	We're manually managing the lifetime
		//	of the RSA structure
		try {
		
			auto * ptr=reinterpret_cast<const unsigned char *>(pub_key.begin());
		
			//	Attempt to decode
			if (d2i_RSA_PUBKEY(
				reinterpret_cast<RSA **>(&key),
				&ptr,
				static_cast<int>(SafeWord(pub_key.Count()))
			)==nullptr) throw std::runtime_error(
				ERR_error_string(
					ERR_get_error(),
					nullptr
				)
			);
			
		} catch (...) {
		
			RSA_free(reinterpret_cast<RSA *>(key));
			
			throw;
		
		}
	
	}
	
	
	RSAKey::~RSAKey () noexcept {
	
		//	Cleanup OpenSSL resources
		RSA_free(reinterpret_cast<RSA *>(key));
	
	}
	
	
	static inline Vector<Byte> driver (
		decltype(RSA_public_encrypt) func,
		const void * key,
		const Vector<Byte> & buffer
	) {
	
		//	Get the correct amount of memory
		//	for the destination buffer
		//
		//	The upper bound will always be
		//	RSA_size
		Word out_buffer_size=Word(
			SafeInt<int>(
				RSA_size(
					const_cast<RSA *>(
						reinterpret_cast<const RSA *>(key)
					)
				)
			)
		);
		//	Create a buffer as the destination
		Vector<Byte> out_buffer(out_buffer_size);
		
		//	Size of input, safely converted
		int input_size=int(SafeWord(buffer.Count()));
		
		//	Perform OpenSSL operation
		int out_buffer_count=func(
			//	Input size
			input_size,
			//	Source buffer.
			//
			//	As it's being read (as "source"
			//	implies) it's safe to cast away
			//	constness for interop with a
			//	poorly-declared API.
			//
			//	Note that in my OpenSSL, latest,
			//	downloaded and built from source,
			//	this parameter actually is declared
			//	const, but I've seen documentation
			//	where it isn't so I'm playing it
			//	safe so this code builds on the
			//	widest range of systems.
			const_cast<unsigned char *>(
				reinterpret_cast<const unsigned char *>(
					static_cast<const Byte *>(
						buffer
					)
				)
			),
			//	Destination buffer, size guaranteed
			//	safe by RSA_size call
			reinterpret_cast<unsigned char *>(
				static_cast<Byte *>(
					out_buffer
				)
			),
			//	Encryption key
			const_cast<RSA *>(
				reinterpret_cast<const RSA *>(key)
			),
			//	Padding
			RSA_PKCS1_PADDING
		);
		
		//	Check if operation failed
		if (out_buffer_count<0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		//	Set count, this conversion
		//	was verified safe by checking
		//	for negatives and by checking
		//	the return value of RSA_size
		out_buffer.SetCount(static_cast<Word>(out_buffer_count));
		
		//	Return
		return out_buffer;
	
	}
	
	
	Vector<Byte> RSAKey::PrivateEncrypt (const Vector<Byte> & buffer) const {
	
		return driver(RSA_private_encrypt,key,buffer);
	
	}
	
	
	Vector<Byte> RSAKey::PrivateDecrypt (const Vector<Byte> & buffer) const {
	
		return driver(RSA_private_decrypt,key,buffer);
	
	}
	
	
	Vector<Byte> RSAKey::PublicEncrypt (const Vector<Byte> & buffer) const {
	
		return driver(RSA_public_encrypt,key,buffer);
	
	}
	
	
	Vector<Byte> RSAKey::PublicDecrypt (const Vector<Byte> & buffer) const {
	
		return driver(RSA_public_decrypt,key,buffer);
	
	}
	
	
	Vector<Byte> RSAKey::PublicKey () const {
	
		//	Get the length of buffer that we
		//	need
		int len=i2d_RSA_PUBKEY(
			reinterpret_cast<RSA *>(
				const_cast<void *>(
					key
				)
			),
			nullptr
		);
		
		if (len<0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		//	Allocate
		Vector<Byte> retr(static_cast<Word>(SafeInt<int>(len)));
		
		//	Encode
		auto * ptr=reinterpret_cast<unsigned char *>(retr.begin());
		if (i2d_RSA_PUBKEY(
			reinterpret_cast<RSA *>(
				const_cast<void *>(
					key
				)
			),
			&ptr
		)<0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		//	Update count
		retr.SetCount(reinterpret_cast<const Byte *>(ptr)-retr.begin());
		
		return retr;
	
	}


}

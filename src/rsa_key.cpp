#include <rsa_key.hpp>
#include <stdexcept>
#include <rand.h>
#include <ssl.h>
#include <err.h>


namespace MCPP {


	//	Error messages
	static const char * insufficient_randomness="Insufficient randomness";
	
	
	//	Constants
	static const int num=1024;	//	Number of bits in the key
	static const unsigned long e=65537;	//	Public exponent of key
	
	
	static inline Vector<Byte> der_count (Word count) {
	
		//	Can this count be represented
		//	by one byte?
		if (count<128) {
		
			Vector<Byte> buffer(1);
			buffer.Add(static_cast<Byte>(count));
			
			return buffer;
		
		}
		
		//	Otherwise, how many bytes will
		//	it take?
		Word num_bytes=0;
		//	Scan each byte in the count
		//	and record the most significant
		//	one which is not 0.
		for (Word i=0;i<sizeof(Word);++i) {
		
			if ((count&(static_cast<Word>(255)<<(i*BitsPerByte())))!=0) num_bytes=i+1;
		
		}
		
		//	Buffer for encoding
		//
		//	We add one because there's a leading
		//	byte that gives information about
		//	the length of the count
		Vector<Byte> buffer(num_bytes+1);
		
		//	In a DER encoded multi-byte count
		//	the first byte encodes the number
		//	of bytes that follow.
		//
		//	It does this by setting its high
		//	bit to 1, and then encoding the
		//	count in the 7 low-order bits
		buffer.Add(static_cast<Byte>(num_bytes)|128);
		
		//	Bytes in DER encoding are big-endian,
		//	which means we can just carve
		//	the bytes out of the count in reverse
		//	order
		for (Word i=num_bytes-1;(num_bytes--)>0;) {
		
			buffer.Add(
				static_cast<Byte>(
					(count&(static_cast<Word>(255)<<(i*BitsPerByte())))>>(i*BitsPerByte())
				)
			);
		
		}
		
		//	Return encoding
		return buffer;
	
	}
	
	
	static inline Vector<Byte> bn_to_der (const BIGNUM * bn) {
	
		//	In the case where the most
		//	significant bit of the leading
		//	byte of the big-endian representation
		//	of the BIGNUM is set, we'll have
		//	to add a leading zero byte to
		//	disambiguate -- negative vs.
		//	positive.
		//
		//	Preallocate this extra byte to
		//	reduce the number of calls that
		//	have to be made to the heap
		//	manager
		Word bignum_size=Word(
			SafeWord(
				Word(
					SafeInt<int>(
						BN_num_bytes(bn)
					)
				)
			)+
			SafeWord(1)
		);
		Vector<Byte> bignum(bignum_size);
		
		//	Insert the big-endian representation
		//	of the big num into the buffer we
		//	just allocated
		//
		//	We know this integer conversion
		//	is safe since we checked above
		bignum.SetCount(
			static_cast<Word>(
				BN_bn2bin(
					bn,
					reinterpret_cast<unsigned char *>(
						static_cast<Byte *>(
							bignum
						)
					)
				)
			)
		);
		
		//	Check leading byte's most
		//	significant bit
		if (!(
			(bignum.Count()==0) ||
			((bignum[0]&128)==0)
		)) bignum.Insert(0,0);
		
		//	Return DER INTEGER representation
		return bignum;
	
	}


	RSAKey::RSAKey (const String & seed) : public_key(0) {
	
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
		
		//	Create ASN.1/DER representation
		//	of public key
		
		//	DER encoded modulus
		Vector<Byte> modulus(bn_to_der(reinterpret_cast<RSA *>(key)->n));
		//	Modulus count
		Vector<Byte> modulus_count(der_count(modulus.Count()));
		//	DER encoded exponent
		Vector<Byte> exponent(bn_to_der(reinterpret_cast<RSA *>(key)->e));
		//	Exponent count
		Vector<Byte> exponent_count(der_count(exponent.Count()));
		
		//	Object identifier and algorithm (constant)
		Vector<Byte> id_and_algo={
			0x30, 13,	//	Begin sequence
			0x06, 9, 42, 134, 72, 134, 247, 13, 1, 1, 1,	//	Object identifier
			0x05, 0	//	Algorthim (NULL)
		};
		
		//	A sequence wraps the modulus
		//	and exponent, prepare for that
		
		//	Get count
		Word mod_exp_seq_count=Word(
			SafeWord(modulus.Count())+	//	Count of the modulus' representation
			SafeWord(modulus_count.Count())+	//	Count of the modulus' count's representation
			SafeWord(1)+	//	One byte for the modulus' tag
			SafeWord(exponent.Count())+	//	Count of the exponent's representation
			SafeWord(exponent_count.Count())+	//	Count of the exponent's count's representation
			SafeWord(1)	//	One byte for the exponent's tag
		);
		
		//	Get DER count
		Vector<Byte> mod_exp_seq_der_count(der_count(mod_exp_seq_count));
		
		//	A bit string wraps the sequence
		//	which wraps the modulus and exponent,
		//	prepare for that
		
		//	Get Count
		Word bit_string_count=Word(
			SafeWord(mod_exp_seq_count)+	//	The sequence being wrapped
			SafeWord(mod_exp_seq_der_count.Count())+	//	The count of that sequence
			SafeWord(1)+	//	That sequence's tag
			SafeWord(1)	//	One byte for unused bits
		);
		
		//	Get DER count
		Vector<Byte> bit_string_der_count(der_count(bit_string_count));
		
		//	Everything is wrapped in a sequence
		//	prepare for that
		
		Word seq_count=Word(
			SafeWord(bit_string_count)+	//	Bit string
			SafeWord(bit_string_der_count.Count())+	//	Bit string's count
			SafeWord(1)+	//	Bit string's tag
			SafeWord(id_and_algo.Count())	//	Object ID and algorithm count
		);
		
		Vector<Byte> seq_der_count(der_count(seq_count));
		
		//	Outermost sequence's length
		//	determines final buffer count
		
		Word public_key_count=Word(
			SafeWord(seq_der_count.Count())+
			SafeWord(seq_count)+
			SafeWord(1)
		);
		
		//	Allocate space
		public_key=Vector<Byte>(public_key_count);
		
		//	Populate
		
		//	Outermost sequence metadata
		public_key.Add(0x30);
		public_key.Add(seq_der_count.begin(),seq_der_count.end());
		
		//	Object ID and Algorithm sequence
		public_key.Add(id_and_algo.begin(),id_and_algo.end());
		
		//	Bit string metedata
		public_key.Add(0x03);
		public_key.Add(bit_string_der_count.begin(),bit_string_der_count.end());
		public_key.Add(0);	//	Zero unused bits
		
		//	Modulus/exponent sequence metadata
		public_key.Add(0x30);
		public_key.Add(mod_exp_seq_der_count.begin(),mod_exp_seq_der_count.end());
		
		//	Modulus
		public_key.Add(0x02);
		public_key.Add(modulus_count.begin(),modulus_count.end());
		public_key.Add(modulus.begin(),modulus.end());
		
		//	Exponent
		public_key.Add(0x02);
		public_key.Add(exponent_count.begin(),exponent_count.end());
		public_key.Add(exponent.begin(),exponent.end());
		
		//	DONE
	
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
	
	
	const Vector<Byte> & RSAKey::PublicKey () const noexcept {
	
		return public_key;
	
	}


}

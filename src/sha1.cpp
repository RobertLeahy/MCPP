#include <sha1.hpp>
#include <err.h>
#include <stdexcept>


namespace MCPP {


	SHA1::SHA1 () {
	
		EVP_MD_CTX_init(&sha);
		
		if (EVP_DigestInit_ex(&sha,EVP_sha1(),nullptr)==0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
	
	}
	
	
	SHA1::~SHA1 () noexcept {
	
		EVP_MD_CTX_cleanup(&sha);
	
	}
	
	
	SHA1::SHA1 (const SHA1 & other) {
	
		EVP_MD_CTX_init(&sha);
		
		if (EVP_MD_CTX_copy_ex(&sha,&other.sha)==0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
	
	}
	
	
	SHA1 & SHA1::operator = (const SHA1 & other) {
	
		EVP_MD_CTX_cleanup(&sha);
		
		EVP_MD_CTX_init(&sha);
		
		if (EVP_MD_CTX_copy_ex(&sha,&other.sha)==0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		return *this;
	
	}
	
	
	void SHA1::Update (const Vector<Byte> & data) {
	
		if (EVP_DigestUpdate(
			&sha,
			static_cast<const Byte *>(data),
			data.Count()
		)==0) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
	
	}
	
	
	Vector<Byte> SHA1::Complete () {
	
		//	Create a buffer to hold
		//	the completed digest
		Vector<Byte> digest(EVP_MAX_MD_SIZE);
		unsigned int digest_count;
		
		//	Copy out the digest and reinitialize
		if (
			(EVP_DigestFinal_ex(
				&sha,
				reinterpret_cast<unsigned char *>(
					static_cast<Byte *>(
						digest
					)
				),
				&digest_count
			)==0) ||
			(EVP_DigestInit_ex(
				&sha,
				EVP_sha1(),
				nullptr
			)==0)
		) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		//	Set buffer's count
		Word digest_count_safe=Word(SafeInt<unsigned int>(digest_count));
		digest.SetCount(digest_count_safe);
		
		return digest;
	
	}
	
	
	String SHA1::HexDigest () {
	
		//	Get the digest
		Vector<Byte> digest(Complete());
		
		String hex_digest;
		bool found=false;
		bool negative=false;
		for (Word i=0;i<digest.Count();++i) {
		
			//	Negative check
			if (
				(i==0) &&
				((digest[i]&128)!=0)
			) {
			
				hex_digest << "-";
			
				negative=true;
				
			}
			
			//	Render negative numbers
			//	properly
			if (negative) {
			
				digest[i]=~digest[i];
				
				if (i==(digest.Count()-1)) digest[i]=(digest[i]==255) ? 0 : digest[i]+1;
			
			}
			
			//	Loop for each nibble
			//	(i.e. each hex digit)
			for (Word n=2;(n--)>0;) {
			
				//	Extract a single hexadecimal digit
				Byte digit=(digest[i]&(
					static_cast<Byte>(15)<<(n*(BitsPerByte()/2))
				))>>(n*(BitsPerByte()/2));
				
				//	Don't add leading zeroes
				if (found || (digit!=0)) {
				
					found=true;
					
					hex_digest << String(digit,16);
				
				}
			
			}
		
		}
		
		return hex_digest.ToLower();
	
	}


}

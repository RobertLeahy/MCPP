#include <random.hpp>
#include <openssl/rand.h>
#include <openssl/err.h>


namespace MCPP {


	Vector<Byte> CryptoRandom (Word num) {
	
		int safe_num=int(SafeWord(num));
		Vector<Byte> buffer(num);
		
		if (RAND_bytes(
			reinterpret_cast<unsigned char *>(
				static_cast<Byte *>(buffer)
			),
			safe_num
		)!=1) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
		
		buffer.SetCount(num);
		
		return buffer;
	
	}


}

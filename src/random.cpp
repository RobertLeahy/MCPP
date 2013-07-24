#include <random.hpp>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <stdexcept>


namespace MCPP {


	void CryptoRandom (Byte * ptr, Word num) {
	
		int safe_num=int(SafeWord(num));
		
		if (RAND_bytes(
			reinterpret_cast<unsigned char *>(ptr),
			safe_num
		)!=1) throw std::runtime_error(
			ERR_error_string(
				ERR_get_error(),
				nullptr
			)
		);
	
	}


}

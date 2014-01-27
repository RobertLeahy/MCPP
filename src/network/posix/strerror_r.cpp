//	Apparently setting -std=gnu++11 sets
//	_GNU_SOURCE on the command line
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE
#endif
#define _POSIX_C_SOURCE 200112L
#include <string.h>


namespace MCPP {


	namespace NetworkImpl {
	
		
		int strerror_r (int errnum, char * buf, size_t buflen) noexcept {
		
			return ::strerror_r(errnum,buf,buflen);
			
		}
		
		
	}
	
	
}

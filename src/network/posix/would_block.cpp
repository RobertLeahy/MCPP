#include <network.hpp>
#include <errno.h>


namespace MCPP {


	namespace NetworkImpl {
	
	
		bool WouldBlock () noexcept {
		
			return
			#if EAGAIN!=EWOULDBLOCK
			(errno==EAGAIN) ||
			#endif
			(errno==EWOULDBLOCK);
		
		}
	
	
	}


}

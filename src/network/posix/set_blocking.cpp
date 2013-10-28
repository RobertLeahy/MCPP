#include <network.hpp>
#include <fcntl.h>


namespace MCPP {


	namespace NetworkImpl {
	
	
		static void set_blocking (int fd, bool blocking) {
		
			//	Get old flags
			auto flags=fcntl(fd,F_GETFL);
			
			//	Was there an error retreiving the
			//	flags?
			if (flags==-1) Raise();
			
			//	Create new flags
			if (blocking) flags&=~static_cast<int>(O_NONBLOCK);
			else flags|=O_NONBLOCK;
			
			//	Attempt to set flags
			if (fcntl(fd,F_SETFL,flags)==-1) Raise();
		
		}


		void SetNonBlocking (int fd) {
		
			set_blocking(fd,false);
		
		}
		
		
		void SetBlocking (int fd) {
		
			set_blocking(fd,true);
		
		}
		
		
	}
	
	
}

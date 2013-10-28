#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		int Notification::Socket () const noexcept {
		
			return event.data.fd;
		
		}
	
	
		bool Notification::Error () const noexcept {
		
			return (event.events&(EPOLLHUP|EPOLLERR))!=0;
		
		}
	
	
		bool Notification::Writeable () const noexcept {
		
			return (event.events&EPOLLOUT)!=0;
		
		}
	
	
		bool Notification::Readable () const noexcept {
		
			return (event.events&EPOLLIN)!=0;
		
		}
	
	
	}


}

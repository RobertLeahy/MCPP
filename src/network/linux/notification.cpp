#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		bool Notification::impl (uint32_t flag) const noexcept {
		
			return (event.events&flag)!=0;
		
		}
	
	
		FDType Notification::FD () const noexcept {
		
			return event.data.fd;
		
		}
	
	
		bool Notification::Readable () const noexcept {
		
			return impl(EPOLLIN);
		
		}
		
		
		bool Notification::Writeable () const noexcept {
		
			return impl(EPOLLOUT);
		
		}
		
		
		bool Notification::Error () const noexcept {
		
			return impl(EPOLLERR);
		
		}
		
		
		bool Notification::End () const noexcept {
		
			return impl(EPOLLHUP);
		
		}
	
	
	}


}

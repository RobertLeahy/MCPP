#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		int Notification::Socket () const noexcept {
		
			return static_cast<int>(event.ident);
		
		}
		
		
		bool Notification::Error () const noexcept {
		
			//	kqueue doesn't report this
			return false;
		
		}
		
		
		bool Notification::Readable () const noexcept {
		
			return event.filter==EVFILT_READ;
		
		}
		
		
		bool Notification::Writeable () const noexcept {
		
			return event.filter==EVFILT_WRITE;
		
		}
	
	
	}


}

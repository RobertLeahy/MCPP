#include <network.hpp>
#include <cstring>
#include <errno.h>
#include <sys/socket.h>


namespace MCPP {


	namespace NetworkImpl {
	
	
		static std::system_error get_exception () noexcept {
		
			return std::system_error(
				std::error_code(
					errno,
					std::system_category()
				)
			);
		
		}
	
	
		std::exception_ptr GetException () noexcept {
		
			return std::make_exception_ptr(get_exception());
		
		}
		
		
		void Raise () {
		
			throw get_exception();
		
		}
		
		
		String GetErrorMessage () {
		
			Vector<Byte> buffer;
			
			//	Loop until the entire error
			//	message has been extracted,
			//	or we fail
			int result;
			do {
			
				//	Get more space
				buffer.SetCapacity();
				
				//	Attempt to extract the error
				//	message
				result=strerror_r(
					errno,
					reinterpret_cast<char *>(buffer.begin()),
					buffer.Capacity()
				);
			
			} while (result==ERANGE);
			
			//	Did we fail?
			if (result!=0) {
			
				errno=result;
				
				Raise();
			
			}
			
			//	Decode and return
			return UTF8().Decode(
				buffer.begin(),
				buffer.begin()+std::strlen(
					reinterpret_cast<char *>(buffer.begin())
				)
			);
		
		}
		
		
		bool WouldBlock () noexcept {
		
			return (
				(errno==EAGAIN)
				#if EAGAIN!=EWOULDBLOCK
				|| (errno==EWOULDBLOCK)
				#endif
			);
		
		}
		
		
		bool WasInterrupted () noexcept {
		
			return errno==EINTR;
		
		}
		
		
		ErrorType GetSocketError (FDType fd) {
		
			ErrorType code;
			socklen_t code_len=sizeof(code);
			if (getsockopt(
				fd,
				SOL_SOCKET,
				SO_ERROR,
				&code,
				&code_len
			)==-1) Raise();
			
			return code;
		
		}
		
		
		void SetError (ErrorType code) noexcept {
		
			errno=code;
		
		}
		
		
		Error::Error () noexcept : set(false) {	}
		
		
		Error::operator bool () const noexcept {
		
			return lock.Execute([&] () {	return set;	});
		
		}
		
		
		void Error::Capture () {
		
			lock.Execute([&] () {
			
				//	If an error has already been captured,
				//	do nothing
				if (set) return;
				
				//	Otherwise, capture
				Message=GetErrorMessage();
				Exception=GetException();
				
				set=true;
			
			});
		
		}
		
		
		void Error::Set (String message) noexcept {
		
			lock.Execute([&] () {
			
				//	Ignore if already set
				if (set) return;
				
				Message=std::move(message);
				
				set=true;
			
			});
		
		}
	
	
	}


}

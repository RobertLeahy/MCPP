#include <network.hpp>
#include <safeint.hpp>
#include <cstring>
#include <unistd.h>


namespace MCPP {


	namespace NetworkImpl {
	
	
		Word Notifier::wait (void * ptr, Word len) {
		
			//	Convert the len parameter
			//	into something safe for epoll
			auto os_len=safe_cast<int>(len);
			//	Loop until we extract some
			//	events
			for (;;) {
			
				auto result=epoll_wait(
					handle,
					reinterpret_cast<struct epoll_event *>(ptr),
					os_len,
					-1	//	INFINITE
				);
				
				//	Was there an error?
				if (result==-1) {
				
					//	Interruption is not really
					//	an error, we just ignore it
					//	and repeat
					if (WasInterrupted()) continue;
					
					//	Otherwise it was an actual
					//	error, throw
					Raise();
				
				}
				
				//	Were no events extracted?
				if (result==0) continue;
				
				//	Some events were extracted
				//
				//	This can't be an unsafe cast
				//	because above we cast the MAXIMUM
				//	that this value can be from a
				//	Word to an int, and that was
				//	successful
				return static_cast<Word>(result);
			
			}
		
		}
	
	
		//	Old versions of Linux need a size, modern
		//	versions require that it be non-zero, here's
		//	a sensible default
		static const int epoll_size=256;
	
	
		Notifier::Notifier () {
		
			//	Create an epoll FD
			if ((handle=epoll_create(epoll_size))==-1) Raise();
		
		}
		
		
		Notifier::~Notifier () noexcept {
		
			close(handle);
		
		}
		
		
		static struct epoll_event get_event (FDType fd) noexcept {
		
			struct epoll_event retr;
			std::memset(&retr,0,sizeof(retr));
			retr.data.fd=fd;
			
			return retr;
		
		}
		
		
		void Notifier::Attach (FDType fd) {
		
			auto event=get_event(fd);
			if (epoll_ctl(
				handle,
				EPOLL_CTL_ADD,
				fd,
				&event
			)==-1) Raise();
		
		}
		
		
		void Notifier::Update (FDType fd, bool read, bool write) {
		
			auto event=get_event(fd);
			event.events=EPOLLET|EPOLLERR|EPOLLHUP;
			if (read) event.events|=EPOLLIN;
			if (write) event.events|=EPOLLOUT;
			if (epoll_ctl(
				handle,
				EPOLL_CTL_MOD,
				fd,
				&event
			)==-1) Raise();
		
		}
		
		
		void Notifier::Detach (FDType fd) {
		
			//	In newer Linuxes, this is unnecessary,
			//	but older Linuxes require a non-null
			//	event pointer, so, for legacy support,
			//	we do this
			auto event=get_event(fd);
			if (epoll_ctl(
				handle,
				EPOLL_CTL_DEL,
				fd,
				&event
			)==-1) Raise();
			
		}
		
		
		Word Notifier::Wait (Notification & n) {
		
			return wait(&n,1);
		
		}
	
	
	}


}

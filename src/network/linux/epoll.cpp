#include <network.hpp>
#include <unistd.h>
#include <system_error>


namespace MCPP {


	namespace NetworkImpl {
	
	
		//	epoll_create takes a size parameter,
		//	which is ignored BUT MUST BE GREATER
		//	THAN ZERO in all Linuxes after
		//	2.6.8.  This should be a sane value
		//	for Linuxes before 2.6.8.
		static const int size=256;
	
	
		Notifier::Notifier () {
		
			//	Create an epoll fd
			if ((handle=epoll_create(size))==-1) Raise();
		
		}
		
		
		Notifier::~Notifier () noexcept {
		
			close(handle);
		
		}
		
		
		void Notifier::Attach (int fd) {
		
			struct epoll_event event;
			event.events=EPOLLET;
			event.data.fd=fd;
			
			if (epoll_ctl(handle,EPOLL_CTL_ADD,fd,&event)==-1) Raise();
		
		}
		
		
		void Notifier::Update (int fd, bool read, bool write) {
		
			struct epoll_event event;
			event.events=EPOLLET;
			if (read) event.events|=EPOLLIN;
			if (write) event.events|=EPOLLOUT;
			event.data.fd=fd;
			
			//	Update notifications received
			//	for this file descriptor
			if (epoll_ctl(handle,EPOLL_CTL_MOD,fd,&event)==-1) Raise();
		
		}
		
		
		Word Notifier::Wait (Notification * ptr, Word size) {
		
			//	Convert input size to make sure
			//	the conversion doesn't overflow
			auto int_size=static_cast<int>(SafeWord(size));
			
			//	Loop until we dequeue events
			//	or encounter an error
			for (;;) {
			
				auto result=epoll_wait(
					handle,
					reinterpret_cast<struct epoll_event *>(ptr),
					int_size,
					-1	//	Infinite
				);
				
				//	Loop again if nothing was
				//	dequeued
				if (result==0) continue;
				
				//	Error detection/handling
				if (result<0) {
				
					//	Interrupted is NOT an error,
					//	wait again
					if (errno==EINTR) continue;
					
					Raise();
					
				}
				
				//	Return number of events dequeued
				return static_cast<Word>(result);
			
			}
		
		}
		
		
		void Notifier::Detach (int socket) {
		
			struct epoll_event event;
			event.events=0;
			event.data.fd=socket;
			
			//	Stop receiving notifications for
			//	this file descriptor
			if (epoll_ctl(handle,EPOLL_CTL_DEL,socket,&event)==-1) Raise();
		
		}
	
	
	}



}

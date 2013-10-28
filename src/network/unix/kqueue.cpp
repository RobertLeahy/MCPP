#include <network.hpp>
#include <unistd.h>
#include <system_error>


namespace MCPP {


	namespace NetworkImpl {
	
	
		Notifier::Notifier () {
		
			//	Create a kernel queue
			if ((handle=kqueue())==-1) Raise();
		
		}
		
		
		Notifier::~Notifier () noexcept {
		
			close(handle);
		
		}
		
		
		//	Does nothing
		void Notifier::Attach (int) {	}
		
		
		enum class EventAction {
		
			Enable,
			Disable,
			Delete
		
		};
		
		
		static void make_event (struct kevent & event, int fd, short filter, EventAction action) noexcept {
		
			u_short flags;
			switch (action) {
			
				case EventAction::Enable:
				case EventAction::Disable:
					fflags=EV_ADD|((action==EventAction::Enable) ? EV_ENABLE : EV_DISABLE);
					break;
				case EventAction::Delete:
				default:
					fflags=EV_DELETE;
					break;
			
			}
		
			EV_SET(
				&event,
				fd,
				filter,
				flags,
				0,
				0,
				nullptr
			);
		
		}
		
		
		void Notifier::Detach (int socket) {
		
			struct kevent events [2];
			make_event(
				events[0],
				socket,
				EVFILT_READ,
				EventAction::Delete
			);
			make_event(
				events[1],
				socket,
				EVFILT_WRITE,
				EventAction::Delete
			);
			
			//	Remove from kernel queue
			if (kevent(handle,events,2,nullptr,0,nullptr)!=0) Raise();
		
		}
		
		
		void Notifier::Update (int fd, bool read, bool write) {
		
			struct kevent events [2];
			//	Read filter
			make_event(
				events[0],
				fd,
				EVFILT_READ,
				read ? EventAction::Enable : EventAction::Disable
			);
			//	Write filter
			make_event(
				events[1],
				fd,
				EVFILT_WRITE,
				write ? EventAction::Enable : EventAction::Disable
			);
			
			//	Add to kernel queue
			if (kevent(handle,events,2,nullptr,0,nullptr)!=0) Raise();
		
		}
		
		
		Word Notifier::Wait (Notification * ptr, Word num) {
		
			//	Convert num to acceptable type
			//	to be passed to kevent
			auto int_size=static_cast<int>(SafeWord(num));
			
			//	Loop until we extract events
			
			for (;;) {
			
				auto result=kevent(
					handle,
					nullptr,
					0,
					reinterpret_cast<struct kevent *>(ptr),
					int_size,
					nullptr	//	Infinite
				);
				
				//	Loop again if nothing was dequeued
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
	
	
	}


}

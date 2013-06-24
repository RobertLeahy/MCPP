#include <select.hpp>


namespace MCPP {


	Select::Select () {
	
		#ifdef ENVIRONMENT_WINDOWS
		if ((wait=WSACreateEvent())==WSA_INVALID_EVENT) SocketException::Raise(WSAGetLastError());
		#endif
	
	}
	
	
	void Select::Interrupt () {
	
		#ifdef ENVIRONMENT_WINDOWS
		if (!WSASetEvent(wait)) SocketException::Raise(WSAGetLastError());
		#endif
	
	}
	
	
	Word Select::Max () noexcept {
	
		//	Subtract one for our dummy event that allows
		//	early wake ups
		return WSA_MAXIMUM_WAIT_EVENTS-1;
	
	}
	
	
	Word Select::Select (, Word milliseconds) {
	
		Word events
	
	}


}

#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		Initializer::Initializer () {
		
			WSADATA temp;	//	Ignored
			int result=WSAStartup(MAKEWORD(2,2),&temp);
			if (result!=0) Raise(result);
		
		}
		
		
		Initializer::~Initializer () noexcept {
		
			WSACleanup();
		
		}
	
	
	}


}

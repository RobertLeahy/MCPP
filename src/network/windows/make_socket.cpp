#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		SOCKET MakeSocket (bool is_v6) {
		
			SOCKET retr=socket(
				is_v6 ? AF_INET6 : AF_INET,
				SOCK_STREAM,
				IPPROTO_TCP
			);
			
			if (retr==INVALID_SOCKET) RaiseWSA();
			
			return retr;
		
		}
	
	
	}


}

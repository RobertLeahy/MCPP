#include <network.hpp>
#include <system_error>


namespace MCPP {


	namespace NetworkImpl {
	
	
		SOCKET MakeSocket (bool v6) {

			SOCKET retr=::socket(
				v6 ? AF_INET6 : AF_INET,
				SOCK_STREAM,
				IPPROTO_TCP
			);
			
			if (retr==INVALID_SOCKET) throw std::system_error(
				std::error_code(
					WSAGetLastError(),
					std::system_category()
				)
			);
			
			return retr;
		
		}
	
	
	}


}

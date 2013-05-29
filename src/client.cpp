#include <client.hpp>


namespace MCPP {


	Client::Client (SmartPointer<Connection> && conn) noexcept : conn(std::move(conn)) {	}


	SmartPointer<Connection> Client::Conn () noexcept {
	
		return conn;
	
	}
	
	
	


}

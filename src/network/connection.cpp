#include <network.hpp>
#include <utility>


using namespace MCPP::NetworkImpl;


namespace MCPP {
	
	
	IPAddress Connection::IP () const noexcept {
	
		return remote_ip;
	
	}
	
	
	UInt16 Connection::Port () const noexcept {
	
		return remote_port;
	
	}
	
	
	Word Connection::Sent () const noexcept {
	
		return sent;
	
	}
	
	
	Word Connection::Received () const noexcept {
	
		return received;
	
	}


}

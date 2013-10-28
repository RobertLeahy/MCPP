#include <network.hpp>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	IPAddress Connection::LocalIP () const noexcept {
	
		return local_ip;
	
	}
	
	
	UInt16 Connection::LocalPort () const noexcept {
	
		return local_port;
	
	}
	
	
	Word Connection::Sent () const noexcept {
	
		return sent;
	
	}
	
	
	Word Connection::Received () const noexcept {
	
		return received;
	
	}
	
	
	IPAddress Connection::IP () const noexcept {
	
		return remote_ip;
	
	}
	
	
	UInt16 Connection::Port () const noexcept {
	
		return remote_port;
	
	}
	
	
	String Connection::Reason () noexcept {
	
		return reason_lock.Execute([&] () mutable {	return std::move(reason);	});
	
	}
	
	
	void Connection::Disconnect () {
	
		disconnect();
	
	}
	
	
	void Connection::Disconnect (String reason) {
	
		reason_lock.Execute([&] () mutable {	this->reason=std::move(reason);	});
		
		disconnect();
	
	}


}

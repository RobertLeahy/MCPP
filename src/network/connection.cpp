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
	
	
	Word Connection::Pending () const noexcept {
	
		return pending;
	
	}
	
	
	void Connection::Disconnect () noexcept {
	
		//	Prevent the reason from being set
		//	elsewhere
		reason_lock.Execute([&] () mutable {	set_reason=true;	});
		
		//	Shutdown the socket so that it's
		//	removed from the handler
		shutdown();
	
	}
	
	
	void Connection::Disconnect (String reason) noexcept {
	
		//	Set the reason unless it's
		//	already been set
		reason_lock.Execute([&] () mutable {
		
			if (!set_reason) {
			
				this->reason.Construct(std::move(reason));
			
				set_reason=true;
			
			}
		
		});
		
		//	Shutdown the socket so that it's removed
		//	from the handler
		shutdown();
	
	}


}

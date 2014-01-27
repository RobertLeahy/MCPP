#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
		
		FollowUp::FollowUp () noexcept
			:	Remove(false),
				Sent(0),
				Received(0),
				Incoming(0),
				Outgoing(0),
				Accepted(0),
				Disconnected(0)
		{	}
		
		
	}
	
	
}

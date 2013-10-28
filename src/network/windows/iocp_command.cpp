#include <network.hpp>
#include <cstring>


namespace MCPP {


	namespace NetworkImpl {
	
	
		IOCPCommand::IOCPCommand (NetworkCommand command) noexcept : Command(command) {
		
			std::memset(&Overlapped,0,sizeof(Overlapped));
		
		}
	
	
	}


}

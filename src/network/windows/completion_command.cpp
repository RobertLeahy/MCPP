#include <network.hpp>
#include <cstring>


namespace MCPP {


	namespace NetworkImpl {
	
	
		CompletionCommand::CompletionCommand (CommandType type) noexcept : Type(type) {
		
			//	Zero out overlapped structure as
			//	WinAPI requires
			std::memset(&overlapped,0,sizeof(overlapped));
		
		}
	
	
	}


}

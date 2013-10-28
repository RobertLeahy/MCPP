#include <network.hpp>
#include <errno.h>
#include <system_error>


namespace MCPP {


	namespace NetworkImpl {
	
	
		void Raise () {
		
			throw std::system_error(
				std::error_code(
					errno,
					std::system_category()
				)
			);
		
		}
	
	
	}


}

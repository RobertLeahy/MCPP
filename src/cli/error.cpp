#include <cli/cli.hpp>
#include <system_error>


namespace MCPP {


	namespace CLIImpl {
	
	
		void Raise () {

			throw std::system_error(
				std::error_code(
					GetLastError(),
					std::system_category()
				)
			);

		}
	
	
	}


}

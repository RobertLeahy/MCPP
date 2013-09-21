#include <hardware_concurrency.hpp>


#ifdef ENVIRONMENT_WINDOWS
#include <windows.h>
#endif


namespace MCPP {


	Word HardwareConcurrency () noexcept {
	
		#ifdef ENVIRONMENT_WINDOWS
		
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		return sysinfo.dwNumberOfProcessors;
		
		#endif
	
	}


}

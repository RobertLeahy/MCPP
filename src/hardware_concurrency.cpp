#include <hardware_concurrency.hpp>


#ifdef ENVIRONMENT_WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif


namespace MCPP {


	Word HardwareConcurrency () noexcept {
	
		#ifdef ENVIRONMENT_WINDOWS
		
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		return sysinfo.dwNumberOfProcessors;
		
		#else
		
		return sysconf(_SC_NPROCESSORS_ONLN);
		
		#endif
	
	}


}

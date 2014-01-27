#include <mysql_data_provider/mysql_data_provider.hpp>


namespace MCPP {


	namespace MySQL {
	
	
		Blob::Blob (const void * ptr, Word len) : Pointer(ptr), Length(safe_cast<unsigned long>(len)) {	}
	
	
	}


}

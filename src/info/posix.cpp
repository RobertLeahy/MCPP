#include <sys/utsname.h>


static inline String from_os (const char * str) {

	return UTF8().Decode(
		reinterpret_cast<const Byte *>(
			str
		),
		reinterpret_cast<const Byte *>(
			str+strlen(str)
		)
	);

}


static inline String get_os_string () {

	struct utsname info;
	if (uname(&info)!=0) throw std::system_error(
		std::error_code(
			errno,
			std::system_category()
		)
	);
	
	String output;
	
	output	<<	from_os(info.sysname)
			<<	" "
			<<	from_os(info.release)
			<<	" "
			<<	from_os(info.machine)
			<<	" "
			<<	from_os(info.version);
	
	return output;

}

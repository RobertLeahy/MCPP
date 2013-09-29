#include "date_time.hpp"
#include <windows.h>


static const String date_template(
	"{0}/{1}/{2} {3}:{4}:{5}.{6}"
);


static inline String format (WORD num, Word len) {

	String str(num);
	
	while (str.Count()<len) str=String("0")+str;
	
	return str;

}


String GetDateTime () {

	//	Get the current local time
	SYSTEMTIME time;
	GetLocalTime(&time);
	
	return String::Format(
		date_template,
		format(
			time.wDay,
			2
		),
		format(
			time.wMonth,
			2
		),
		format(
			time.wYear,
			4
		),
		format(
			time.wHour,
			2
		),
		format(
			time.wMinute,
			2
		),
		format(
			time.wSecond,
			2
		),
		format(
			time.wMilliseconds,
			3
		)
	);

}

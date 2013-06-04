#include <curl.h>
#include <system_error>


void print_bytes (const Vector<Byte> & buffer) {

	bool first=true;
	for (const Byte b : buffer) {
	
		if (first) first=false;
		else StdOut << " ";
		
		StdOut << String(b,16);
	
	}
	
	StdOut << Newline;

}


bool test () {

	return false;

}

#include <common.hpp>
#include <rleahylib/main.hpp>


int Main (const Vector<const String> & args) {

	Memory::NetAlloc=0;

	for (;;) {
	
		Thread::Sleep(100);
	
		Word net_alloc=Memory::NetAlloc;
	
		StdOut << net_alloc << Newline;
	
	}
	
	return EXIT_SUCCESS;

}

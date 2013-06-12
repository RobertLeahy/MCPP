#include <common.hpp>
#include <rleahylib/main.hpp>


#ifdef DEBUG
#include "test.cpp"
#endif


int Main (const Vector<const String> & args) {

	#ifdef DEBUG
	if (test()) return EXIT_SUCCESS;
	#endif
	
	RunningServer.Construct();
	
	RunningServer->StartInteractive(args);
	
	StdIn.ReadLine();
	//for (;;) Thread::Sleep(1000);
	
	RunningServer.Destroy();
	
	return EXIT_SUCCESS;

}

#include <common.hpp>
#include <rleahylib/main.hpp>
#include <unordered_map>


static std::unordered_map<const Connection *, bool> map;
static Mutex map_lock;
static Mutex lock;
static CondVar wait;
static const Word timeout=50;
static Nullable<ThreadPool> console_write;


int Main (const Vector<const String> & args) {

	#ifdef DEBUG
	Memory::NetAlloc=0;
	#endif

	#ifdef DEBUG
	//if (test()) return EXIT_SUCCESS;
	#endif

	console_write.Construct(1);
	RunningServer.Construct();
	
	Server::Get().SetDebug(true);
	//Server::Get().LoggedPackets.insert(0x0D);
	//Server::Get().Verbose.insert("raw_send");
	Server::Get().SetVerbose("world");
	
	Server::Get().OnLog.Add([] (const String & log, Service::LogType) {
	
		console_write->Enqueue([=] () {	StdOut << log << Newline;	});
	
	});
	
	//Thread t(thread_func);

	Server::Get().StartInteractive(args);
	
	//	Test
	/*std::function<void ()> callback;
	callback=[&] () {
	
		Server::Get().WriteLog(
			"Tick",
			Service::LogType::Information
		);
		
		Server::Get().Pool().Enqueue(
			1000,
			callback
		);
	
	};
	Server::Get().Pool().Enqueue(
		1000,
		callback
	);*/
	
	/*Thread alloc_count([] () {
	
		for (;;) {
		
			Thread::Sleep(100);
			
			Word net_alloc=Memory::NetAlloc;
			
			StdOut << "Allocated memory blocks: " << net_alloc << Newline;
		
		}
	
	});*/
	
	//StdIn.ReadLine();
	for (;;) Thread::Sleep(1000);
	
	/*lock.Execute([] () {
	
		stop=true;
		
		wait.WakeAll();
	
	});
	
	t.Join();*/
	
	RunningServer.Destroy();
	
	return EXIT_SUCCESS;

}

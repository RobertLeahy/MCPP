#include <common.hpp>
#include <rleahylib/main.hpp>
#include <unordered_map>


#ifdef DEBUG
#include "test.cpp"
#endif


static std::unordered_map<const Connection *, bool> map;
static Mutex map_lock;
static Mutex lock;
static CondVar wait;
static bool stop;
static const Word timeout=50;
static UInt128 tick=0;
static Mutex tick_lock;
static CondVar tick_wait;
static bool tick_stop;
static Nullable<ThreadPool> console_write;


void tick_thread_func () {

	Packet packet;
	packet.SetType<PacketTypeMap<0x04>>();
	
	for (;;) {
	
		if (tick_lock.Execute([] () {
		
			if (tick_stop) return tick_stop;
		
			tick_wait.Sleep(tick_lock,timeout);
			
			return tick_stop;
		
		})) break;
		
		RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
		
			packet.Retrieve<Int64>(0)=static_cast<Int64>(++tick);
			packet.Retrieve<Int64>(1)=static_cast<Int64>(tick);
		
			if (client->GetState()==ClientState::Authenticated) {
			
				map_lock.Execute([&] () {
				
					auto iter=map.find(client->GetConn());
					
					if (
						(iter!=map.end()) &&
						(iter->second)
					) client->Send(packet);
				
				});
			
			}
		
		});
	
	}

}


void thread_func () {

	typedef PacketTypeMap<0x0D> pt;

	Packet packet;
	packet.SetType<pt>();
	
	packet.Retrieve<pt,0>()=0.0;
	packet.Retrieve<pt,1>()=0.0;
	packet.Retrieve<pt,2>()=74.0;
	packet.Retrieve<pt,3>()=0.0;
	packet.Retrieve<pt,4>()=0.0;
	packet.Retrieve<pt,5>()=0.0;
	packet.Retrieve<pt,6>()=true;

	for (;;) {
	
		if (lock.Execute([] () {
		
			if (stop) return stop;
		
			wait.Sleep(lock,timeout);
			
			return stop;
		
		})) break;
		
		RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
		
			if (client->GetState()==ClientState::Authenticated) {
			
				map_lock.Execute([&] () {
				
					auto iter=map.find(client->GetConn());
					
					if (
						(iter!=map.end()) &&
						(iter->second)
					) client->Send(packet);
				
				});
			
			}
		
		});
	
	}

}


int Main (const Vector<const String> & args) {

	#ifdef DEBUG
	Memory::NetAlloc=0;
	#endif

	#ifdef DEBUG
	//if (test()) return EXIT_SUCCESS;
	#endif

	console_write.Construct(1);
	RunningServer.Construct();
	
	RunningServer->ProtocolAnalysis=true;
	RunningServer->LoggedPackets.insert(0x03);
	RunningServer->OnLogin.Add([] (SmartPointer<Client> client) {
		
		typedef PacketTypeMap<0x06> sp;
		
		Packet packet;
		packet.SetType<sp>();
		
		packet.Retrieve<sp,0>()=0;
		packet.Retrieve<sp,1>()=0;
		packet.Retrieve<sp,2>()=0;
		
		client->Send(packet);
		
		map_lock.Execute([&] () {
		
			auto iter=map.find(client->GetConn());
			
			if (iter!=map.end()) iter->second=true;
		
		});
	
	});
	
	RunningServer->OnConnect.Add([] (SmartPointer<Client> client) {
	
		map_lock.Execute([&] () {
		
			map.emplace(
				client->GetConn(),
				false
			);
		
		});
	
	});
	
	RunningServer->OnDisconnect.Add([] (SmartPointer<Client> client, const String & reason) {
	
		map_lock.Execute([&] () {
		
			map.erase(client->GetConn());
		
		});
	
	});
	
	RunningServer->OnLog.Add([] (const String & log, Service::LogType) {
	
		console_write->Enqueue([=] () {	StdOut << log << Newline;	});
	
	});
	
	Thread t(thread_func);
	Thread t1(tick_thread_func);

	RunningServer->StartInteractive(args);
	
	//	Test
	/*std::function<void ()> callback;
	callback=[&] () {
	
		RunningServer->WriteLog(
			"Tick",
			Service::LogType::Information
		);
		
		RunningServer->Pool().Enqueue(
			1000,
			callback
		);
	
	};
	RunningServer->Pool().Enqueue(
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
	
	StdIn.ReadLine();
	//for (;;) Thread::Sleep(1000);
	
	lock.Execute([] () {
	
		stop=true;
		
		wait.WakeAll();
	
	});
	
	t.Join();
	
	tick_lock.Execute([] () {
	
		tick_stop=true;
		
		tick_wait.WakeAll();
	
	});
	
	t1.Join();
	
	RunningServer.Destroy();
	
	return EXIT_SUCCESS;

}

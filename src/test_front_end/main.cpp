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
static Mutex console_lock;


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

	Packet packet;
	packet.SetType<PacketTypeMap<0x0D>>();
	
	packet.Retrieve<Double>(0)=0.0;
	packet.Retrieve<Double>(1)=0.0;
	packet.Retrieve<Double>(2)=74.0;
	packet.Retrieve<Double>(3)=0.0;
	packet.Retrieve<Single>(4)=0.0;
	packet.Retrieve<Single>(5)=0.0;
	packet.Retrieve<bool>(6)=true;

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
	if (test()) return EXIT_SUCCESS;
	#endif
	
	RunningServer.Construct();
	
	RunningServer->ProtocolAnalysis=true;
	RunningServer->OnLogin.Add([] (SmartPointer<Client> client) {
		
		Packet packet;
		packet.SetType<PacketTypeMap<0x06>>();
		
		packet.Retrieve<Int32>(0)=0;
		packet.Retrieve<Int32>(1)=0;
		packet.Retrieve<Int32>(2)=0;
		
		client->Send(packet);
		
		map_lock.Execute([&] () {
		
			auto iter=map.find(client->GetConn());
			
			if (iter!=map.end()) iter->second=true;
		
		});
	
	});
	
	RunningServer->OnConnect.Add([] (SmartPointer<Connection> conn) {
	
		map_lock.Execute([&] () {
		
			map.emplace(
				static_cast<Connection *>(conn),
				false
			);
		
		});
	
	});
	
	RunningServer->OnDisconnect.Add([] (SmartPointer<Connection> conn, const String & reason) {
	
		map_lock.Execute([&] () {
		
			map.erase(static_cast<Connection *>(conn));
		
		});
	
	});
	
	RunningServer->OnLog.Add([] (const String & log, Service::LogType) {
	
		console_lock.Execute([&] () {	StdOut << log << Newline;	});
	
	});
	
	Thread t(thread_func);
	Thread t1(tick_thread_func);
	
	RunningServer->StartInteractive(args);
	
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

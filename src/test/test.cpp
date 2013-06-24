#include <network.hpp>
#include <thread_pool.hpp>
#include <rleahylib/main.hpp>
#include <unordered_map>


using namespace MCPP;


Nullable<ThreadPool> pool;
Nullable<ConnectionHandler> connections;
Mutex console_lock;
std::unordered_map<const Connection *,SmartPointer<Connection>> map;
Mutex map_lock;


void write_console (const String & str) {

	console_lock.Execute([&] () {	StdOut << str << Newline;	});

}



int Main (const Vector<const String> & args) {

	pool.Construct(1);
	
	Vector<Endpoint> eps;
	eps.EmplaceBack(
		IPAddress("0.0.0.0"),
		25565
	);
	
	connections.Construct(
		eps,
		[] (IPAddress ip, UInt16 port) {
		
			write_console(String::Format(
				"Approving {0}:{1}",
				ip,
				port
			));
		
			return true;
			
		},
		[] (SmartPointer<Connection> conn) {
		
			write_console(String::Format(
				"{0}:{1} connected",
				conn->IP(),
				conn->Port()
			));
			
			map_lock.Execute([&] () {	map.emplace(static_cast<Connection *>(conn),conn);	});
		
		},
		[] (SmartPointer<Connection> conn, const String &) {
		
			write_console(String::Format(
				"{0}:{1} disconnected",
				conn->IP(),
				conn->Port()
			));
			
			map_lock.Execute([&] () {	map.erase(static_cast<Connection *>(conn));	});
		
		},
		[] (SmartPointer<Connection> conn, Vector<Byte> & buffer) {
		
			String output;
			output << String(conn->IP()) << ":" << String(conn->Port()) << " sends:";
			
			for (Byte b : buffer) {
			
				output << " 0x";
			
				String byte(b,16);
				
				if (byte.Count()==1) output << "0";
				
				output << byte;
			
			}
			
			write_console(output);
			
			conn->Send(buffer);
			
			buffer.Clear();
		
		},
		LogCallback(),
		[] () {	write_console("PANIC");	},
		*pool
	);
	
	StdIn.ReadLine();
	
	connections.Destroy();
	pool.Destroy();
	
	StdIn.ReadLine();
	/*for (;;) {
	
		map_lock.Execute([&] () {
		
			for (auto & pair : map) {
			
				pair.second->Send(UTF8().Encode("hello world"));
			
			}
		
		});
		
		Thread::Sleep(25);
	
	}*/

	return EXIT_SUCCESS;

}

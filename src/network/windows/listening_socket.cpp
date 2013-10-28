#include <network.hpp>
#include <system_error>
#include <utility>


namespace MCPP {


	namespace NetworkImpl {
	
	
		ListeningSocket::ListeningSocket (IPAddress ip, UInt16 port, IOCP & iocp) : ip(ip), port(port) {
		
			//	Create socket
			socket=MakeSocket(ip.IsV6());
			
			//	We're now responsible for the
			//	lifetime of a socket
			try {
			
				//	Bind to interface and begin
				//	listening
				struct sockaddr_storage addr;
				ip.ToOS(&addr,port);
				
				if (!(
					(bind(
						socket,
						reinterpret_cast<struct sockaddr *>(&addr),
						sizeof(addr)
					)==0) &&
					(listen(
						socket,
						SOMAXCONN
					)==0)
				)) throw std::system_error(
					std::error_code(
						WSAGetLastError(),
						std::system_category()
					)
				);
				
				//	Associate with IOCP
				iocp.Attach(socket,this);
			
			} catch (...) {
			
				closesocket(socket);
				
				throw;
			
			}
		
		}
		
		
		void ListeningSocket::Dispatch () {
		
			//	Create a socket for the new incoming
			//	connection
			auto s=MakeSocket(ip.IsV6());
			
			//	We're responsible for that socket
			//	until it gets placed into an
			//	AcceptCommand object
			std::unique_ptr<AcceptCommand> command;
			try {
			
				command=lock.Execute([&] () mutable {
				
					//	We reused already allocated
					//	commands if possible
					
					//	None to reuse, allocate a new one
					if (available.Count()==0) return std::unique_ptr<AcceptCommand>(
						new AcceptCommand(s,socket)
					);
					
					//	At least one is available,
					//	reuse it
					
					Word loc=available.Count()-1;
					
					auto retr=std::move(available[loc]);
					available.Delete(loc);
					
					retr->Imbue(s);
					
					return retr;
				
				});
			
			} catch (...) {
			
				closesocket(s);
				
				throw;
			
			}
			
			auto ptr=command.get();
			
			lock.Execute([&] () mutable {	pending.emplace(ptr,std::move(command));	});
			
			//	We need to remove the command
			//	from the collection if posting
			//	to the completion port fails
			try {
			
				ptr->Dispatch(socket);
			
			} catch (...) {
			
				lock.Execute([&] () mutable {	pending.erase(ptr);	});
				
				throw;
			
			}
		
		}
		
		
		void ListeningSocket::Complete (AcceptCommand * command) {
		
			lock.Execute([&] () mutable {
			
				auto iter=pending.find(command);
				
				auto ptr=std::move(iter->second);
				
				pending.erase(iter);
				
				available.Add(std::move(ptr));
			
			});
		
		}
	
	
	}


}

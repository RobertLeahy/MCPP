#include <network.hpp>
#include <stdexcept>
#include <sys/socket.h>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	//	Ends of the socket pair that the master
	//	and slave will variously use
	static const Word master=0;
	static const Word slave=1;


	ConnectionHandler::WorkerChannel::WorkerChannel () {
	
		//	Make sure sockets are blocking/non-blocking
		//	as appropriate
		SetBlocking(pair[master],true);
		SetBlocking(pair[slave],false);
	
	}
	
	
	void ConnectionHandler::WorkerChannel::Attach (NetworkImpl::Notifier & n) {
	
		n.Attach(pair[slave]);
		n.Update(pair[slave],true,false);
	
	}
	
	
	void ConnectionHandler::WorkerChannel::Send (Command c) {
	
		lock.Execute([&] () {
		
			//	Send one byte through to slave to
			//	wake them up so they'll process
			//	this message
			Byte b=0;
			for (;;) switch (send(pair[master],&b,1,0)) {
			
				case 0:
					//	Nothing was sent, try again
					continue;
					
				case 1:
					//	We're done
					goto done;
					
				default:
					//	Error
					if (WasInterrupted()) continue;
					Raise();
			
			}
			
			done:

			//	Add to the collection
			commands.Add(std::move(c));
			
		});	
	}
	
	
	static const char * end_of_stream="Unexpected end of stream";
	Nullable<ConnectionHandler::Command> ConnectionHandler::WorkerChannel::Receive () {
		
		//	Get everything out of the socket
		Byte b;
		for (;;) switch (recv(pair[slave],&b,1,0)) {
		
			case 0:
				//	THIS SHOULD NEVER HAPPEN
				throw std::runtime_error(end_of_stream);
				
			case 1:
				//	Keep going...
				continue;
				
			default:
				//	ERROR
				if (WouldBlock()) goto done;
				if (WasInterrupted()) continue;
				Raise();
		
		}
		
		done:
		
		//	Actually extract a command (if applicable)
		return lock.Execute([&] () {
		
			Nullable<Command> retr;
			
			if (commands.Count()==0) return retr;
			
			retr.Construct(std::move(commands[0]));
			commands.Delete(0);
			
			return retr;
		
		});
	
	}
	
	
	bool ConnectionHandler::WorkerChannel::Is (FDType fd) const noexcept {
	
		return pair[slave]==fd;
	
	}


}

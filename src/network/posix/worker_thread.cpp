#include <network.hpp>
#include <utility>


namespace MCPP {


	namespace NetworkImpl {
	
	
		WorkerThread::WorkerThread () {
		
			//	We need to associate the control
			//	socket with the notifier
			auto fd=control.Wait();
			notifier.Attach(fd);
			notifier.Update(fd,true,false);
		
		}
	
	
		Word WorkerThread::Count () const noexcept {
		
			return lock.Execute([&] () {	return connections.size();	});
		
		}
		
		
		void WorkerThread::AddSocket (int socket, SmartPointer<Connection> conn) {
		
			lock.Execute([&] () mutable {
			
				//	Insert new socket and connection
				auto iter=connections.emplace(socket,std::move(conn)).first;
				
				//	If failure occurs, we have to
				//	roll back
				try {
				
					//	Insert pending inter thread communication
					//	message
					auto pair=pending.emplace(socket);
					
					//	If failure occurs, we have to
					//	roll back
					try {
					
						//	Notify worker
						control.Put();
					
					} catch (...) {
					
						if (pair.second) pending.erase(pair.first);
						
						throw;
					
					}
				
				} catch (...) {
				
					connections.erase(iter);
					
					throw;
				
				}
			
			});
		
		}
		
		
		Nullable<int> WorkerThread::GetSocket () {
		
			//	Flush all pending messages
			control.Get();
			
			Nullable<int> retr;
			
			//	Retrieve a pending message
			lock.Execute([&] () mutable {
			
				auto iter=pending.begin();
				
				//	If there are no messages, return
				//	at once
				if (iter==pending.end()) return;
				
				retr.Construct(*iter);
				
				pending.erase(iter);
			
			});
			
			return retr;
		
		}
		
		
		void WorkerThread::Shutdown () {
		
			//	We enqueue an invalid file
			//	handle to tell the worker to
			//	stop
			lock.Execute([&] () mutable {
			
				auto iter=pending.emplace(-1).first;
				
				//	We have to roll back the insert
				//	if sending inter thread communications
				//	fails
				try {
				
					control.Put();
				
				} catch (...) {
				
					pending.erase(iter);
					
					throw;
				
				}
			
			});
		
		}
		
		
		void WorkerThread::Put (int socket) {
		
			lock.Execute([&] () {
			
				auto iter=connections.find(socket);
				
				//	If this socket has already been
				//	disassociated from this worker,
				//	DO NOTHING
				if (iter==connections.end()) return;
				
				auto pair=pending.emplace(socket);
				
				//	We have to rollback if sending
				//	message fails
				try {
				
					control.Put();
				
				} catch (...) {
				
					if (pair.second) pending.erase(pair.first);
					
					throw;
				
				}
			
			});
		
		}
		
		
		SmartPointer<Connection> WorkerThread::Get (int socket) const noexcept {
		
			return lock.Execute([&] () {
			
				auto iter=connections.find(socket);
				
				return (iter==connections.end()) ? SmartPointer<Connection>() : iter->second;
			
			});
		
		}
		
		
		void WorkerThread::Remove (int socket) {
		
			//	We remove the socket-in-question
			//	AND any pending messages for it
			lock.Execute([&] () mutable {
			
				//	Ignore this socket while
				//	processing all remaining
				//	events dequeued from the
				//	notifier this cycle
				ignored.insert(socket);
			
				connections.erase(socket);
				pending.erase(socket);
			
			});
		
		}
		
		
		bool WorkerThread::IsControl (int fd) const noexcept {
		
			return fd==control.Wait();
		
		}
		
		
		Notifier & WorkerThread::Get () noexcept {
		
			return notifier;
		
		}
		
		
		void WorkerThread::Clear () noexcept {
		
			ignored.clear();
		
		}
		
		
		bool WorkerThread::IsIgnored (int socket) const noexcept {
		
			return ignored.count(socket)!=0;
		
		}
	
	
	}


}

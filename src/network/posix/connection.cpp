#include <network.hpp>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>


using namespace MCPP::NetworkImpl;


namespace MCPP {
	
	
	//	Default size for a receive buffer
	static const Word default_size=256;
	
	
	Connection::Connection (
		int socket,
		IPAddress remote_ip,
		UInt16 remote_port,
		WorkerThread & worker,
		IPAddress local_ip,
		UInt16 local_port
	) noexcept
		:	socket(socket),
			worker(worker),
			is_shutdown(false),
			attached(false),
			callback_in_progress(false),
			remote_ip(remote_ip),
			remote_port(remote_port),
			local_ip(local_ip),
			local_port(local_port)
	{
	
		received=0;
		sent=0;
	
	}
	
	
	Connection::~Connection () noexcept {
	
		close(socket);
		
		for (auto & handle : sends) handle->Fail();
	
	}
	
	
	Word Connection::Pending () const noexcept {
	
		return lock.Execute([&] () {	return sends.Count();	});
	
	}
	
	
	void Connection::disconnect () {
	
		shutdown();
		
		//	Tell our worker to wake up
		//	and check this socket, leading
		//	to it being removed, detached,
		//	and closed.
		worker.Put(socket);
	
	}
	
	
	void Connection::update () {
	
		auto & notifier=worker.Get();
		
		if (is_shutdown) {
		
			if (attached) {
			
				notifier.Detach(socket);
				
				attached=false;
			
			}
		
		} else {
		
			if (!attached) {
			
				notifier.Attach(socket);
				
				attached=true;
			
			}
			
			notifier.Update(
				socket,
				!callback_in_progress,
				sends.Count()!=0
			);
		
		}
	
	}
	
	
	void Connection::shutdown () noexcept {
	
		lock.Execute([&] () mutable {
		
			if (is_shutdown) return;
			
			is_shutdown=true;
			
			::shutdown(socket,SHUT_RDWR);
		
		});
	
	}
	
	
	void Connection::Shutdown () noexcept {
	
		shutdown();
	
	}
	
	
	bool Connection::Update () {
	
		return lock.Execute([&] () mutable {
		
			update();
			
			return !is_shutdown;
			
		});
	
	}
	
	
	void Connection::CompleteReceive () {
	
		lock.Execute([&] () mutable {
		
			callback_in_progress=false;
			
			update();
		
		});
	
	}
	
	
	int Connection::Socket () const noexcept {
	
		return socket;
	
	}
	
	
	Word Connection::Receive () {
	
		//	Do not listen for receives until
		//	the callback completes
		lock.Execute([&] () mutable {
		
			callback_in_progress=true;
			update();
		
		});
		
		//	Loop until we can't receive
		//	any more data
		Word recvd=0;
		for (;;) {
		
			//	Resize buffer as necessary
			if (buffer.Capacity()==1) buffer.SetCapacity(default_size);
			else if (buffer.Count()==buffer.Capacity()) buffer.SetCapacity();
			
			//	Receive data
			auto count=recv(
				socket,
				buffer.begin(),
				buffer.Capacity()-buffer.Count(),
				0
			);
			
			//	End loop if we receive nothing
			if (count==0) break;
			
			//	Did receive fail?
			//
			//	If receive failed because it would
			//	block, we simply proceed as usualy,
			//	that's not an error condition from
			//	our point-of-view
			if (count<0) {
			
				if (WouldBlock()) break;
				
				//	Zero is our error condition
				return 0;
			
			}
			
			//	Set the buffer's count appropriately
			buffer.SetCount(buffer.Count()+static_cast<Word>(count));
			
			//	Increment received overall count
			recvd+=static_cast<Word>(count);
		
		}
		
		//	Increment received count for this
		//	connection
		received+=recvd;
		
		return recvd;
	
	}
	
	
	Word Connection::Send (ThreadPool & pool) {
	
		//	Loop until we have nothing more
		//	to send
		Word sent=0;
		lock.Execute([&] () mutable {
		
			while (sends.Count()!=0) {
			
				auto & send=sends[0];
				
				while (send->Sent!=send->Buffer.Count()) {
				
					auto count=::send(
						socket,
						send->Buffer.begin()+send->Sent,
						send->Buffer.Count()-send->Sent,
						0
					);
					
					//	If we sent nothing, that's an
					//	error
					if (count==0) return;
					
					if (count<0) {
					
						//	We return normally if call would
						//	block
						if (WouldBlock()) return;
						
						//	Otherwise we fail
						sent=0;
						return;
					
					}
					
					//	Update buffer count
					send->Sent+=static_cast<Word>(count);
					//	Update total sent
					sent+=static_cast<Word>(count);
				
				}
				
				//	Done with that
				auto handle=std::move(sends[0]);
				sends.Delete(0);
				
				//	Complete this send
				handle->Complete(pool);
			
			}
			
			//	We sent everything, update
			//	what events we're listening
			//	for (no longer interested
			//	in writeability)
			update();
		
		});
		
		//	Increment sent count for this
		//	connection
		this->sent+=sent;
		
		return sent;
	
	}
	
	
	Vector<Byte> & Connection::Get () noexcept {
	
		return buffer;
	
	}
	
	
	SmartPointer<SendHandle> Connection::Send (Vector<Byte> buffer) {
	
		//	Store the buffer's count
		auto count=buffer.Count();
	
		//	Create a send handle
		auto handle=SmartPointer<SendHandle>::Make(std::move(buffer));
		
		//	If there are no bytes in the buffer,
		//	succeed at once
		if (count==0) {
		
			handle->Complete();
			
			return handle;
		
		}
		
		//	Lock to queue send
		lock.Execute([&] () mutable {
		
			//	Fail immediately if socket is
			//	already shutdown
			if (is_shutdown) {
			
				handle->Fail();
				
				return;
			
			}
			
			//	We only have to update if this
			//	is the first send
			bool do_update=sends.Count()==0;
		
			//	Add to set of pending sends
			sends.Add(handle);
			
			//	If an error occurs, we must
			//	remove the send handle from
			//	the list of pending sends
			if (do_update) try {
			
				//	Send inter thread communication
				//	message to the associated worker
				//	so that this socket's associated
				//	notifications will be updated
				worker.Put(socket);
			
			} catch (...) {
			
				sends.Delete(sends.Count()-1);
				
				throw;
			
			}
		
		});
		
		return handle;
	
	}


}

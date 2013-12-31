#include <network.hpp>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	void Connection::shutdown () noexcept {
	
		//	Shutdown, but only if the socket
		//	has not already been shutdown
		if (sends_lock.Execute([&] () mutable {
		
			if (is_shutdown) return false;
			
			is_shutdown=true;
			
			//	Fail all remaining receives
			for (auto & pair : sends) pair.second->Fail();
			
			return true;
		
		})) ::shutdown(socket,SD_BOTH);
	
	}
	
	
	void Connection::complete (DWORD code) {
		
		//	If the error code is not zero,
		//	we have some work to do
		if (code!=0) {

			//	Set the reason appropriately
			reason_lock.Execute([&] () mutable {
			
				//	We only set the reason if one
				//	isn't already set -- i.e. the
				//	first error reported is always the
				//	one that is regarded
				if (!set_reason) {
				
					reason.Construct(GetErrorMessage(code));
					ex=GetError(code);
					
					set_reason=true;
				
				}
			
			});
			
			//	Shutdown
			shutdown();
			
		}
		
		//	Proceed to remove this from the
		//	handler only if we're the last
		//	ones out
		if ((--pending)==0) {
		
			//	Make sure the socket is shutdown
			shutdown();
			
			//	Maintain statistics within
			//	the handler
			++handler.Disconnected;
			
			//	Fire event
			//	Fire event asynchronously
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wpedantic"
			handler.Enqueue([
				callback=std::move(disconnect),
				event=DisconnectEvent{
					handler.Get(this),
					std::move(reason),
					std::move(ex)
				}
			] () mutable {	if (callback) callback(std::move(event));	});
			#pragma GCC diagnostic pop
			
			//	Remove from handler
			//
			//	After this completes THIS OBJECT
			//	MAY BE INVALIDATED, WE CANNOT
			//	ACCESS ANYTHING OR DO ANYTHING
			//	SAFELY EXCEPT RETURN
			handler.Remove(this);
		
		}
	
	}


	void Connection::receive (Packet packet) {
	
		//	Did the receive operation succeed?
		if (packet.Result) {
		
			//	YES
			
			//	Was nothing received?
			if (packet.Count==0) {
			
				//	This indicates a graceful end
				//	of stream
			
				complete();
				return;
			
			}
			
			//	Adjust the number of bytes in the
			//	receive buffer accordingly
			recv.Buffer.SetCount(
				recv.Buffer.Count()+
				packet.Count
			);
			//	Maintain statistics
			received+=packet.Count;
			handler.Received+=packet.Count;
			
			//	Fire asynchronous event
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wpedantic"
			handler.Enqueue([this] () mutable {
			
				if (receive_callback) try {
				
					receive_callback(
						ReceiveEvent{
							handler.Get(this),
							recv.Buffer
						}
					);
				
				//	This callback's errors are
				//	not our errors
				} catch (...) {	}
				
				//	Pump a new receive
				try {
				
					auto result=recv.Dispatch(socket);
					if (result!=0) complete(result);
				
				//	If an exception is thrown, we're
				//	in an asynchronous callback, the
				//	handler MUST PANIC THIS WILL LEAVE
				//	IT IN AN INCONSISTENT STATE
				} catch (...) {
				
					handler.Panic(std::current_exception());
					
					throw;
				
				}
			
			});
			#pragma GCC diagnostic pop
		
		} else {
		
			//	NO
			
			complete(packet.Error);
		
		}
	
	}
	
	
	void Connection::send (Packet packet) {
	
		//	Did the send operation succeed?
		if (packet.Result) {
		
			//	YES
			
			//	Maintain statistics
			sent+=packet.Count;
			handler.Sent+=packet.Count;
			
			//	Get the corresponding send handle
			auto handle=sends_lock.Execute([&] () mutable {
			
				auto iter=sends.find(packet.Command);
				
				auto retr=std::move(iter->second);
				
				sends.erase(iter);
				
				return retr;
			
			});
			
			//	Complete
			handle->Complete(handler.Pool);
			
			complete();
		
		} else {
		
			//	NO
			
			complete(packet.Error);
		
		}
	
	}
	
	
	void Connection::connect (Packet packet) {
	
		//	Event to pass through to callback
		ConnectEvent event;
		
		if (packet.Result) {
		
			//	Connection success
			
			event.Conn=handler.Get(this);
		
		} else {
		
			//	Connection failed
			
			event.Reason=GetErrorMessage(packet.Error);
			event.Error=GetError(packet.Error);
		
		}
		
		//	Fire callback
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		handler.Enqueue([
			this,
			callback=std::move(connect_callback),
			event=std::move(event)
		] () mutable {
		
			//	Should we pump a receive after
			//	firing the callback?
			//
			//	I.e., did the connect succeed?
			bool dispatch=!event.Conn.IsNull();
		
			//	Fire callback
			if (callback) try {
				
				callback(std::move(event));
				
			} catch (...) {	}
			
			//	Start the receive loop if necessary
			if (dispatch) try {
			
				auto result=recv.Dispatch(socket);
				if (result!=0) complete(result);
			
			} catch (...) {
			
				//	CRITICAL ERROR
				
				handler.Panic(std::current_exception());
				
				throw;
			
			}
		
		});
		#pragma GCC diagnostic pop
		
		//	Remove from handler on failure
		if (!packet.Result) handler.Remove(this);
	
	}


	void Connection::Complete (Packet packet) {
	
		//	Decide how to proceed
		switch (packet.Command->Type) {
		
			case CommandType::Send:
				send(std::move(packet));
				break;
			
			case CommandType::Receive:
				receive(std::move(packet));
				break;
			
			case CommandType::Connect:
			default:
				connect(std::move(packet));
				break;
		
		}
	
	}
	
	
	void Connection::Connect () {
	
		conn.Dispatch(socket,remote_ip,remote_port);
	
	}
	
	
	void Connection::Attach () {
	
		handler.Port.Attach(socket,this);
	
	}
	
	
	void Connection::Begin () {
	
		auto result=recv.Dispatch(socket);
		if (result!=0) complete(result);
	
	}
	
	
	Connection::Connection (
		SOCKET socket,
		IPAddress remote_ip,
		UInt16 remote_port,
		IPAddress local_ip,
		UInt16 local_port,
		ConnectionHandler & handler,
		ReceiveType receive_callback,
		DisconnectType disconnect,
		ConnectType connect_callback
	)	:	socket(socket),
			handler(handler),
			is_shutdown(false),
			remote_ip(remote_ip),
			remote_port(remote_port),
			local_ip(local_ip),
			local_port(local_port),
			disconnect(std::move(disconnect)),
			receive_callback(std::move(receive_callback)),
			connect_callback(std::move(connect_callback)),
			set_reason(false)
	{
	
		sent=0;
		received=0;
		pending=1;
	
	}
	
	
	Connection::~Connection () noexcept {
	
		//	Make sure the socket is
		//	shutdown
		shutdown();
	
		//	Close the socket
		closesocket(socket);
	
	}
	
	
	SmartPointer<SendHandle> Connection::Send (Vector<Byte> buffer) {
	
		//	Create a send handle to represent
		//	this send operation
		auto handle=SmartPointer<SendHandle>::Make(std::move(buffer));
		
		//	If attempting to send 0 bytes,
		//	succeed unconditionally (what
		//	does it even mean to send 0
		//	bytes?)
		if (handle->Command.Buffer.Count()==0) {

			handle->SetState(SendState::Sent);
			
			return handle;

		}
		
		//	Lock, add, and send
		if (!sends_lock.Execute([&] () mutable {
		
			if (
				//	If the socket is shutdown
				//	fail at once
				is_shutdown ||
				//	If this would be the only
				//	pending operation, we've
				//	entered a very narrow window
				//	between the last pending I/O
				//	event failing, and the socket
				//	being shutdown, bail out
				//	at once
				((++pending)==1)
			) return false;
			
			//	If this would be the only pending
			//	I/O operation, 
			
			//	Add
			auto pair=sends.emplace(
				&(handle->Command),
				handle
			);
			
			//	If an exception is thrown, we
			//	must roll back
			auto result=handle->Command.Dispatch(socket);
			//	Something went wrong on the
			//	connection
			if (result!=0) {
			
				//	Make sure we roll back the insertion
				//	of the send
				sends.erase(pair.first);
				
				try {
				
					complete(result);
				
				} catch (...) {
				
					//	Something went seriously wrong,
					//	panic the handler
				
					handler.Panic(std::current_exception());
					
					throw;
				
				}
				
				return false;
			
			}
			
			return true;
		
		})) handle->Fail();
		
		return handle;
	
	}


}

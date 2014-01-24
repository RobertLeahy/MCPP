#include <network.hpp>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	void Connection::shutdown (bool synchronous) {
	
		//	The send method checks to see if the
		//	socket is shutdown, therefore we must
		//	lock to synchronize with that
		lock.Execute([&] () {
		
			//	Don't shut the socket down if it's
			//	already shutdown
			if (is_shutdown) return;
			
			if (::shutdown(socket,SHUT_RDWR)==-1) Raise();
			
			is_shutdown=true;
			
			//	Fail all promises
			for (auto & send : sends) send.Completion.Complete(false);
			sends.Clear();
			
			//	Tell the worker thread to update this
			//	file descriptor unless we're running in
			//	the worker thread, in which case we'll
			//	be returning a command to the worker to
			//	remove us
			if (!(synchronous || (updater==nullptr))) updater->Update(socket);
		
		});
	
	}


	void Connection::get_disconnect (FollowUp & f) {
	
		f.Remove=true;
		++f.Disconnected;
	
		//	If the owner of this connection doesn't
		//	want disconnect events, don't bother to
		//	proceed
		if (!disconnect) return;
		
		//	Otherwise form a callback
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		f.Action.Add([
			disconnect=std::move(disconnect),
			ex=std::move(error.Exception),
			message=std::move(error.Message)
		] (SmartPointer<ChannelBase> channel) mutable noexcept {
		
			DisconnectEvent event;
			event.Conn=std::move(channel).Convert<Connection>();
			if (!message.IsNull()) event.Reason=std::move(message);
			event.Error=std::move(ex);
			
			//	Consumer code exceptions do not
			//	concern us
			try {
			
				disconnect(std::move(event));
				
			} catch (...) {	}
			
			return FollowUp{};
		
		});
		#pragma GCC diagnostic pop
		
		//	Shutdown the socket
		shutdown(true);
	
	}
	
	
	bool Connection::read (FollowUp & f) {
	
		//	Loop until every byte has been
		//	read
		Word total=0;
		bool end=false;
		for (;;) {
		
			//	Resize the buffer if necessary
			if (buffer.Capacity()==buffer.Count()) buffer.SetCapacity();
			
			auto result=recv(
				socket,
				buffer.end(),
				buffer.Capacity()-buffer.Count(),
				0
			);
			//	Error checking
			if (result==-1) {
			
				//	That the operation would block is NOT
				//	an error
				if (WouldBlock()) break;
				
				//	That the operation was interrupted is
				//	NOT an error
				if (WasInterrupted()) continue;
				
				//	Actually an error
				Raise();
			
			}
			//	If nothing was read, that's a graceful
			//	shutdown
			if (result==0) {
			
				end=true;
				
				break;
				
			}
			
			//	Something was read
			auto num=static_cast<Word>(result);
			buffer.SetCount(buffer.Count()+num);
			received+=num;
			f.Received+=num;
			total+=num;
		
		}
		
		//	If we read nothing at all,
		//	we're done, that's the end
		//	of the stream
		if (total==0) return false;
		
		//	Set callback if appropriate
		if (receive) {
		
			f.Action.Add([this] (SmartPointer<ChannelBase> channel) mutable noexcept {
			
				//	We don't care about consumer exceptions
				try {
				
					receive(ReceiveEvent{
						std::move(channel).Convert<Connection>(),
						buffer
					});
				
				} catch (...) {	}
				
				pending_recv=false;
				
				return FollowUp{};
			
			});
			
			pending_recv=true;
		
		} else {
		
			//	If the consumer doesn't want to receive
			//	receive events, don't deliver them, just
			//	delete everything we've received
			buffer.Clear();
		
		}
		
		//	We're done.  If the end of stream was
		//	reached, report that, otherwise continue
		//	normally
		return !end;
	
	}
	
	
	void Connection::write (FollowUp & f) {
	
		lock.Execute([&] () {
		
			//	Loop until every send has been
			//	performed or until no more data
			//	can be sent
			for (;;) {
			
				//	If there's nothing more to send,
				//	we're done
				if (sends.Count()==0) return;
				
				//	Get the next send
				auto & s=sends[0];
				
				//	Attempt to send
				auto result=send(
					socket,
					&(s.Buffer[s.Sent]),
					s.Buffer.Count()-s.Sent,
					0
				);
				//	Error checking
				if (result==-1) {
				
					//	That the operation would block is NOT
					//	an error
					if (WouldBlock()) return;
					
					//	That the operation was interrupt is
					//	NOT an error
					if (WasInterrupted()) continue;
					
					//	Actually an error
					Raise();
				
				}
				
				//	Advance the sent count
				auto num=static_cast<Word>(result);
				sent+=num;
				f.Sent+=num;
				s.Sent+=num;
				
				//	Did we send it all?
				if (s.Sent==s.Buffer.Count()) {
					
					//	Complete
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wpedantic"
					f.Action.Add([completion=std::move(s.Completion)] (SmartPointer<ChannelBase>) mutable noexcept {
						
						completion.Complete(true);
						
						return FollowUp{};
						
					});
					#pragma GCC diagnostic pop
					
					sends.Delete(0);
				
				}
			
			}
		
		});
	
	}
	
	
	bool Connection::get_local_endpoint () noexcept {
	
		//	Get the local address the
		//	socket is bound to
		struct sockaddr_storage addr;
		socklen_t len=sizeof(addr);
		if (getsockname(
			socket,
			reinterpret_cast<struct sockaddr *>(&addr),
			&len
		)==-1) return false;
		
		//	Populate/convert information
		auto ep=GetEndpoint(&addr);
		local_ip=ep.IP;
		local_port=ep.Port;
		
		return true;
		
	}
	
	
	void Connection::get_connect (FollowUp & f) {
	
		//	No longer connecting...
		connecting=false;
	
		//	The connection attempt has completed,
		//	see if it was successful
		auto code=GetSocketError(socket);
		if (!(
			//	If unsuccessful do not continue
			(code==0) &&
			//	If we can't get the local address
			//	the socket bound to while connecting,
			//	do not continue
			get_local_endpoint()
		)) {
		
			if (code!=0) SetError(code);
			error.Capture();
			
			f.Remove=true;
			
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wpedantic"
			if (connect) f.Action.Add([
				connect=std::move(connect),
				ex=std::move(error.Exception),
				message=std::move(error.Message)
			] (SmartPointer<ChannelBase> channel) mutable noexcept {
			
				try {
				
					connect(ConnectEvent{
						std::move(channel).Convert<Connection>(),
						std::move(message),
						std::move(ex)
					});
				
				} catch (...) {	}
				
				return FollowUp{};
			
			});
			#pragma GCC diagnostic pop
			
			return;
		
		}
		
		//	Connection completed successfully!
		
		++f.Outgoing;
		
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		if (connect) f.Action.Add([this,connect=std::move(connect)] (SmartPointer<ChannelBase> channel) mutable noexcept {
		
			try {
			
				ConnectEvent event;
				event.Conn=std::move(channel).Convert<Connection>();
				connect(std::move(event));
			
			} catch (...) {	}
			
			//	This ensures data doesn't flow
			//	until callback is complete
			connected=true;
			
			return FollowUp{};
		
		});
		#pragma GCC diagnostic pop
	
	}
	

	FollowUp Connection::Perform (const Notification & n) {
	
		FollowUp retr;
	
		try {
		
			//	If this is an error, disconnect
			//	at once
			if (n.Error()) {
			
				auto code=GetSocketError(socket);
				if (code!=0) {
				
					SetError(code);
					error.Capture();
				
				}
				
				get_disconnect(retr);
				
				return retr;
			
			}
			
			//	If this is an end of stream,
			//	disconnect at once
			if (n.End()) {
			
				get_disconnect(retr);
				
				return retr;
			
			}
			
			//	Are we connected?
			if (connected) {
			
				//	YES
				
				if (n.Readable() && !read(retr)) {
				
					get_disconnect(retr);
					
					return retr;
				
				}
				
				if (n.Writeable()) write(retr);
				
				return retr;
			
			}
			
			//	We're not connected, we need to
			//	complete the connection
			get_connect(retr);
			
		
		} catch (const std::system_error & e) {
		
			error.Capture();
			
			get_disconnect(retr);
		
		}
		
		return retr;
	
	}
	
	
	void Connection::SetUpdater (Updater * updater) {
	
		lock.Execute([&] () {	this->updater=updater;	});
	
	}
	
	
	bool Connection::Update (Notifier & n) {
	
		//	Determine number of pending sends
		//	and whether or not this socket is
		//	shutdown and should therefore be
		//	removed
		Word count;
		if (!lock.Execute([&] () {
		
			if (is_shutdown) return false;
			
			count=sends.Count();
			
			return true;
		
		})) return false;
	
		//	Only do one atomic read
		bool connected=this->connected;
	
		n.Update(
			socket,
			connected && !pending_recv,
			connecting || (connected && (count!=0))
		);
		
		return true;
	
	}
	
	
	Connection::Connection (
		FDType fd,
		IPAddress local_ip,
		UInt16 local_port,
		IPAddress remote_ip,
		UInt16 remote_port,
		ReceiveType receive,
		DisconnectType disconnect
	) noexcept
		:	socket(fd),
			connecting(false),
			is_shutdown(false),
			disconnect(std::move(disconnect)),
			receive(std::move(receive)),
			updater(nullptr),
			local_ip(local_ip),
			local_port(local_port),
			remote_ip(remote_ip),
			remote_port(remote_port)
	{
	
		connected=true;
		pending_recv=false;
		sent=0;
		received=0;
	
	}
	
	
	Connection::Connection (FDType fd, RemoteEndpoint ep) noexcept
		:	socket(fd),
			connecting(true),
			is_shutdown(false),
			disconnect(std::move(ep.Disconnect)),
			receive(std::move(ep.Receive)),
			connect(std::move(ep.Connect)),
			updater(nullptr),
			remote_ip(ep.IP),
			remote_port(ep.Port)
	{
	
		connected=false;
		pending_recv=false;
		sent=0;
		received=0;
	
	}
	
	
	Connection::~Connection () noexcept {
	
		//	Fail all pending sends
		for (auto & send : sends) send.Completion.Complete(false);
		
		//	Close the socket
		close(socket);
	
	}
	
	
	void Connection::Disconnect () {
	
		shutdown(false);
	
	}
	
	
	void Connection::Disconnect (String reason) {
	
		error.Set(std::move(reason));
	
		shutdown(false);
	
	}
	
	
	Promise<bool> Connection::Send (Vector<Byte> buffer) {
	
		//	Create a send buffer
		SendBuffer send(std::move(buffer));
		
		return lock.Execute([&] () {
		
			//	Check to make sure we can send
			if (is_shutdown) {
			
				//	Socket is shutdown, no more
				//	sends
				
				//	Fail the promise at once
				send.Completion.Complete(false);
				
				return std::move(send.Completion);
			
			}
			
			//	We can send
			
			//	Tell the worker that's managing us
			//	that we need to be updated if
			//	applicable
			if (
				(sends.Count()==0) &&
				(updater!=nullptr)
			) updater->Update(socket);
			
			//	Add to send queue
			auto promise=send.Completion;
			sends.Add(std::move(send));
			
			return promise;
		
		});
	
	}


}

#include <network.hpp>
#include <unistd.h>


using namespace MCPP::NetworkImpl;


namespace MCPP {
	
	
	class Socket {
	
		
		private:
			
			
			mutable FDType socket;
			Endpoint ep;
			
			
			void destroy () noexcept {
			
				if (socket!=-1) {
				
					close(socket);
					
					socket=-1;
					
				}
				
			}
			
			
		public:
		
		
			Socket () = delete;
			Socket (FDType socket, Endpoint ep) noexcept : socket(socket), ep(std::move(ep)) {	}
			~Socket () noexcept {
			
				destroy();
				
			}
			Socket (const Socket &) = delete;
			Socket & operator = (const Socket &) = delete;
			Socket (Socket && other) noexcept : socket(other.socket), ep(std::move(other.ep)) {
			
				other.socket=-1;
				
			}
			Socket & operator = (Socket && other) noexcept {
			
				destroy();
				
				if (&other!=this) {
					
					socket=other.socket;
					other.socket=-1;
					ep=std::move(other.ep);
					
				}
				
				return *this;
				
			}
			
			
			FDType Get () noexcept {
			
				auto retr=socket;
				socket=-1;
				
				return retr;
				
			}
			
			
			IPAddress IP () const noexcept {
			
				return ep.IP;
				
			}
			
			
			UInt16 Port () const noexcept {
			
				return ep.Port;
				
			}
		
		
	};
	
	
	FollowUp ListeningSocket::Perform (const Notification & n) {
	
		FollowUp retr;
		
		if (do_shutdown) {
		
			retr.Remove=true;
			
			return retr;
			
		}
		
		for (;;) {
		
			struct sockaddr_storage addr;
			socklen_t len=sizeof(addr);
			auto socket=accept(
				this->socket,
				reinterpret_cast<struct sockaddr *>(&addr),
				&len
			);
			//	Check for failure
			if (socket==-1) {
			
				//	Was the failure just because
				//	the operation would block?
				if (WouldBlock()) break;
				
				//	Were we interrupted?
				if (WasInterrupted()) continue;
				
				//	Actually an error
				Raise();
				
			}
			
			//	Increment statistic
			++retr.Accepted;
			
			auto ep=GetEndpoint(&addr);
			SmartPointer<Connection> conn;
			try {
				
				conn=SmartPointer<Connection>::Make(
					socket,
					ep.IP,
					ep.Port,
					this->ep.IP,
					this->ep.Port,
					this->ep.Receive,
					this->ep.Disconnect
				);
				
			} catch (...) {
			
				//	Make sure the socket gets
				//	closed
				close(socket);
				
				throw;
				
			}
			
			//	Fire off a callback to handle this
			//	incoming connections
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wpedantic"
			retr.Action.Add([
				this,
				conn=std::move(conn),
				socket
			] (SmartPointer<ChannelBase> channel) mutable {
				
				FollowUp f;
				
				//	Should this connection be allowed?
				if (this->ep.Accept) {
				
					try {
						
						//	If the connection is not admitted,
						//	return at once, the connection will
						//	automatically be dropped by the Socket
						//	RAII container
						if (!this->ep.Accept(AcceptEvent{
							conn->IP(),
							conn->Port(),
							this->ep.IP,
							this->ep.Port
						})) return f;
						
					} catch (...) {
					
						//	Consider throwing the same thing
						//	as returning false
						return f;
						
					}
					
				}
				
				//	The connection is allowed
				++f.Incoming;
				
				//	Fire the connect handler
				if (this->ep.Connect) {
				
					ConnectEvent event;
					event.Conn=conn;
					
					try {
						
						this->ep.Connect(std::move(event));
						
					//	Ignore user exceptions
					} catch (...) {	}
					
				}
				
				//	Add the connection
				f.Add=Channel{
					socket,
					std::move(conn).Convert<ChannelBase>()
				};
				
				return f;
				
			});
			#pragma GCC diagnostic pop
			
		}
		
		return retr;
		
	}
	
	
	void ListeningSocket::SetUpdater (Updater * updater) {
		
		lock.Execute([&] () {	this->updater=updater;	});
		
	}
	
	
	bool ListeningSocket::Update (Notifier & n) {
		
		//	The listening socket may persist
		//	long past when it is actually removed,
		//	so make sure that events for its socket
		//	are not actually propagated through to
		//	the notifier
		if (do_shutdown) {
		
			if (!detached) {
				
				n.Detach(socket);
				
				detached=true;
				
			}
			
			return false;
			
		}
		
		n.Update(socket,true,false);
		
		return true;
		
	}


	ListeningSocket::ListeningSocket (FDType socket, LocalEndpoint ep) noexcept
		:	socket(socket),
			updater(nullptr),
			detached(false),
			ep(std::move(ep))
	{
	
		do_shutdown=false;
		
	}
	
	
	ListeningSocket::~ListeningSocket () noexcept {
	
		close(socket);
		
	}
	
	
	void ListeningSocket::Shutdown () noexcept {
		
		//	Attempt to issue shutdown command
		//	if already shutdown, don't do
		//	anything
		bool expected=false;
		if (!do_shutdown.compare_exchange_strong(expected,true)) return;
		
		//	Tell worker to update this channel
		//	so it'll be removed
		lock.Execute([&] () {
			
			//	If there's no updater, no need
			//	to proceed
			if (updater==nullptr) return;
					 
			updater->Update(socket);
			
		});	
		
	}
	
	
}

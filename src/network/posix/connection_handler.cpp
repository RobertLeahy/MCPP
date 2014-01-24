#include <hardware_concurrency.hpp>
#include <network.hpp>
#include <scope_guard.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <unistd.h>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	void ConnectionHandler::do_panic () noexcept {
	
		if (panic) try {
		
			panic(std::current_exception());
		
		} catch (...) {	}
		
		std::abort();
	
	}


	void ConnectionHandler::complete_callback () noexcept {
	
		lock.Execute([&] () {	if ((--callbacks)==0) wait.WakeAll();	});
	
	}


	template <typename T, typename... Args>
	auto ConnectionHandler::enqueue (T && callback, Args &&... args) -> Promise<decltype(callback(std::forward<Args>(args)...))> {
		
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		return lock.Execute([this,callback=std::bind(std::forward<T>(callback),std::forward<Args>(args)...)] () mutable {
		
			++callbacks;
			
			try {
				
				return pool.Enqueue([this,callback=std::move(callback)] () mutable {
					
					auto guard=AtExit([this] () mutable noexcept {	complete_callback();	});
					
					try {
					
						return callback();
						
					} catch (...) {
					
						do_panic();
						
					}
					
				});
			
			} catch (...) {
			
				--callbacks;
				
				throw;
			
			}
		
		});
		#pragma GCC diagnostic pop
	
	}


	void ConnectionHandler::handle (const FollowUp & f) noexcept {
	
		//	Increment statistics
		sent+=f.Sent;
		received+=f.Received;
		incoming+=f.Incoming;
		outgoing+=f.Outgoing;
		accepted+=f.Accepted;
		disconnected+=f.Disconnected;
	
	}
	
	
	void ConnectionHandler::add (NetworkImpl::FDType fd, SmartPointer<ChannelBase> channel) {
	
		//	Scan the workers to find the one
		//	with the fewest managed connections
		Word i=0;
		Word min=workers[0].Count;
		for (Word n=1;n<workers.Count();++n) {
		
			//	Load atomic
			Word temp=workers[n].Count;
			
			if (temp<min) {
			
				i=n;
				min=temp;
			
			}
		
		}
		
		//	We've found the worker with the fewest
		//	managed connections, they get this
		//	connection
		workers[i].Control.Send(
			Command(
				CommandType::Add,
				fd,
				std::move(channel)
			)
		);
	
	}


	void ConnectionHandler::handle (Worker & self, FDType fd, SmartPointer<ChannelBase> & channel, FollowUp f, bool synchronous) {
	
		//	Maintain statistics
		handle(f);
		
		//	Add connection if necessary
		if (f.Add.Impl) add(
			f.Add.FD,
			std::move(f.Add.Impl)
		);
		
		//	Run follow up if necessary
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		for (auto & callback : f.Action) enqueue([
			this,
			&self,
			fd,
			channel,
			callback=std::move(callback)
		] () mutable {
		
			handle(
				self,
				fd,
				channel,
				callback(channel),
				false
			);
		
		});
		#pragma GCC diagnostic pop
		
		//	Update/remove if necessary
		if (synchronous) {
		
			if ((f.Remove) || (!channel->Update(self.N))) {
			
				self.FDs.erase(fd);
				--self.Count;
			
			}
		
		} else {
		
			self.Control.Send(
				Command(
					CommandType::Update,
					fd
				)
			);
			
		}
	
	}
	
	
	bool ConnectionHandler::process_control (Worker & self) {
	
		//	Loop until there's no more commands
		//	to handle
		for (;;) {
		
			//	Get a command
			auto command=self.Control.Receive();
			//	If the command is null, we're
			//	done
			if (!command) return true;
			
			switch (command->Type) {
			
				case CommandType::Add:
					self.N.Attach(command->FD);
					command->Impl->SetUpdater(&self);
					command->Impl->Update(self.N);
					self.FDs.emplace(
						command->FD,
						std::move(command->Impl)
					);
					break;
					
				case CommandType::Update:{
					auto iter=self.FDs.find(command->FD);
					//	If the channel is no more, just
					//	ignore
					if (iter==self.FDs.end()) continue;
					if (!iter->second->Update(self.N)) self.FDs.erase(command->FD);
				}break;
				
				case CommandType::Shutdown:
				default:return false;
			
			}
		
		}
	
	}
	
	
	bool ConnectionHandler::process_notification (Worker & self, Notification & n) {
	
		//	Get the file descriptor for this notification
		auto fd=n.FD();
		
		//	Is this notification for the control channel?
		if (self.Control.Is(fd)) return process_control(self);
		
		//	Process a notification for a channel
		auto iter=self.FDs.find(fd);
		//	If the channel-in-question is no more,
		//	ignore
		if (iter==self.FDs.end()) return true;
		auto & channel=iter->second;
		
		//	Perform action
		handle(
			self,
			fd,
			channel,
			channel->Perform(n)
		);
		
		//	We only return false on shutdown, which
		//	can only come from control, therefore
		//	we unconditionally return true here
		return true;
	
	}
	
	
	//	Maximum number of events which may be
	//	dequeued
	constexpr Word max_dequeue=256;


	void ConnectionHandler::worker (Worker & self) {
		
		//	We don't want to receive SIGPIPEs
		sigset_t set;
		if (
			(sigemptyset(&set)==-1) ||
			(sigaddset(&set,SIGPIPE)==-1) ||
			(pthread_sigmask(SIG_BLOCK,&set,nullptr)==-1)
		) Raise();
	
		for (;;) {
		
			//	Get a notification
			Notification n;
			auto num=self.N.Wait(n);
			
			//	If -- for whatever reason -- no notification
			//	was dequeued, just try again
			if (num==0) continue;
			
			//	Process the notification
			if (!process_notification(self,n)) return;
		
		}
		
	}
	
	
	void ConnectionHandler::worker_func (Word i) noexcept {
	
		//	Wait for startup
		lock.Execute([&] () {	while (!proceed) wait.Sleep(lock);	});
		
		//	Did startup succeed?
		if (shutdown) return;
	
		try {
			
			auto & self=workers[i];
		
			worker(self);
			
			//	Remove pointer to the worker_func
			//	control block from all channels
			for (auto & pair : self.FDs) pair.second->SetUpdater(nullptr);
		
		} catch (...) {
		
			do_panic();
		
		}
	
	}
	
	
	ConnectionHandler::ConnectionHandler (ThreadPool & pool, Nullable<Word> num_workers, PanicType panic)
		:	pool(pool),
			callbacks(0),
			proceed(false),
			shutdown(false),
			panic(std::move(panic))
	{
	
		//	Determine how many threads will be
		//	used
		Word num;
		if (num_workers.IsNull()) num=HardwareConcurrency();
		else num=*num_workers;
		if (num==0) num=1;
		
		//	Initialize statistics
		sent=0;
		received=0;
		incoming=0;
		outgoing=0;
		accepted=0;
		disconnected=0;
		
		//	Create worker blocks
		workers=Vector<Worker>(num);
		Word i;
		for (i=0;i<num;++i) workers.EmplaceBack();
		
		//	Spawn workers
		try {
		
			for (i=0;i<num;++i) workers[i].T=Thread([this,i] () mutable {	worker_func(i);	});
		
		} catch (...) {
		
			//	We need to shut everything
			//	down
			
			lock.Execute([&] () {
			
				proceed=true;
				shutdown=true;
				
				wait.WakeAll();
			
			});
			
			for (Word n=0;n<i;++n) workers[n].T.Join();
			
			throw;
		
		}
		
		//	Successfully started up, tell all
		//	workers to begin
		lock.Execute([&] () {
		
			proceed=true;
			
			wait.WakeAll();
		
		});
	
	}
	
	
	ConnectionHandler::~ConnectionHandler () noexcept {
	
		try {
		
			//	Tell all workers to shutdown
			for (auto & worker : workers) worker.Control.Send(Command(CommandType::Shutdown));
		
		} catch (...) {
		
			do_panic();
		
		}
		
		//	Wait for workers to shutdown
		for (auto & worker : workers) worker.T.Join();
		
		//	Wait for all callbacks to complete
		lock.Execute([&] () {	while (callbacks!=0) wait.Sleep(lock);	});
	
	}
	
	
	void ConnectionHandler::Connect (RemoteEndpoint ep) {
	
		//	Get a socket
		auto socket=GetSocket(ep.IP.IsV6());
		
		//	We're responsible for the socket
		//	now
		SmartPointer<ChannelBase> channel;
		try {
			
			//	Make socket non-blocking
			SetBlocking(socket,false);
		
			//	Get a sockaddr_storage from the IP
			//	address we're to connect to
			struct sockaddr_storage addr;
			std::memset(&addr,0,sizeof(addr));
			ep.IP.ToOS(&addr,ep.Port);
			
			//	Try to connect
			if (
				(connect(
					socket,
					reinterpret_cast<struct sockaddr *>(&addr),
					sizeof(addr)
				)==-1) &&
				//	Make sure the "error" just isn't
				//	a non-blocking connect
				(errno!=EINPROGRESS)
			) Raise();
			
			//	Wrap it
			channel=SmartPointer<ChannelBase>::Make<Connection>(socket,std::move(ep));
			
		} catch (...) {
		
			close(socket);
			
			throw;
		
		}
		
		//	Smart pointer is now managing the
		//	connection, so we're safe
		
		//	Add the channel to a worker thread
		add(socket,std::move(channel));
	
	}
	
	
	SmartPointer<ListeningSocket> ConnectionHandler::Listen (LocalEndpoint ep) {
	
		//	Get a socket
		auto socket=GetSocket(ep.IP.IsV6());
		
		//	We're responsible for the socket
		//	now
		SmartPointer<ListeningSocket> listening;
		try {
			
			//	Make socket non-blocking
			SetBlocking(socket,false);
			
			//	Setup the address/port that we're
			//	going to bind to
			struct sockaddr_storage addr;
			ep.IP.ToOS(&addr,ep.Port);
			//	Bind and listen
			if (
				(bind(
					socket,
					reinterpret_cast<struct sockaddr *>(&addr),
					sizeof(addr)
				)==-1) ||
				(listen(
					socket,
					SOMAXCONN
				)==-1)
			) Raise();
			
			//	Wrap in a ListeningSocket object
			listening=SmartPointer<ListeningSocket>::Make(socket,std::move(ep));
			
		} catch (...) {
		
			close(socket);
			
			throw;
			
		}
		
		//	Socket is wrapped and therefore
		//	safe
		
		//	Add to handler
		add(socket,listening);
		
		return listening;
		
	}


}

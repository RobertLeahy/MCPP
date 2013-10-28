#include <hardware_concurrency.hpp>
#include <network.hpp>
#include <signal.h>
#include <unistd.h>
#include <stdexcept>
#include <utility>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	//	The number of notifications each thread
	//	will attempt to dequeue each time it
	//	loops
	constexpr Word num_notifications=256;
	
	
	template <typename T, typename... Args>
	void ConnectionHandler::enqueue (const T & callback, Args &&... args) {
	
		lock.Acquire();
		++pending;
		lock.Release();
		
		//	Make sure the count gets decremented
		//	if we throw
		try {
		
			//	Dispatch asynchronous callback
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wpedantic"
			pool.Enqueue([this,callback=std::bind(callback,std::forward<Args>(args)...)] () mutable {
			
				try {
				
					callback();
				
				} catch (...) {
				
					lock.Acquire();
					--pending;
					wait.WakeAll();
					lock.Release();
				
					throw;
				
				}
			
			});
			#pragma GCC diagnostic pop
		
		} catch (...) {
		
			lock.Acquire();
			--pending;
			wait.WakeAll();
			lock.Release();
			
			throw;
		
		}
	
	}
	
	
	WorkerThread & ConnectionHandler::select () noexcept {
	
		Word index=0;
		Word count;
		for (Word i=0;i<workers.Count();++i) {
		
			auto curr=workers[i].Item<0>()->Count();
			
			if (i==0) {
			
				count=curr;
			
			} else if (count>curr) {
			
				count=curr;
				index=i;
			
			}
		
		}
		
		return *workers[index].Item<0>();
	
	}
	
	
	ListeningSocket & ConnectionHandler::get (int fd) noexcept {
	
		auto iter=listening.find(fd);
		
		return iter->second;
	
	}
	
	
	void ConnectionHandler::make (ListeningSocket::ConnectionType t) {
	
		++connected;
	
		//	Dispatch an asynchronous callback to
		//	perform processing to determine whether
		//	this connection should be allowed
		
		//	We're responsible for the socket's
		//	lifetime until the asynchronous callback
		//	has been successfully dispatched
		try {
		
			enqueue([=] () mutable {
			
				//	Default to false, as we assume
				//	throwing an exception means
				//	that the connection should be
				//	declined...
				bool accepted=false;
				try {
				
					if (accept) accepted=this->accept(
						t.Item<1>(),
						t.Item<2>(),
						t.Item<3>(),
						t.Item<4>()
					);
					//	If no callback was provided,
					//	accept all connections
					else accepted=true;
				
				//	Ignore callback's exceptions,
				//	they're not our problem
				} catch (...) {	}
				
				//	If the connection was not accepted,
				//	close the connection and move
				//	on
				if (!accepted) {
				
					close(t.Item<0>());
					
					return;
				
				}
				
				//	Connection was accepted
				
				++accepted;
				
				//	Find a worker thread to take
				//	this connection
				auto & worker=this->select();
				
				SmartPointer<Connection> conn;
				//	Remember to manage socket's lifetime
				try {
				
					conn=SmartPointer<Connection>::Make(
						t.Item<0>(),
						t.Item<1>(),
						t.Item<2>(),
						worker,
						t.Item<3>(),
						t.Item<4>()
					);
				
				} catch (...) {
				
					close(t.Item<0>());
					
					//	We're in an asynchronous callback,
					//	make sure this gets passed on
					//	appropriately
					do_panic(std::current_exception());
					
					throw;
				
				}
				
				//	Socket has been installed in an object,
				//	so we no longer need to concern ourselves
				//	with manually managing its lifetime
				
				//	If the user callback to connect throws,
				//	we don't catch it, we let this connection
				//	die
				if (connect) connect(conn);
				
				//	We care about exceptions now, nothing
				//	should throw, especially since the
				//	connection hasn't been installed into
				//	a worker, but the user thinks that the
				//	connection is active
				try {
				
					//	Add the client to the chosen worker
					worker.AddSocket(
						t.Item<0>(),
						std::move(conn)
					);
				
				} catch (...) {
				
					do_panic(std::current_exception());
					
					throw;
				
				}
			
			});
		
		} catch (...) {
		
			close(t.Item<0>());
			
			throw;
		
		}
	
	}
	
	
	void ConnectionHandler::do_panic (std::exception_ptr ex) noexcept {
	
		if (panic) try {
		
			panic(ex);
		
		//	We're already panicking, can't
		//	do anything about this
		} catch (...) {	}
	
	}
	
	
	void ConnectionHandler::kill (SmartPointer<Connection> conn, WorkerThread & self) {
	
		//	Shutdown the connection
		conn->Shutdown();
		
		//	Remove the connection from the
		//	list of connections this thread
		//	is managing
		self.Remove(conn->Socket());
		
		//	Get the reason (if any) for the
		//	disconnect
		String reason(conn->Reason());
		
		//	Fire disconnect callback asynchronously
		if (disconnect) enqueue(
			disconnect,
			std::move(conn),
			std::move(reason)
		);
	
	}
	
	
	void ConnectionHandler::thread_func_init (Word index) noexcept {
	
		//	Wait for other workers to be
		//	started
		startup.Acquire();
		while (!proceed) startup_wait.Sleep(startup);
		startup.Release();
		
		//	If startup of other workers failed,
		//	end at once
		if (!success) return;
		
		//	On exception, we panic
		try {
		
			//	Ignore SIGPIPEs
			sigset_t block;
			if (
				(sigemptyset(&block)==-1) ||
				(sigaddset(&block,SIGPIPE)==-1) ||
				(pthread_sigmask(SIG_BLOCK,&block,nullptr)==-1)
			) Raise();
		
			//	Invoke inner worker function
			thread_func(*workers[index].Item<0>());
		
		} catch (...) {
		
			do_panic(std::current_exception());
			
			throw;
		
		}
	
	}


	void ConnectionHandler::thread_func (WorkerThread & self) {
	
		//	Create an array of notifications
		Notification notifications [num_notifications];
		
		//	Get a reference to this thread's
		//	notifier
		auto & notifier=self.Get();
	
		//	Loop forever
		for (;;) {
		
			//	Clear ignored sockets
			self.Clear();
		
			//	Fetch events
			auto num=notifier.Wait(notifications,num_notifications);
			
			//	Loop over all dequeued notifications
			for (Word i=0;i<num;++i) {
			
				//	Current notification
				auto & event=notifications[i];
				
				//	Socket that generated this
				//	notification
				auto socket=event.Socket();
				
				//	Handle messages from control
				if (self.IsControl(socket)) {
				
					//	Get messages from control until
					//	we can't anymore
					for (;;) {
					
						auto msg=self.GetSocket();
						
						//	If there are no more messages
						//	from control, stop looping
						if (msg.IsNull()) break;
						
						//	Do we have this connection?
						auto conn=self.Get(*msg);
						
						//	If yes, we update, otherwise
						//	it must be a shutdown message
						//	from control
						if (conn.IsNull()) return;
						
						//	Update connection, if the connection
						//	has been closed, kill it
						if (!conn->Update()) kill(std::move(conn),self);
					
					}
					
					//	Next event
					continue;
				
				}
				
				//	Skip messages from ignored sockets
				if (self.IsIgnored(socket)) continue;
				
				//	Message isn't from control, is it from
				//	one of the clients that this worker is
				//	managing?
				auto conn=self.Get(socket);
				if (conn.IsNull()) {
				
					//	No, it must be from one of the
					//	handler-wide listening sockets
					
					auto & l=get(socket);
					
					//	Loop until we can't accept anymore
					//	connections
					for (;;) {
					
						auto accepted=l.Accept();
						
						if (accepted.IsNull()) break;
						
						make(std::move(*accepted));
					
					}
					
					//	Next event
					continue;
				
				}
				
				//	Client socket associated with this
				//	worker
				
				//	Is there an error?
				if (event.Error()) {
				
					//	Kill the connection
					kill(std::move(conn),self);
				
					continue;
				
				}
				
				//	Can we read?
				if (event.Readable()) {
				
					//	Perform read
					auto read=conn->Receive();
					
					//	If we read nothing, that means
					//	other end went away.
					if (read==0) {
					
						kill(std::move(conn),self);
						
						//	Skip remainder of processing
						//	for this event -- we don't
						//	care if we can write to a broken
						//	connection
						continue;
					
					}
					
					//	Dispatch asynchronous callback
					enqueue([this,conn] () mutable {
					
						try {
						
							if (recv) recv(conn,conn->Get());
						
						//	We don't care about this
						//	callback's exceptions
						} catch (...) {	}
						
						try {
						
							//	Callback is done, receive data
							//	once more
							conn->CompleteReceive();
						
						} catch (...) {
						
							do_panic(std::current_exception());
							
							throw;
						
						}
					
					});
					
					received+=read;
				
				}
				
				//	Can we write?
				if (event.Writeable()) {
				
					//	Perform send
					auto wrote=conn->Send(pool);
					
					//	If we sent nothing, that means
					//	something went wrong -- the socket
					//	was reported as being writeable
					if (wrote==0) {
					
						kill(std::move(conn),self);
						
						continue;
					
					}
					
					sent+=wrote;
				
				}
			
			}
		
		}
	
	}
	
	
	ConnectionHandler::~ConnectionHandler () noexcept {
	
		//	Tell all workers to shut down
		
		for (auto & t : workers) t.Item<0>()->Shutdown();
		for (auto & t : workers) t.Item<1>().Join();
		
		//	Wait for all pending asynchronous
		//	callbacks
		lock.Acquire();
		while (pending!=0) wait.Sleep(lock);
		lock.Release();
	
	}
	
	
	static Word num_workers_helper (const Nullable<Word> & in) noexcept {
	
		Word retr=in.IsNull() ? HardwareConcurrency() : *in;
		
		if (retr==0) retr=1;
		
		return retr;
	
	}
	
	
	ConnectionHandler::ConnectionHandler (
		const Vector<Tuple<IPAddress,UInt16>> & binds,
		AcceptCallback accept,
		ConnectCallback connect,
		DisconnectCallback disconnect,
		ReceiveCallback recv,
		PanicCallback panic,
		ThreadPool & pool,
		Nullable<Word> num_workers
	)	:	pool(pool),
			recv(std::move(recv)),
			disconnect(std::move(disconnect)),
			accept(std::move(accept)),
			connect(std::move(connect)),
			panic(std::move(panic)),
			workers(num_workers_helper(num_workers)),
			pending(0),
			proceed(false),
			success(true)
	{
	
		//	Initialize statistics
		sent=0;
		received=0;
		connected=0;
		accepted=0;
		
		//	Create listening sockets
		for (const auto & ep : binds) {
		
			//	Create listening socket
			ListeningSocket l(
				ep.Item<0>(),
				ep.Item<1>()
			);
			
			//	Get the socket it created
			int socket=l.Get();
			
			//	Emplace in hash map
			listening.emplace(
				socket,
				std::move(l)
			);
		
		}
		
		//	Create worker threads
		Word created=0;
		try {
		
			for (Word i=0;i<num_workers_helper(num_workers);++i) {
			
				std::unique_ptr<WorkerThread> worker(new WorkerThread());
				
				//	We associated all listening threads
				//	with each worker
				for (auto & l : listening) l.second.Attach(worker->Get());
				
				workers.EmplaceBack(
					std::move(worker),
					Thread([=] () mutable {	thread_func_init(i);	})
				);
				
				++created;
			
			}
			
		} catch (...) {
		
			//	Tell whichever workers that started
			//	that they need to shutdown
			if (created!=0) {
			
				success=false;
			
				startup.Acquire();
				proceed=true;
				startup_wait.WakeAll();
				startup.Release();
				
				for (Word i=0;i<created;++i) workers[i].Item<1>().Join();
			
			}
			
			throw;
		
		}
		
		//	Startup success, release
		//	workers
		
		startup.Acquire();
		proceed=true;
		startup_wait.WakeAll();
		startup.Release();
	
	}


}

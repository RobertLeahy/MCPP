#include <connection_manager.hpp>
#include <utility>


namespace MCPP {


	ConnectionManager::ConnectionManager (
		ConnectCallback connect,
		DisconnectCallback disconnect,
		ReceiveCallback recv,
		LogType log,
		PanicType panic,
		ThreadPool & pool
	)
		:	cleanup_count(0),
			connect(std::move(connect)),
			disconnect(std::move(disconnect)),
			panic(std::move(panic)),
			recv(std::move(recv)),
			log(std::move(log)),
			pool(&pool),
			stop(false),
			watcher(thread_func,this)
	{	}


	ConnectionManager::~ConnectionManager () noexcept {
	
		//	Stop watcher
		lock.Acquire();
		stop=true;
		wait.WakeAll();
		lock.Release();
		
		watcher.Join();
	
		//	Clean up all handlers
	
		handlers_lock.Acquire();
		
		try {
		
			while (handlers.Count()!=0) handlers.Delete(0);
		
		} catch (...) {	}
	
		handlers_lock.Release();
		
		//	Wait for any pending cleanup
		//	tasks
		cleanup_lock.Acquire();
		
		while (cleanup_count!=0) cleanup_wait.Sleep(cleanup_lock);
		
		cleanup_lock.Release();
	
	}
	
	
	void ConnectionManager::Add (Socket socket, const IPAddress & ip, UInt16 port) {
	
		SmartPointer<Connection> conn(
			SmartPointer<Connection>::Make(
				std::move(socket),
				ip,
				port
			)
		);
		
		//	Fire connection callback
		//	we do this synchronously
		//	otherwise it would be possible
		//	for a message to be received
		//	from a client that hadn't
		//	yet been added to be
		//	handled, which would cause
		//	odd behaviour (to say the
		//	least)
		try {
		
			try {
			
				connect(conn);
			
			} catch (const std::exception & e) {
			
				//	TODO: Log
				
				throw;
			
			} catch (...) {
			
				//	TODO: Log
				
				throw;
			
			}
			
		} catch (...) {
		
			try {	panic();	} catch (...) {	}
			
			throw;
		
		}
		
		pending_lock.Acquire();
		
		try {
		
			pending.Add(std::move(conn));
		
		} catch (...) {
		
			pending_lock.Release();
		
			throw;
		
		}
		
		pending_lock.Release();
	
	}
	
	
	Nullable<SmartPointer<Connection>> ConnectionManager::Dequeue () {
	
		Nullable<SmartPointer<Connection>> returnthis;
	
		pending_lock.Acquire();
		
		try {
		
			if (pending.Count()!=0) {
			
				returnthis=std::move(pending[0]);
				
				pending.Delete(0);
			
			}
		
		} catch (...) {
		
			pending_lock.Release();
			
			throw;
		
		}
		
		pending_lock.Release();
		
		return returnthis;
	
	}
	
	
	static const String watcher_error("Error in pending connection monitoring thread");
	static const String watcher_error_what("Error in pending connection monitoring thread: {0}");
	
	
	void ConnectionManager::thread_func (void * ptr) {
	
		ConnectionManager * cm=reinterpret_cast<ConnectionManager *>(ptr);
	
		try {
		
			try {
			
				cm->thread_func_impl();
			
			} catch (const std::exception & e) {
			
				try {
				
					cm->log(
						String::Format(
							watcher_error_what,
							e.what()
						),
						Service::LogType::Error
					);
				
				} catch (...) {	}
			
				throw;
			
			} catch (...) {
			
				try {
				
					cm->log(
						watcher_error,
						Service::LogType::Error
					);
				
				} catch (...) {	}
			
				throw;
			
			}
		
		} catch (...) {
		
			try {
			
				cm->panic();
			
			} catch (...) {	}
			
			throw;
		
		}
	
	}
	
	
	static const Word timeout=50;
	
	
	void ConnectionManager::thread_func_impl () {
	
		#ifdef DEBUG
		log("Watcher up",Service::LogType::Information);
		
		try {
		#endif
	
		//	Loop until told to stop
		for (;;) {
		
			//	Check stop and sleep if
			//	not stopping
			lock.Acquire();
			
			try {
			
				if (!stop) wait.Sleep(lock,timeout);
				
				if (stop) {
				
					lock.Release();
					
					break;
				
				}
			
			} catch (...) {
			
				lock.Release();
				
				throw;
			
			}
			
			lock.Release();
		
			//	Wait
			Thread::Sleep(timeout);
			
			//	Determine whether we should spawn
			//	a new handler
			pending_lock.Acquire();
			
			try {
			
				//	If there are no pending connections,
				//	it'd be pointless to go on
				if (pending.Count()!=0) {
				
					//	Check the amount of
					//	capacity in the running
					//	handlers
					
					handlers_lock.Acquire();
					
					try {
					
						SafeWord available=0;
						
						for (SmartPointer<ConnectionHandler> & handler : handlers) available+=SafeWord(handler->Available());
						
						//	Insufficient handlers,
						//	spawn another
						if (Word(available)<pending.Count()) {
						
							handlers.EmplaceBack(
								SmartPointer<ConnectionHandler>::Make(*this)
							);
						
						}
					
					} catch (...) {
					
						handlers_lock.Release();
						
						throw;
					
					}
					
					handlers_lock.Release();
				
				}
			
			} catch (...) {
			
				pending_lock.Release();
			
				throw;
			
			}
			
			pending_lock.Release();
		
		}
		
		#ifdef DEBUG
		} catch (...) {
		
			log("Watcher down",Service::LogType::Information);
		
			throw;
		
		}
		
		log("Watcher down",Service::LogType::Information);
		#endif
	
	}


}

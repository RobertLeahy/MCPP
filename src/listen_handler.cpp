#include <listen_handler.hpp>


namespace MCPP {


	//	Messages
	static const String log_template="{0}: {1}";
	static const String could_not_bind="Failed to bind to {0} : {1}";
	static const String no_binds="Could not bind to any interfaces";
	static const String unexpected_exception="Unexpected exception thrown";
	static const String unexpected_exception_what_template="{0}: {1}";
	
	
	//	Constants
	static const Word backlog=std::numeric_limits<Word>::max();
	static const Word select_timeout=50;


	ListenHandler::ListenHandler (
		const String & desc,
		const Vector<Tuple<IPAddress,UInt16>> & binds,
		ThreadPool & pool,
		const OnConnectType & on_connect,
		const ConnectType & connect,
		const LogType & log,
		const PanicType & panic
	) : desc(desc), sockets(binds.Count()), pool(&pool), on_connect(on_connect), connect(connect), log(log), panic(panic), stop(false) {
	
		//	Attempt to bind
		for (const Tuple<IPAddress,UInt16> & t : binds) {
		
			try {
		
				sockets.EmplaceBack(
					Socket::Type::Stream,
					t.Item<0>().IsV6()
				);
				
				Socket & sock=sockets[sockets.Count()-1];
				
				sock.Bind(
					t.Item<0>(),
					t.Item<1>()
				);
				
				sock.Listen(backlog);
				
			} catch (const SocketException & e) {
			
				try {
			
					log(
						String::Format(
							log_template,
							desc,
							String::Format(
								could_not_bind,
								t.Item<0>(),
								t.Item<1>()
							)
						),
						Service::LogType::Warning
					);
					
				} catch (...) {	}
			
			}
		
		}
		
		if (sockets.Count()==0) {
		
			try {
		
				log(
					String::Format(
						log_template,
						desc,
						no_binds
					),
					Service::LogType::Error
				);
				
			} catch (...) {	}
			
			throw NoBindsException();
		
		}
		
		//	Create worker
		thread=Thread(
			thread_func,
			this
		);
	
	}
	
	
	ListenHandler::~ListenHandler () noexcept {
	
		//	Command worker to stop
		stop=true;
		
		//	Wait for worker to stop
		thread->Join();
		
		//	Wait for all pending thread pool
		//	tasks to flush out (to avoid
		//	dangling pointers in binds
		for (SmartPointer<ThreadPoolHandle> & p : pending) p->Wait();
		
		//	Rest of cleanup is automatic
	
	}
	
	
	void ListenHandler::thread_func (void * ptr) {
	
		ListenHandler * lh=reinterpret_cast<ListenHandler *>(ptr);
	
		try {
		
			try {
			
				lh->thread_func_impl();
			
			} catch (const std::exception & e) {
			
				try {
			
					lh->log(
						String::Format(
							log_template,
							lh->desc,
							String::Format(
								unexpected_exception_what_template,
								unexpected_exception,
								e.what()
							)
						),
						Service::LogType::Error
					);
					
				} catch (...) {	}
				
				throw;
			
			} catch (...) {
			
				try {
				
					lh->log(
						String::Format(
							log_template,
							lh->desc,
							unexpected_exception
						),
						Service::LogType::Error
					);
				
				} catch (...) {	}
				
				throw;
			
			}
		
		} catch (...) {
		
			try {
		
				lh->panic();
				
			} catch (...) {	}
			
			throw;
		
		}
	
	}
	
	
	void ListenHandler::thread_func_impl () {
	
		while (!stop) {
		
			//	Check sockets
			SocketSet set(sockets);
			if (Socket::Select(&set,nullptr,nullptr,select_timeout)!=0) {
			
				//	Grab pending connections
				for (Socket & s : sockets) if (set.Contains(s)) {
				
					IPAddress ip;
					UInt16 port;
					SmartPointer<Socket> conn=SmartPointer<Socket>::Make(
						s.Accept(&ip,&port)
					);
					
					//	Filter
					pending.EmplaceBack(
						pool->Enqueue(
							[this,conn,ip,port] () mutable -> Nullable<Tuple<SmartPointer<Socket>,IPAddress,UInt16>> {
								
								if (on_connect(ip,port)) return Tuple<SmartPointer<Socket>,IPAddress,UInt16>(
									std::move(conn),
									ip,
									port
								);
								
								return Nullable<Tuple<SmartPointer<Socket>,IPAddress,UInt16>>();
								
							}
						)
					);
				
				}
			
			}
			
			//	Check pending tasks
			for (Word i=0;i<pending.Count();) {
			
				if (pending[i]->Completed()) {
				
					//	Check for null
					if (
						pending[i]->Success() &&
						!pending[i]->Result<
							Nullable<Tuple<SmartPointer<Socket>,IPAddress,UInt16>>
						>()->IsNull()
					) {
					
						//	Keep the smart pointer alive
						SmartPointer<ThreadPoolHandle> tph(std::move(pending[i]));
						
						//	Check result/create connection
						ConnectType connect=this->connect;
						pool->Enqueue(
							[connect,tph] () mutable -> void {
							
								Tuple<
									SmartPointer<Socket>,
									IPAddress,
									UInt16
								> & t=**tph->Result<
									Nullable<Tuple<SmartPointer<Socket>,IPAddress,UInt16>>
								>();
							
								connect(
									std::move(*t.Item<0>()),
									t.Item<1>(),
									t.Item<2>()
								);
							
							}
						);
					
					}
					
					//	Delete
					pending.Delete(i);
					
				} else {
				
					++i;
				
				}
				
			}
		
		}
	
	}


}

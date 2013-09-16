#include <mysql_data_provider/mysql_data_provider.hpp>
#include <stdexcept>


namespace MCPP {


	static const char * mysql_library_init_failed="mysql_library_init failed";
	static const char * mysql_thread_init_failed="mysql_thread_init failed";
	
	
	void MySQLDataProvider::tasks_func () noexcept {
	
		for (;;) {
		
			tasks_lock.Acquire();
			
			//	Wait for something to do
			//	or the command to stop
			while (
				(tasks.Count()==0) &&
				!tasks_stop
			) tasks_wait.Sleep(tasks_lock);
			
			//	If commanded to stop,
			//	do so
			if (tasks_stop) {
			
				tasks_lock.Release();
				
				break;
			
			}
			
			//	Dequeue a task
			auto task=std::move(tasks[0]);
			tasks.Delete(0);
			
			//	Wake up destructor if it's
			//	waiting for us to finish
			//	all pending tasks
			if (tasks.Count()==0) tasks_wait.WakeAll();
			
			tasks_lock.Release();
			
			try {
			
				execute(std::move(task));
			
			//	There's nothing we can do to inform
			//	anyone of errors that occur here,
			//	so just eat them
			} catch (...) {	}
		
		}
	
	}
	
	
	static inline void thread_init () {
	
		if (mysql_thread_init()!=0) throw std::runtime_error(mysql_thread_init_failed);
	
	}
	
	
	static inline void thread_end () noexcept {
	
		mysql_thread_end();
	
	}
	
	
	std::unique_ptr<MySQLConnection> MySQLDataProvider::get () {
	
		thread_init();
		
		try {
	
			lock.Acquire();
			
			//	Wait until we can either create
			//	a new connection or dequeue an
			//	existing one
			bool waited=false;
			Timer timer;
			
			try {
			
				timer=Timer::CreateAndStart();
			
			} catch (...) {
			
				lock.Release();
				
				throw;
			
			}
			
			while (
				(pool.Count()==0) &&
				(pool_max!=0) &&
				(pool_curr==pool_max)
			) {
			
				waited=true;
			
				wait.Sleep(lock);
				
			}
			
			if (waited) {
			
				try {
				
					waiting+=timer.ElapsedNanoseconds();
				
				} catch (...) {
				
					lock.Release();
					
					throw;
				
				}
				
				++waited;
			
			}
			
			//	We can acquire a connection
			//	from the pool
			if (pool.Count()!=0) {
			
				auto where=pool.Count()-1;
				auto retr=std::move(pool[where]);
				pool.Delete(where);
				
				lock.Release();
				
				Timer timer=Timer::CreateAndStart();
				
				if (retr->KeepAlive(
					host,
					port,
					username,
					password,
					database
				)) {
				
					connecting+=timer.ElapsedNanoseconds();
					++connected;
				
				}
				
				return retr;
			
			}
			
			//	We must create a new connection
			
			++pool_curr;
			
			lock.Release();
			
			try {
			
				Timer timer=Timer::CreateAndStart();
			
				auto retr=std::unique_ptr<MySQLConnection>(
					new MySQLConnection(
						host,
						port,
						username,
						password,
						database
					)
				);
				
				connecting+=timer.ElapsedNanoseconds();
				++connected;
				
				return retr;
				
			} catch (...) {
			
				lock.Acquire();
				--pool_curr;
				wait.Wake();
				lock.Release();
				
				throw;
			
			}
			
		} catch (...) {
		
			thread_end();
			
			throw;
		
		}
	
	}
	
	
	void MySQLDataProvider::done (std::unique_ptr<MySQLConnection> & conn) {
	
		thread_end();
	
		lock.Acquire();
		
		try {
		
			pool.Add(
				std::move(
					conn
				)
			);
		
		} catch (...) {
		
			--pool_curr;
			
			wait.Wake();
			
			lock.Release();
			
			throw;
		
		}
		
		wait.Wake();
		
		lock.Release();
	
	}
	
	
	void MySQLDataProvider::done () noexcept {
	
		thread_end();
	
		lock.Acquire();
		
		--pool_curr;
		
		wait.Wake();
		
		lock.Release();
	
	}
	
	
	void MySQLDataProvider::enqueue (std::function<void (std::unique_ptr<MySQLConnection> &)> task) {
	
		tasks_lock.Execute([&] () {
		
			tasks.Add(
				std::move(
					task
				)
			);
			
			tasks_wait.Wake();
		
		});
	
	}


	MySQLDataProvider::MySQLDataProvider (
		Nullable<String> host,
		Nullable<UInt16> port,
		Nullable<String> username,
		Nullable<String> password,
		Nullable<String> database,
		Word pool_max
	)	:	tasks_stop(false),
			pool(pool_max),
			pool_curr(0),
			pool_max(pool_max),
			username(std::move(username)),
			password(std::move(password)),
			host(std::move(host)),
			database(std::move(database)),
			port(port)
	{
	
		//	Initialize libmysqlclient
		if (mysql_library_init(
			0,
			nullptr,
			nullptr
		)!=0) throw std::runtime_error(mysql_library_init_failed);
		
		//	Initialize statistics
		executed=0;
		executing=0;
		connected=0;
		connecting=0;
		waited=0;
		waiting=0;
		
		try {
		
			tasks_thread=Thread([this] () mutable {	tasks_func();	});
		
		} catch (...) {
		
			mysql_library_end();
			
			throw;
		
		}
	
	}
	
	
	MySQLDataProvider::~MySQLDataProvider () noexcept {
	
		//	Stop/wait for worker
		tasks_lock.Acquire();
		while (tasks.Count()!=0) tasks_wait.Sleep(tasks_lock);
		tasks_stop=true;
		tasks_wait.WakeAll();
		tasks_lock.Release();
		tasks_thread.Join();
	
		//	Destroy/close all connections
		for (Word i=pool.Count();(i--)>0;) pool.Delete(i);
		
		//	Clean up libmysqlclient
		mysql_library_end();
	
	}


}

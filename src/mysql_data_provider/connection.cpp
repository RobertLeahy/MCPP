#include <mysql_data_provider/mysql_data_provider.hpp>
#include <stdexcept>
#include <string>


namespace MCPP {


	namespace MySQL {
	
	
		MYSQL * Connection::get () noexcept {
		
			return &(*handle);
		
		}
		
		
		void Connection::destroy () noexcept {
		
			if (handle.IsNull()) return;
			
			//	Make sure all statements get cleaned up
			//	before the connection
			//
			//	I'm not 100% sure that this matters,
			//	but it doesn't hurt to be safe...
			statements.clear();
			
			mysql_close(get());
			
			handle.Destroy();
		
		}
		
		
		template <typename T>
		void Connection::set (enum mysql_option option, const T & arg) {
		
			if (mysql_options(
				get(),
				MYSQL_OPT_RECONNECT,
				&arg
			)!=0) Raise();
		
		}
		
		
		static const char * charsets []={
			"utf8mb4",
			"utf8"
		};
		
		
		static const char * insufficient_memory="Insufficient memory to allocate MYSQL object";
		
		
		void Connection::connect () {
		
			handle.Construct();
			
			try {
			
				if (mysql_init(get())==nullptr) throw std::runtime_error(insufficient_memory);
				
				try {
		
					//	No automatic reconnects -- we'll check
					//	by pinging
					my_bool reconnect=0;
					set(MYSQL_OPT_RECONNECT,reconnect);
				
					//	Encode strings
					Vector<char> h;
					if (!host.IsNull()) h=host->ToCString();
					Vector<char> u;
					if (!username.IsNull()) u=username->ToCString();
					Vector<char> p;
					if (!password.IsNull()) p=password->ToCString();
					Vector<char> db;
					if (!database.IsNull()) db=database->ToCString();
					
					//	Make the call
					if (mysql_real_connect(
						get(),
						host.IsNull() ? nullptr : h.begin(),
						username.IsNull() ? nullptr : u.begin(),
						password.IsNull() ? nullptr : p.begin(),
						database.IsNull() ? nullptr : db.begin(),
						port.IsNull() ? 0 : safe_cast<unsigned int>(*port),
						nullptr,
						0
					)==nullptr) Raise();
					
					//	Attempt to set all character sets we have
					int result;
					for (auto charset : charsets) if ((result=mysql_set_character_set(get(),charset))==0) break;
					if (result!=0) Raise();
					
				} catch (...) {
				
					mysql_close(get());
				
					throw;
				
				}
				
			} catch (...) {
			
				handle.Destroy();
				
				throw;
			
			}
		
		}
		
		
		bool Connection::check () {
		
			if (mysql_ping(get())==0) return true;
			
			auto guard=AtExit([&] () {	destroy();	});
			
			auto code=mysql_errno(get());
			if (
				(code==CR_SERVER_GONE_ERROR) ||
				//	The documentation for mysql_ping
				//	explicitly says that it doesn't cause
				//	CR_SERVER_LOST:
				//
				//	http://dev.mysql.com/doc/refman/5.7/en/mysql-ping.html
				//
				//	However, if you manually close all
				//	this data provider's connections on
				//	the server, and then watch as this
				//	function executes in the debugger,
				//	you'll discover that mysql_ping
				//	absolutely does cause CR_SERVER_LOST.
				(code==CR_SERVER_LOST)
			) return false;
			
			Raise();
		
		}
		
		
		static const char * init_failed="libmysqlclient or libmysqld failed to initialize";
		
		
		//	RAII wrapper around mysql_library_init and
		//	mysql_library_end
		class Library {
		
		
			private:
			
			
				bool success;
				
				
			public:
			
			
				Library () noexcept : success(mysql_library_init(0,nullptr,nullptr)==0) {	}
				
				
				~Library () noexcept {
				
					if (success) mysql_library_end();
				
				}
				
				
				//	Checks to make sure the library
				//	was initialized properly.
				//
				//	Throws if it wasn't
				void Check () const {
				
					if (!success) throw std::runtime_error(init_failed);
				
				}
		
		
		};
		
		
		static const Library mysql;
		
		
		static const char * not_thread_safe="MySQL library not thread safe";
		static const char * thread_init_failed="MySQL per thread initialization failed";
		
		
		//	RAII wrapper around mysql_thread_init and
		//	mysql_thread_end
		class Thread {
		
		
			public:
			
			
				Thread () {
				
					//	This whole implementation depends on
					//	multiple threads hitting MySQL at the
					//	same time, so if it's not thread safe
					//	just bail out at once.
					if (!mysql_thread_safe()) throw std::runtime_error(not_thread_safe);
				
					if (mysql_thread_init()!=0) throw std::runtime_error(thread_init_failed);
				
				}
				
				
				~Thread () noexcept {
				
					mysql_thread_end();
				
				}
		
		
		};
		
		
		static void ready_thread () {
		
			static thread_local const Thread thread;
		
		}
	
	
		Connection::Connection (
			Nullable<String> host,
			Nullable<String> username,
			Nullable<String> password,
			Nullable<String> database,
			Nullable<UInt16> port
		)	:	host(std::move(host)),
				username(std::move(username)),
				password(std::move(password)),
				database(std::move(database)),
				port(std::move(port))
		{
		
			//	Make sure the library is good and
			//	ready to go
			mysql.Check();
		
		}
		
		
		Connection::~Connection () noexcept {
		
			destroy();
		
		}
	
	
		bool Connection::Ready () {
		
			//	Make sure thread initialization is done
			ready_thread();
		
			//	Check to see if we have to reconnect, and
			//	do so if necessary
			if (
				handle.IsNull() ||
				!check()
			) {
			
				connect();
				
				return true;
				
			}
			
			return false;
		
		}
		
		
		static const char * generic_error="MySQL error";
	
	
		void Connection::Raise () {
		
			std::string str;
			try {
			
				str=mysql_error(get());
			
			} catch (...) {
			
				throw std::runtime_error(generic_error);
			
			}
			
			throw std::runtime_error(std::move(str));
		
		}
		
		
		Connection::operator MYSQL * () noexcept {
		
			return get();
		
		}
		
		
		PreparedStatement & Connection::Get (const String & text) {
		
			auto iter=statements.find(text);
			if (iter!=statements.end()) return iter->second;
			
			return statements.emplace(
				text,
				PreparedStatement(*this,text)
			).first->second;
		
		}
	
	
	}


}

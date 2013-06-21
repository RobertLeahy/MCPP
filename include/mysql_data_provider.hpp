/**
 *	\file
 */
 
 
#pragma once


#include <data_provider.hpp>
#include <thread_pool.hpp>
#include <mysql.h>


namespace MCPP {


	class MySQLDataProvider : public DataProvider {
	
	
		private:
			
			
			//	Connection
			MYSQL conn;
			//	Connection info for
			//	reconnections
			//
			//	Stored API encoded
			//	for efficiency's sake
			Vector<Byte> username;
			Vector<Byte> password;
			Vector<Byte> database;
			Vector<Byte> host;
			UInt16 port;
			
			
			//	Statements
			MYSQL_STMT * log_stmt;
			MYSQL_STMT * setting_stmt;
			MYSQL_STMT * kvp_stmt;
			MYSQL_STMT * kvp_delete_pair_stmt;
			MYSQL_STMT * kvp_insert_stmt;
			
			
			//	Thread pool for worker
			//	thread, which will be
			//	"polluted" by the MySQL
			//	thread local variables
			//	et cetera and which
			//	will be the only thread
			//	that touches the connection,
			//	thus obviating problems even
			//	if libmysql is not thread safe.
			//
			//	It is important that this be
			//	declared last so that username,
			//	password, database, host, and
			//	port will be available/populated
			//	when it starts its worker.
			ThreadPool pool;
			
			
			//	Private methods
			inline void connect ();
			inline void keep_alive ();
			inline void prepare_stmt (MYSQL_STMT * &, const char *);
			inline void destroy_stmts () noexcept;
			inline void destroy () noexcept;
			
			
		public:
		
		
			MySQLDataProvider (
				const String & host,
				UInt16 port,
				const String & username,
				const String & password,
				const String & database
			);
			
			
			virtual void WriteLog (const String & log, Service::LogType type) override;
			virtual Nullable<String> GetSetting (const String & setting) override;
			virtual Vector<Nullable<String>> GetValues (const String & key) override;
			virtual void DeleteValues (const String & key, const String & value) override;
			virtual void InsertValue (const String & key, const String & value) override;
	
	
	};


}

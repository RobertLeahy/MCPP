/**
 *	\file
 */
 
 
#pragma once


#include <data_provider.hpp>
#include <thread_pool.hpp>
#ifdef ENVIRONMENT_WINDOWS
#include <mysql.h>
#else
#include <mysql/mysql.h>
#endif


namespace MCPP {


	class MySQLDataProvider : public DataProvider {
	
	
		private:
		
		
			typedef Tuple<String,Vector<Tuple<String,String>>> InfoType;
			
			
			//	Connection
			mutable MYSQL conn;
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
			MYSQL_STMT * chat_log_stmt;
			MYSQL_STMT * column_stmt;
			MYSQL_STMT * column_insert_stmt;
			MYSQL_STMT * column_update_stmt;
			
			
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
			mutable ThreadPool pool;
			
			
			//	Statistics
			//
			//	Number of requests handled
			UInt64 requests;
			//	Number of times the provider has
			//	had to reconnect to the SQL
			//	server
			UInt64 reconnects;
			
			
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
			void WriteChatLog (const String & from, const Vector<String> & to, const String & message, const Nullable<String> & notes) override;
			virtual InfoType GetInfo () const override;
	
	
	};


}

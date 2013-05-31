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
		
		
			//	Thread pool
			Nullable<ThreadPool> pool;
			
			
			//	Connection
			MYSQL conn;
			
			
			//	Statements
			MYSQL_STMT * setting_stmt;
			MYSQL_STMT * setting_update_stmt;
			MYSQL_STMT * setting_insert_stmt;
			MYSQL_STMT * setting_delete_stmt;
			MYSQL_STMT * log_stmt;
			MYSQL_STMT * key_stmt;
			MYSQL_STMT * pair_delete_stmt;
			MYSQL_STMT * key_delete_stmt;
			MYSQL_STMT * chunk_stmt;
			MYSQL_STMT * chunk_update_stmt;
			MYSQL_STMT * chunk_insert_stmt;
			
			
			//	Statement locks
			Mutex setting_lock;
			//	One lock for UPDATE and
			//	INSERT since they follow
			//	one another
			Mutex setting_set_lock;
			Mutex setting_delete_lock;
			Mutex log_lock;
			Mutex key_lock;
			Mutex pair_delete_lock;
			Mutex key_delete_lock;
			Mutex chunk_lock;
			//	One lock for UPDATE and
			//	INSERT since they follow
			//	one another
			Mutex chunk_set_lock;
			
			
			inline MYSQL_STMT * prepare_stmt (const String &);
			inline void prepare_statements ();
			inline void destroy () noexcept;
			
			
		public:
		
		
			MySQLDataProvider () = delete;
			
			
			MySQLDataProvider (
				const String & host,
				UInt16 port,
				const String & username,
				const String & password,
				const String & database
			);
			
			
			virtual ~MySQLDataProvider () noexcept override;
			
			
			virtual void WriteLog (const String &, Service::LogType) override;
			
			
			virtual Nullable<String> GetSetting (const String & setting) override;
			virtual void SetSetting (const String & setting, const Nullable<String> & value) override;
			virtual void DeleteSetting (const String & setting) override;
			
			
			virtual Vector<Nullable<String>> GetValues (const String & key) override;
			virtual void DeletePairs (const String & key, const String & value) override;
			virtual void DeleteKey (const String & key) override;
			
			
			virtual void LoadChunk (Int32 x, Int32 y, Int32 z, SByte dimension, const ChunkLoad & callback) override;
			virtual void SaveChunk (
				Int32 x,
				Int32 y,
				Int32 z,
				SByte dimension,
				const Byte * begin,
				const Byte * end,
				const ChunkSaveBegin & callback_begin,
				const ChunkSaveEnd & callback_end
			) override;
	
	
	};


}

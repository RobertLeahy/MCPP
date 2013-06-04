#include <mysql_data_provider.hpp>
#include <cstring>
#include <utility>
#include <type_traits>
#include <limits>


namespace MCPP {


	DataProvider * DataProvider::GetDataProvider () {
	
		//	Will eventually pull from a config file
		//	this is just for testing
		#include "login_info.cpp"
	
		return new MySQLDataProvider(
			host,
			port,
			username,
			password,
			database
		);
	
	}


	//	Messages
	static const char * mysql_init_failed="MySQL initialization failed";
	static const char * mysql_failed="MySQL query failed";
	
	
	//	Queries
	static const String setting_query("SELECT `value` FROM `settings` WHERE `setting`=?");
	static const String setting_insert_query("INSERT INTO `settings` (`value`,`setting`) VALUES(?,?)");
	static const String setting_update_query("UPDATE `settings` SET `value`=? WHERE `setting`=?");
	static const String setting_delete_query("DELETE FROM `settings` WHERE `setting`=?");
	static const String log_query("INSERT INTO `log` (`type`,`text`) VALUES(?,?)");
	static const String key_query("SELECT `value` FROM `keyvaluepairs` WHERE `key`=?");
	static const String pair_delete_query("DELETE FROM `keyvaluepairs` WHERE `key`=? AND `value`=?");
	static const String key_delete_query("DELETE FROM `keyvaluepairs` WHERE `key`=?");
	static const String chunk_query("SELECT `data` FROM `chunks` WHERE `x`=? AND `y`=? AND `z`=? AND `dimension`=?");
	static const String chunk_insert_query("INSERT INTO `chunks` (`data`,`x`,`y`,`z`,`dimension`) VALUES (?,?,?,?,?)");
	static const String chunk_update_query("UPDATE `chunks` SET `data`=? WHERE `x`=? AND `y`=? AND `z`=? AND `dimension`=?");
	
	
	//	Constants
	static const Word num_threads=5;


	static inline Vector<Byte> api_encode (const String & string) {
	
		//	API accepts UTF-8 encoded, null-terminated strings
		Vector<Byte> encoded=UTF8().Encode(string);
		
		//	Add null terminator
		encoded.Add(0);
		
		return encoded;
	
	}
	
	
	static inline char * to_char (Vector<Byte> & buffer) noexcept {
	
		return reinterpret_cast<char *>(static_cast<Byte *>(buffer));
	
	}
	
	
	inline MYSQL_STMT * MySQLDataProvider::prepare_stmt (const String & query) {
	
		MYSQL_STMT * stmt=mysql_stmt_init(&conn);
		
		if (stmt==nullptr) throw std::runtime_error(mysql_error(&conn));
		
		try {
		
			Vector<Byte> query_encoded=api_encode(query);
			
			if (mysql_stmt_prepare(
				stmt,
				to_char(query_encoded),
				query_encoded.Length()
			)!=0) throw std::runtime_error(mysql_stmt_error(stmt));
		
		} catch (...) {
		
			mysql_stmt_close(stmt);
			
			throw;
		
		}
		
		return stmt;
	
	}
	
	
	static inline void destroy_stmt (MYSQL_STMT * stmt) noexcept {
	
		if (stmt!=nullptr) mysql_stmt_close(stmt);
	
	}
	
	
	template <typename T>
	inline void bind (MYSQL_BIND & bind, const T & param) noexcept {
	
		switch (sizeof(T)) {
		
			case 1:
			
				bind.buffer_type=MYSQL_TYPE_TINY;
				break;
				
			case 2:
			
				bind.buffer_type=MYSQL_TYPE_SHORT;
				break;
				
			case 4:
			default:
			
				bind.buffer_type=std::is_floating_point<T>::value ? MYSQL_TYPE_FLOAT : MYSQL_TYPE_LONG;
				break;
				
			case 8:
			
				bind.buffer_type=std::is_floating_point<T>::value ? MYSQL_TYPE_DOUBLE : MYSQL_TYPE_LONGLONG;
				break;
				
		}
		
		if (std::is_unsigned<T>::value) bind.is_unsigned=static_cast<my_bool>(true);
		
		bind.buffer=const_cast<T *>(&param);
		bind.buffer_length=sizeof(T);
	
	}
	
	
	inline void MySQLDataProvider::prepare_statements () {
	
		//	Initialize all statements to NULL
		setting_stmt=nullptr;
		setting_update_stmt=nullptr;
		setting_insert_stmt=nullptr;
		setting_delete_stmt=nullptr;
		log_stmt=nullptr;
		key_stmt=nullptr;
		pair_delete_stmt=nullptr;
		key_delete_stmt=nullptr;
		chunk_stmt=nullptr;
		chunk_update_stmt=nullptr;
		chunk_insert_stmt=nullptr;
		
		//	Attempt to initialize statements
		try {
		
			setting_stmt=prepare_stmt(setting_query);
			setting_update_stmt=prepare_stmt(setting_update_query);
			setting_insert_stmt=prepare_stmt(setting_insert_query);
			setting_delete_stmt=prepare_stmt(setting_delete_query);
			log_stmt=prepare_stmt(log_query);
			key_stmt=prepare_stmt(key_query);
			pair_delete_stmt=prepare_stmt(pair_delete_query);
			key_delete_stmt=prepare_stmt(key_delete_query);
			chunk_stmt=prepare_stmt(chunk_query);
			chunk_update_stmt=prepare_stmt(chunk_update_query);
			chunk_insert_stmt=prepare_stmt(chunk_insert_query);
		
		} catch (...) {
		
			destroy_stmt(setting_stmt);
			destroy_stmt(setting_update_stmt);
			destroy_stmt(setting_insert_stmt);
			destroy_stmt(setting_delete_stmt);
			destroy_stmt(log_stmt);
			destroy_stmt(key_stmt);
			destroy_stmt(pair_delete_stmt);
			destroy_stmt(key_delete_stmt);
			destroy_stmt(chunk_stmt);
			destroy_stmt(chunk_update_stmt);
			destroy_stmt(chunk_insert_stmt);
			
			throw;
		
		}
	
	}
	
	
	inline void MySQLDataProvider::destroy () noexcept {
	
		//	Dispose of resources
		
		//	Statements first
		mysql_stmt_close(setting_stmt);
		mysql_stmt_close(setting_update_stmt);
		mysql_stmt_close(setting_insert_stmt);
		mysql_stmt_close(setting_delete_stmt);
		mysql_stmt_close(log_stmt);
		mysql_stmt_close(key_stmt);
		mysql_stmt_close(pair_delete_stmt);
		mysql_stmt_close(key_delete_stmt);
		mysql_stmt_close(chunk_stmt);
		mysql_stmt_close(chunk_update_stmt);
		mysql_stmt_close(chunk_insert_stmt);
		
		//	Then the connection
		mysql_close(&conn);
		
		//	Then the whole MySQL library
		mysql_library_end();
	
	}


	MySQLDataProvider::MySQLDataProvider (
		const String & host,
		UInt16 port,
		const String & username,
		const String & password,
		const String & database
	) {
	
		bool thread_safe=mysql_thread_safe();
		
		//	Startup routine
		auto startup=[&] () {
		
			//	Attempt to initialize MySQL
			if (mysql_library_init(0,nullptr,nullptr)!=0) throw std::runtime_error(mysql_init_failed);
			
			//	Make sure resources get released
			try {
			
				//	Prepare handle
				if (mysql_init(&conn)==nullptr) throw std::runtime_error(mysql_init_failed);
				
				//	Make sure the handle gets
				//	cleaned up if failure occurs
				try {
				
					//	Set automatic reconnect
					my_bool reconnect=static_cast<my_bool>(true);
					mysql_options(&conn,MYSQL_OPT_RECONNECT,&reconnect);
					
					//	Set charset to UTF-8 for
					//	Unicode support
					mysql_options(&conn,MYSQL_SET_CHARSET_NAME,"utf8mb4");
					
					//	Connect
					
					//	Convert strings to null-terminated
					//	UTF-8 (i.e. "API encode" them)
					auto host_encoded=api_encode(host);
					auto username_encoded=api_encode(username);
					auto password_encoded=api_encode(password);
					auto database_encoded=api_encode(database);
					
					//	Attempt to connect
					if (mysql_real_connect(
						&conn,
						to_char(host_encoded),
						to_char(username_encoded),
						to_char(password_encoded),
						to_char(database_encoded),
						static_cast<unsigned int>(port),
						nullptr,
						CLIENT_MULTI_STATEMENTS|CLIENT_FOUND_ROWS
					)==nullptr) throw std::runtime_error(mysql_error(&conn));
					
					//	Prepare statements
					prepare_statements();
				
				} catch (...) {
				
					mysql_close(&conn);
				
					throw;
				
				}
			
			} catch (...) {
			
				mysql_library_end();
				
				throw;
			
			}
			
			//	If this is thread safe, this is
			//	executing in the main thread,
			//	which won't handle MySQL and therefore
			//	the MySQL-specific resources
			//	created for this thread need to be
			//	cleaned up.
			//
			//	Otherwise it's executing in a thread
			//	pool thread, and if we got here
			//	that thread succeeded, so it's
			//	going to need those resources
			if (thread_safe) mysql_thread_end();
		
		};
		
		//	If the library is thread safe, put
		//	the provider into multi-threaded mode.
		if (thread_safe) {
		
			//	Fire initializer
			startup();
			
			//	Create thread pool
			pool.Construct(
				//	Number of workers
				num_threads,
				//	Make sure MySQL variables get
				//	initialized in the thread
				[] () {	if (mysql_thread_init()!=0) throw std::runtime_error(mysql_init_failed);	},
				//	Make sure MySQL variables get
				//	cleaned up once the worker
				//	is done
				[] () {	mysql_thread_end();	}
			);
		
		//	Otherwise put the provider into
		//	single threaded mode
		} else {
		
			//	Since the library is single
			//	threaded in this instance,
			//	we cannot run the startup
			//	routine in the main thread,
			//	we must run it in the one
			//	thread pool worker thread
			pool.Construct(
				//	Single threaded
				1,
				//	Initialize in the worker thread
				startup,
				//	When we clean up, we clean up
				//	the whole library, not just
				//	the per thread resources
				[this] () {	destroy();	}
			);
		
		}
	
	}
	
	
	MySQLDataProvider::~MySQLDataProvider () noexcept {
	
		//	If the library is not thread safe,
		//	we can actually pass, the thread pool
		//	will clean everything up for us.
		if (mysql_thread_safe()!=0) {
		
			//	Shut the thread pool down
			pool.Destroy();
			
			//	Cleanup all resources
			destroy();
		
		}
	
	}
	
	
	void MySQLDataProvider::WriteLog (const String & log, Service::LogType type) {
	
		//	Instruct the thread pool to
		//	write to the log
		pool->Enqueue(
			[=] () {
			
				//	Database is UTF8
				UTF8 encoder;
				
				//	Prepare bound parameters
				MYSQL_BIND param [2];
				memset(param,0,sizeof(MYSQL_BIND)*2);
				
				//	Parameter #1 - Type String
				param[0].buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> type_encoded=encoder.Encode(
					//	Map log type to a meaningful string
					DataProvider::GetLogType(type)
				);
				param[0].buffer=to_char(type_encoded);
				param[0].buffer_length=type_encoded.Length();
				
				//	Parameter #2 - Log Text
				param[1].buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> log_encoded=encoder.Encode(log);
				param[1].buffer=to_char(log_encoded);
				param[1].buffer_length=log_encoded.Length();
				
				//	CRITICAL SECTION BEGINS
				log_lock.Execute([&] () {
				
					//	Perform INSERT
					if (
						(mysql_stmt_bind_param(log_stmt,param)!=0) ||
						(mysql_stmt_execute(log_stmt)!=0)
					) throw std::runtime_error(mysql_stmt_error(log_stmt));
					
				});
			
			}
		);
	
	}
	
	
	Nullable<String> MySQLDataProvider::GetSetting (const String & setting) {
	
		//	Instruct the thread pool to retrieve
		//	the value of the setting
		auto handle=pool->Enqueue(
			[&] () -> Nullable<String> {
			
				//	Database is UTF8
				UTF8 encoder;
				
				//	Setup parameter
				MYSQL_BIND param;
				memset(&param,0,sizeof(MYSQL_BIND));
				param.buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> setting_encoded=encoder.Encode(setting);
				param.buffer=to_char(setting_encoded);
				param.buffer_length=setting_encoded.Length();
				
				//	Setup result
				MYSQL_BIND result;
				memset(&result,0,sizeof(MYSQL_BIND));
				result.buffer_type=MYSQL_TYPE_STRING;
				unsigned long real_length=0;
				result.length=&real_length;
				my_bool is_null;
				result.is_null=&is_null;
				
				//	CRITICAL SECTION BEGINS
				return setting_lock.Execute([&] () -> Nullable<String> {
				
					//	Bind and execute
					if (
						(mysql_stmt_bind_param(setting_stmt,&param)!=0) ||
						(mysql_stmt_bind_result(setting_stmt,&result)!=0) ||
						(mysql_stmt_execute(setting_stmt)!=0)
					) throw std::runtime_error(mysql_stmt_error(setting_stmt));
					
					//	Fetch
					int success=mysql_stmt_fetch(setting_stmt);
					
					//	Check for error
					if (success==1) throw std::runtime_error(mysql_stmt_error(setting_stmt));
					
					//	Value to return, defaults to null
					Nullable<String> value;
					
					//	Check for data
					if (
						(success!=MYSQL_NO_DATA) &&
						!is_null
					) {
					
						//	Empty string
						if (real_length==0) {
						
							value=String();
						
						//	Non-empty string, create
						//	buffer and retrieve result
						} else {
						
							//	Allocate sufficiently-sized buffer
							//
							//	Make sure this is a safe operation
							Word buffer_len=Word(
								SafeInt<
									decltype(real_length)
								>(
									real_length
								)
							);
							Vector<Byte> buffer(buffer_len);
							
							//	Wire buffer into result
							result.buffer=static_cast<Byte *>(buffer);
							result.buffer_length=real_length;
							
							//	Retrieve data
							if (mysql_stmt_fetch_column(
								setting_stmt,
								&result,
								0,
								0
							)!=0) throw std::runtime_error(mysql_stmt_error(setting_stmt));
							
							//	Succeeded, data retrieved, set count
							buffer.SetCount(buffer_len);
							
							//	Decode
							value=encoder.Decode(
								buffer.begin(),
								buffer.end()
							);
						
						}
					
					}
					
					//	Finish up with result set
					while (success!=MYSQL_NO_DATA) {
					
						success=mysql_stmt_fetch(setting_stmt);
						
						if (success==1) throw std::runtime_error(mysql_stmt_error(setting_stmt));
					
					}
					
					//	Return
					return value;
					
				});
			
			}
		);
		
		//	Wait for the task to finish
		handle->Wait();
		
		//	Success?
		if (handle->Success()) return Nullable<String>(std::move(*handle->Result<Nullable<String>>()));
		
		//	Failure
		throw std::runtime_error(mysql_failed);
	
	}
	
	
	void MySQLDataProvider::SetSetting (const String & setting, const Nullable<String> & value) {
	
		//	Instruct thread pool to set
		//	the value of the setting
		auto handle=pool->Enqueue(
			[&] () {
			
				//	Database is UTF8
				UTF8 encoder;
				
				//	Prepare bound parameters
				MYSQL_BIND param [2];
				memset(param,0,sizeof(MYSQL_BIND)*2);
				
				//	Parameter #1 - Target Value
				param[0].buffer_type=MYSQL_TYPE_STRING;
				my_bool is_null;
				//	Zero prevents spurious memory
				//	allocation
				Vector<Byte> value_encoded(0);
				//	Branch based on whether value
				//	is null or not
				if (value.IsNull()) {
				
					is_null=static_cast<my_bool>(true);
					param[0].is_null=&is_null;
				
				} else {
				
					value_encoded=encoder.Encode(*value);
					param[0].buffer=to_char(value_encoded);
					param[0].buffer_length=value_encoded.Length();
				
				}
				
				//	Parameter #2 - Setting Name
				param[1].buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> setting_encoded=encoder.Encode(setting);
				param[1].buffer=to_char(setting_encoded);
				param[1].buffer_length=setting_encoded.Length();
				
				//	CRITICAL SECTION BEGINS
				setting_set_lock.Execute([&] () {
				
					//	Bind parameters and perform UPDATE
					if (
						(mysql_stmt_bind_param(setting_update_stmt,param)!=0) ||
						(mysql_stmt_execute(setting_update_stmt)!=0)
					) throw std::runtime_error(mysql_stmt_error(setting_update_stmt));
					
					//	Check to see if we hit the target
					//	setting
					if (mysql_stmt_affected_rows(setting_update_stmt)==0) {
					
						//	We didn't, insert
						
						if (
							(mysql_stmt_bind_param(setting_insert_stmt,param)!=0) ||
							(mysql_stmt_execute(setting_insert_stmt)!=0)
						) throw std::runtime_error(mysql_stmt_error(setting_insert_stmt));
					
					}
				
				});
			
			}
		);
		
		//	Wait for completion
		handle->Wait();
		
		//	Report failure by throwing
		//	if applicable
		if (!handle->Success()) throw std::runtime_error(mysql_failed);
	
	}
	
	
	void MySQLDataProvider::DeleteSetting (const String & setting) {
	
		//	Instruct thread pool to delete
		//	the setting.
		auto handle=pool->Enqueue(
			[&] () {
			
				//	Database is UTF8
				UTF8 encoder;
				
				//	Prepare bound parameter
				MYSQL_BIND param;
				memset(&param,0,sizeof(MYSQL_BIND));
				param.buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> setting_encoded=encoder.Encode(setting);
				param.buffer=to_char(setting_encoded);
				param.buffer_length=setting_encoded.Length();
				
				//	CRITICAL SECTION BEGINS
				setting_delete_lock.Execute([&] () {
				
					//	Bind and execute
					if (
						(mysql_stmt_bind_param(setting_delete_stmt,&param)!=0) ||
						(mysql_stmt_execute(setting_delete_stmt)!=0)
					) throw std::runtime_error(mysql_stmt_error(setting_delete_stmt));
				
				});
			
			}
		);
		
		//	Wait for completion
		handle->Wait();
		
		//	Report failure by throwing if
		//	applicable
		if (!handle->Success()) throw std::runtime_error(mysql_failed);
	
	}
	
	
	Vector<Nullable<String>> MySQLDataProvider::GetValues (const String & key) {
	
		//	Instruct the thread pool to
		//	retrieve the values
		auto handle=pool->Enqueue(
			[&] () {
			
				//	Database is UTF8
				UTF8 encoder;
				
				//	Parameter
				MYSQL_BIND param;
				memset(&param,0,sizeof(MYSQL_BIND));
				param.buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> key_encoded=encoder.Encode(key);
				param.buffer=to_char(key_encoded);
				param.buffer_length=key_encoded.Length();
				
				//	Result
				MYSQL_BIND result;
				memset(&result,0,sizeof(MYSQL_BIND));
				result.buffer_type=MYSQL_TYPE_STRING;
				unsigned long real_length=0;
				result.length=&real_length;
				my_bool is_null;
				result.is_null=&is_null;
				
				//	CRITICAL SECTION BEGINS
				return key_lock.Execute([&] () -> Vector<Nullable<String>> {
				
					//	Bind and execute
					if (
						(mysql_stmt_bind_param(key_stmt,&param)!=0) ||
						(mysql_stmt_bind_result(key_stmt,&result)!=0) ||
						(mysql_stmt_execute(key_stmt)!=0)
					) throw std::runtime_error(mysql_stmt_error(key_stmt));
					
					//	Container to return results
					//	in
					Vector<Nullable<String>> results;
					
					//	Fetch results
					int success;
					while (!(
						//	End of result set
						((success=mysql_stmt_fetch(key_stmt))==MYSQL_NO_DATA) ||
						//	Error
						(success==1)
					)) {
					
						//	Is this result null?
						if (is_null) {
						
							results.EmplaceBack();
						
						//	Is this result the empty string?
						} else if (real_length==0) {
						
							results.EmplaceBack(String());
						
						//	Retrieve the string
						} else {
						
							//	Allocate sufficiently-sized buffer
							//
							//	Make sure this is a safe operation
							Word buffer_len=Word(
								SafeInt<
									decltype(real_length)
								>(
									real_length
								)
							);
							Vector<Byte> buffer(buffer_len);
							
							//	Wire buffer into result
							result.buffer=static_cast<Byte *>(buffer);
							result.buffer_length=real_length;
							
							//	Retrieve data
							if (mysql_stmt_fetch_column(
								key_stmt,
								&result,
								0,
								0
							)!=0) throw std::runtime_error(mysql_stmt_error(key_stmt));
							
							//	Succeeded, data in buffer, set count
							buffer.SetCount(static_cast<Word>(real_length));
							
							//	Decode
							results.EmplaceBack(
								encoder.Decode(
									buffer.begin(),
									buffer.end()
								)
							);
							
							//	We may loop again, make sure that
							//	we're not reading into this buffer
							//	(which will be invalid at the end
							//	of this loop iteration).
							real_length=0;
							result.buffer=nullptr;
							result.buffer_length=0;
						
						}
					
					}
					
					//	Did we pop out of the loop
					//	due to an error?
					if (success==1) throw std::runtime_error(mysql_stmt_error(key_stmt));
					
					//	Success, return
					return results;
				
				});
			
			}
		);
		
		//	Wait for completion
		handle->Wait();
		
		//	Success?
		if (handle->Success()) return Vector<Nullable<String>>(
			std::move(
				*handle->Result<Vector<Nullable<String>>>()
			)
		);
		
		//	Failure
		throw std::runtime_error(mysql_failed);
	
	}
	
	
	void MySQLDataProvider::DeletePairs (const String & key, const String & value) {
	
		//	Instruct thread pool to delete
		auto handle=pool->Enqueue(
			[&] () {
			
				//	Database is UTF8
				UTF8 encoder;
				
				//	Two parameters
				MYSQL_BIND param [2];
				memset(param,0,sizeof(MYSQL_BIND)*2);
				
				//	Parameter #1 - Key
				param[0].buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> key_encoded=encoder.Encode(key);
				param[0].buffer=to_char(key_encoded);
				param[0].buffer_length=key_encoded.Length();
				
				//	Parameter #2 - Value
				param[1].buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> value_encoded=encoder.Encode(value);
				param[1].buffer=to_char(value_encoded);
				param[1].buffer_length=value_encoded.Length();
				
				//	CRITICAL SECTION BEGINS
				pair_delete_lock.Execute([&] () {
				
					//	Execute SQL and delete pair(s)
					if (
						(mysql_stmt_bind_param(pair_delete_stmt,param)!=0) ||
						(mysql_stmt_execute(pair_delete_stmt)!=0)
					) throw std::runtime_error(mysql_stmt_error(pair_delete_stmt));
				
				});
			
			}
		);
		
		//	Wait for completion
		handle->Wait();
		
		//	Throw if failure
		if (!handle->Success()) throw std::runtime_error(mysql_failed);
	
	}
	
	
	void MySQLDataProvider::DeleteKey (const String & key) {
	
		//	Dispatch
		auto handle=pool->Enqueue(
			[&] () {
			
				//	Just one parameter -- name of the key
				MYSQL_BIND param;
				memset(&param,0,sizeof(MYSQL_BIND));
				param.buffer_type=MYSQL_TYPE_STRING;
				Vector<Byte> key_encoded=UTF8().Encode(key);
				param.buffer=to_char(key_encoded);
				param.buffer_length=key_encoded.Length();
				
				//	CRITICAL SECTION BEGINS
				key_delete_lock.Execute([&] () {
				
					//	Execute SQL and delete pairs
					if (
						(mysql_stmt_bind_param(key_delete_stmt,&param)!=0) ||
						(mysql_stmt_execute(key_delete_stmt)!=0)
					) throw std::runtime_error(mysql_stmt_error(key_delete_stmt));
				
				});
			
			}
		);
		
		//	Wait
		handle->Wait();
		
		//	Throw if failure
		if (!handle->Success()) throw std::runtime_error(mysql_failed);
	
	}
	
	
	void MySQLDataProvider::SaveChunk (Int32 x, Int32 y, Int32 z, SByte dimension, const Byte * begin, const Byte * end, const ChunkSaveBegin & callback_begin, const ChunkSaveEnd & callback_end) {
	
		//	Dispatch task into the thread pool
		pool->Enqueue(
			[=] () {
			
				//	Bind five parameters
				MYSQL_BIND param [5];
				memset(param,0,sizeof(MYSQL_BIND)*5);
				
				param[0].buffer_type=MYSQL_TYPE_BLOB;
				param[0].buffer_length=static_cast<decltype(param[1].buffer_length)>(
					SafeInt<decltype(end-begin)>(end-begin)
				);
				param[0].buffer=const_cast<Byte *>(begin);
				
				bind(param[1],x);
				bind(param[2],y);
				bind(param[3],z);
				bind(param[4],dimension);
				
				bool completed;
				try {
				
					//	CRITICAL SECTION BEGINS
					completed=chunk_set_lock.Execute([&] () {
					
						//	BEGIN
						if (
							callback_begin &&
							!callback_begin(x,y,z,dimension)
						) return false;
					
						//	Bind and execute UPDATE
						if (
							(mysql_stmt_bind_param(chunk_update_stmt,param)!=0) ||
							(mysql_stmt_execute(chunk_update_stmt)!=0)
						) throw std::runtime_error(mysql_stmt_error(chunk_update_stmt));
						
						//	Check affected rows, if it's zero
						//	we need to insert
						if (
							(mysql_stmt_affected_rows(chunk_update_stmt)==0) &&
							(
								(mysql_stmt_bind_param(chunk_insert_stmt,param)!=0) ||
								(mysql_stmt_execute(chunk_insert_stmt)!=0)
							)
						) throw std::runtime_error(mysql_stmt_error(chunk_insert_stmt));
						
						return true;
					
					});
				
				} catch (...) {
				
					//	Saving failed, report
					try {
					
						if (callback_end) callback_end(x,y,z,dimension,false);
					
					} catch (...) {	}
					
					throw;
				
				}
				
				//	Success!
				if (
					completed &&
					callback_end
				) callback_end(x,y,z,dimension,true);
			
			}
		);
	
	}
	
	
	void MySQLDataProvider::LoadChunk (Int32 x, Int32 y, Int32 z, SByte dimension, const ChunkLoad & callback) {
	
		//	Dispatch task into the thread pool
		pool->Enqueue(
			[=] () {
			
				//	Bind four parameters
				MYSQL_BIND param [4];
				memset(param,0,sizeof(MYSQL_BIND)*4);
				
				bind(param[0],x);
				bind(param[1],y);
				bind(param[2],z);
				bind(param[3],dimension);
				
				//	Bind results
				MYSQL_BIND result;
				memset(&result,0,sizeof(MYSQL_BIND));
				
				//	Result - Chunk
				result.buffer_type=MYSQL_TYPE_BLOB;
				unsigned long real_length=0;
				result.length=&real_length;
				
				//	Completed chunk goes here
				Nullable<Vector<Byte>> chunk;
				
				try {
				
					//	CRITICAL SECTION BEGINS
					chunk_lock.Execute([&] () {
					
						//	Bind and execute
						if (
							(mysql_stmt_bind_param(chunk_stmt,param)!=0) ||
							(mysql_stmt_bind_result(chunk_stmt,&result)!=0) ||
							(mysql_stmt_execute(chunk_stmt)!=0)
						) throw std::runtime_error(mysql_stmt_error(chunk_stmt));
						
						//	Fetch
						int success=mysql_stmt_fetch(chunk_stmt);
						
						//	Fail on error
						if (success==1) throw std::runtime_error(mysql_stmt_error(chunk_stmt));
						
						//	If the chunk is empty we're as good
						//	as done
						if (real_length==0) return;
						
						//	Get size of buffer safely
						Word size=Word(SafeInt<decltype(real_length)>(real_length));
						
						//	Allocate buffer
						chunk.Construct(size);
						
						//	Prepare bind for real fetch
						result.buffer=to_char(*chunk);
						result.buffer_length=real_length;
						
						//	Retrieve data
						if (mysql_stmt_fetch_column(
							chunk_stmt,
							&result,
							1,
							0
						)!=0) throw std::runtime_error(mysql_stmt_error(setting_stmt));
						
						//	Set buffer count
						chunk->SetCount(size);
						
						//	DONE
					
					});
					
				} catch (...) {
				
					//	Make sure the buffer is nulled
					chunk.Destroy();
				
					//	Retrieval failed, report it
					try {
					
						if (callback) callback(x,y,z,dimension,false,std::move(chunk));
					
					} catch (...) {	}
					
					throw;
				
				}
				
				//	Success!
				if (callback) callback(x,y,z,dimension,true,std::move(chunk));
			
			}
		);
	
	}


}

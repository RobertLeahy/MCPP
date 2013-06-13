#include <mysql_data_provider.hpp>
#include <cstring>
#include <type_traits>
#include <errmsg.h>


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
	
	
	//	Messages for exceptions
	static const char * mysql_init_failed="MySQL initialization failed";
	static const char * mysql_failed="MySQL error";
	
	
	//	Queries
	static const char * log_query="INSERT INTO `log` (`type`,`text`) VALUES (?,?)";
	static const char * setting_query="SELECT `value` FROM `settings` WHERE `setting`=?";
	
	
	//	Encodes a Unicode string such that
	//	it is fit for passing through to
	//	the database -- i.e. it is
	//	UTF8 encoded
	static inline Vector<Byte> db_encode (const String & string) {
	
		return UTF8().Encode(string);
	
	}
	
	
	//	Encodes a Unicode string such that
	//	it is fit for passing through to the
	//	libmysql C API -- i.e. it is UTF8
	//	encoded and null-terminated
	static inline Vector<Byte> api_encode (const String & string) {
	
		Vector<Byte> encoded(
			UTF8().Encode(string)
		);
		
		encoded.Add(0);
		
		return encoded;
	
	}
	
	
	//	Retrieves a char pointer from a
	//	buffer of bytes (just a nice
	//	wrapper for some ugly casts,
	//	should have no runtime cost).
	static inline char * to_char (Vector<Byte> & buffer) noexcept {
	
		return reinterpret_cast<char *>(
			static_cast<Byte *>(
				buffer
			)
		);
	
	}
	
	
	//	Destroys a prepared statement
	//	AND NULLS IT
	static inline void destroy_stmt (MYSQL_STMT * & stmt) noexcept {
	
		if (stmt!=nullptr) {
		
			mysql_stmt_close(stmt);
			
			stmt=nullptr;
		
		}
	
	}
	
	
	//	Prepares a prepared statement
	//	if and only if it is null
	inline void MySQLDataProvider::prepare_stmt (MYSQL_STMT * & stmt, const char * query) {
	
		if (stmt==nullptr) {
		
			stmt=mysql_stmt_init(&conn);
			
			if (stmt==nullptr) throw std::runtime_error(mysql_error(&conn));
			
			try {
				
				if (mysql_stmt_prepare(
					stmt,
					query,
					strlen(query)
				)!=0) throw std::runtime_error(mysql_stmt_error(stmt));
			
			} catch (...) {
			
				//	Make sure statement
				//	doesn't leak
				destroy_stmt(stmt);
				
				throw;
			
			}
		
		}
	
	}
	
	
	//	Destroys all prepared statements
	//	and nulls them
	inline void MySQLDataProvider::destroy_stmts () noexcept {
	
		destroy_stmt(log_stmt);
		destroy_stmt(setting_stmt);
	
	}
	
	
	//	Connects to the MySQL server
	inline void MySQLDataProvider::connect () {
	
		//	Every statement is invalid
		//	and now must be destroyed and
		//	nulled.
		destroy_stmts();
	
		//	Initialize connection handle
		if (mysql_init(&conn)==nullptr) throw std::runtime_error(mysql_init_failed);
		
		//	Set charset to UTF-8 for
		//	Unicode support
		mysql_options(
			&conn,
			MYSQL_SET_CHARSET_NAME,
			"utf8mb4"
		);
		
		//	Connect
		if (mysql_real_connect(
			&conn,
			to_char(host),
			to_char(username),
			to_char(password),
			to_char(database),
			static_cast<unsigned int>(port),
			nullptr,
			CLIENT_MULTI_STATEMENTS|CLIENT_FOUND_ROWS
		)==nullptr) throw std::runtime_error(mysql_error(&conn));
	
	}
	
	
	//	Makes sure the server is still there,
	//	and if it disconnected and had to be
	//	reconnected, cleans up all prepared
	//	statements so they'll be regenerated
	//	as needed.
	inline void MySQLDataProvider::keep_alive () {
	
		//	Ping the server to see if it's
		//	still there
		int result=mysql_ping(&conn);
		
		//	Server is fine
		if (result==0) return;
		
		//	Server is gone
		
		//	Clean up the old
		//	connection
		mysql_close(&conn);
		
		//	Reconnect
		connect();
	
	}
	
	
	//	Cleans up, closes statements,
	//	closes the connection, and
	//	cleans up the whole library
	inline void MySQLDataProvider::destroy () noexcept {
	
		//	Kill statements
		destroy_stmts();
		
		//	Kill connection
		mysql_close(&conn);
		
		//	Kill library
		mysql_library_end();
	
	}
	
	
	MySQLDataProvider::MySQLDataProvider (
		const String & host,
		UInt16 port,
		const String & username,
		const String & password,
		const String & database
	)	:	username(api_encode(username)),
			password(api_encode(password)),
			database(api_encode(database)),
			host(api_encode(host)),
			port(port),
			log_stmt(nullptr),
			setting_stmt(nullptr),
			pool(
				//	Single worker thread
				1,
				//	Initializer to be run
				//	in the worker before
				//	it starts work
				[this] () {
				
					//	Initialize libmysql
					if (mysql_library_init(
						0,
						nullptr,
						nullptr
					)!=0) throw std::runtime_error(mysql_init_failed);
					
					try {
					
						//	Connect to the database
						connect();
					
					} catch (...) {
					
						//	Make sure the library
						//	gets cleaned up
						//	if we can't connect
						mysql_library_end();
						
						throw;
					
					}
				
				},
				//	Clean up all resources
				//	when the thread ends
				[this] () {	destroy();	}
			)
	{	}
	
	
	void MySQLDataProvider::WriteLog (const String & log, Service::LogType type) {
	
		//	Instruct the worker thread
		//	to write to the log
		pool.Enqueue([=] () {
			
			//	Prepare bound parameters
			MYSQL_BIND param [2];
			memset(param,0,sizeof(MYSQL_BIND)*2);
			
			//	Parameter #1 - Log type, string
			param[0].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> type_encoded(
				db_encode(
					//	Map log type to a meaningful
					//	string
					DataProvider::GetLogType(type)
				)
			);
			param[0].buffer=to_char(type_encoded);
			param[0].buffer_length=type_encoded.Length();
			
			//	Parameter #2 - Log text, string
			param[1].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> log_encoded(db_encode(log));
			param[1].buffer=to_char(log_encoded);
			param[1].buffer_length=log_encoded.Length();
			
			//	Is the server still there?
			keep_alive();
			
			//	Regenerate prepared statement
			//	if necessary
			prepare_stmt(log_stmt,log_query);
			
			//	Perform INSERT
			if (
				(mysql_stmt_bind_param(log_stmt,param)!=0) ||
				(mysql_stmt_execute(log_stmt)!=0)
			) throw std::runtime_error(mysql_stmt_error(log_stmt));
			
		});
	
	}
	
	
	Nullable<String> MySQLDataProvider::GetSetting (const String & setting) {
	
		//	Instruct the worker thread
		//	to retrieve the value
		//	of the requested setting
		auto handle=pool.Enqueue([&] () -> Nullable<String> {
		
			//	Setup parameter
			MYSQL_BIND param;
			memset(&param,0,sizeof(MYSQL_BIND));
			param.buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> setting_encoded(db_encode(setting));
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
			
			//	Is the server still there?
			keep_alive();
			
			//	Regenerate prepared statement
			//	if necessary
			prepare_stmt(setting_stmt,setting_query);
			
			//	Bind, execute, and fetch
			int success;
			if (
				(mysql_stmt_bind_param(setting_stmt,&param)!=0) ||
				(mysql_stmt_bind_result(setting_stmt,&result)!=0) ||
				(mysql_stmt_execute(setting_stmt)!=0) ||
				((success=mysql_stmt_fetch(setting_stmt))==1)
			) throw std::runtime_error(mysql_stmt_error(setting_stmt));
			
			//	Value to return, defaults to null
			Nullable<String> value;
			
			//	Check to see if
			//	data exists
			if (!(
				(success==MYSQL_NO_DATA) ||
				is_null
			)) {
			
				//	There is data
				
				//	Empty string
				if (real_length==0) {
				
					value.Construct();
				
				//	Non-empty string, create buffer
				//	and retrieve result
				} else {
				
					//	Allocate a sufficiently-sized
					//	buffer
					//
					//	Make sure integer conversions
					//	are safe
					Word buffer_len=Word(
						SafeInt<
							decltype(real_length)
						>(
							real_length
						)
					);
					Vector<Byte> buffer(buffer_len);
					
					//	Wire buffer into result bind
					result.buffer=to_char(buffer);
					result.buffer_length=real_length;
					
					//	Retrieve actual data
					if (mysql_stmt_fetch_column(
						setting_stmt,
						&result,
						0,
						0
					)!=0) throw std::runtime_error(mysql_stmt_error(setting_stmt));
					
					//	Success, data in buffer,
					//	set count
					buffer.SetCount(buffer_len);
					
					//	Decode string
					value.Construct(
						UTF8().Decode(
							buffer.begin(),
							buffer.end()
						)
					);
				
				}
			
			}
			
			//	Flush all results out of
			//	result set so that more
			//	commands to the server may
			//	be executed
			while (success!=MYSQL_NO_DATA) {
			
				success=mysql_stmt_fetch(setting_stmt);
				
				//	Make sure there are no errors
				if (success==1) throw std::runtime_error(mysql_stmt_error(setting_stmt));
			
			}
			
			//	Return
			return value;
		
		});
		
		//	Wait for the worker thread
		//	to finish retrieving the
		//	setting
		handle->Wait();
		
		//	Was the worker successful?
		if (handle->Success()) return Nullable<String>(
			std::move(
				*(handle->Result<Nullable<String>>())
			)
		);
		
		//	Failure
		throw std::runtime_error(mysql_failed);
	
	}


}

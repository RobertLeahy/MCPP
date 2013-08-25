#include <mysql_data_provider.hpp>
#include <cstring>
#include <type_traits>
#ifdef ENVIRONMENT_WINDOWS
#include <errmsg.h>
#else
#include <mysql/errmsg.h>
#endif
#include <utility>


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
	static const char * kvp_query="SELECT `value` FROM `keyvaluepairs` WHERE `key`=?";
	static const char * kvp_delete_pair_query="DELETE FROM `keyvaluepairs` WHERE `key`=? AND `value`=?";
	static const char * kvp_insert_query="INSERT INTO `keyvaluepairs` (`key`,`value`) VALUES (?,?)";
	static const char * chat_log_query="INSERT INTO `chat_log` (`from`,`to`,`message`,`notes`) VALUES (?,?,?,?)";
	
	
	//	String constants
	static const String requests_label("Requests");
	static const String elapsed_label("Running");
	static const String reconnects_label("Reconnects");
	static const String avg_running_label("Average Time Per Query");
	static const String ns_template("{0}ns");
	static const String queued_label("Queued Requests");
	static const String server_v("Server Version");
	static const String server_info_template("MySQL Server Version {0}");
	static const String client_v("Client Version");
	static const String client_info_template("libmysql Version {0}, Threadsafe: {1}");
	static const String yes("Yes");
	static const String no("No");
	static const String thread_label("Server Thread ID");
	static const String name("MySQL Data Provider");
	static const String host_label("Host");
	static const String proto_label("Protocol Version");
	
	
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
		destroy_stmt(kvp_stmt);
		destroy_stmt(kvp_delete_pair_stmt);
		destroy_stmt(kvp_insert_stmt);
		destroy_stmt(chat_log_stmt);
		destroy_stmt(column_stmt);
		destroy_stmt(column_insert_stmt);
		destroy_stmt(column_update_stmt);
	
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
			#ifdef ENVIRONMENT_WINDOWS
			"utf8mb4"
			#else
			"utf8"
			#endif
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
		
		++reconnects;
		
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
	
	
	template <typename T>
	inline void bind (MYSQL_BIND & b, const T & subject) noexcept {
	
		switch (sizeof(T)) {
		
			case 1:
			
				b.buffer_type=MYSQL_TYPE_TINY;
				break;
				
			case 2:
			
				b.buffer_type=MYSQL_TYPE_SHORT;
				break;
				
			case 4:
			
				b.buffer_type=MYSQL_TYPE_LONG;
				break;
				
			case 8:
			default:
			
				b.buffer_type=MYSQL_TYPE_LONGLONG;
				break;
		
		}
		
		b.buffer=const_cast<T *>(&subject);
		b.buffer_length=sizeof(T);
		b.is_unsigned=std::is_unsigned<T>::value;
	
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
			kvp_stmt(nullptr),
			kvp_delete_pair_stmt(nullptr),
			kvp_insert_stmt(nullptr),
			chat_log_stmt(nullptr),
			column_stmt(nullptr),
			column_insert_stmt(nullptr),
			column_update_stmt(nullptr),
			pool(
				//	Single worker thread
				1,
				//	On panic do nothing
				std::function<void ()>(),
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
			),
			requests(0),
			reconnects(0)
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
			
			++requests;
			
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
			
			++requests;
			
			//	Return
			return value;
		
		});
		
		//	Wait for the worker thread
		//	to finish retrieving the
		//	setting
		handle->Wait();
		
		//	Was the worker successful?
		if (handle->Success()) return std::move(
			*(handle->Result<Nullable<String>>())
		);
		
		//	Failure
		throw std::runtime_error(mysql_failed);
	
	}
	
	
	Vector<Nullable<String>> MySQLDataProvider::GetValues (const String & key) {
	
		//	Dispatch worker thread
		//	to retrieve value
		auto handle=pool.Enqueue([&] () -> Vector<Nullable<String>> {
		
			//	Setup parameter
			MYSQL_BIND param;
			memset(&param,0,sizeof(MYSQL_BIND));
			param.buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> key_encoded(db_encode(key));
			param.buffer=to_char(key_encoded);
			param.buffer_length=key_encoded.Length();
			
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
			
			//	Regenerate prepared statement if
			//	necessary
			prepare_stmt(kvp_stmt,kvp_query);
			
			//	Bind and execute
			if (
				(mysql_stmt_bind_param(kvp_stmt,&param)!=0) ||
				(mysql_stmt_bind_result(kvp_stmt,&result)!=0) ||
				(mysql_stmt_execute(kvp_stmt)!=0)
			) throw std::runtime_error(mysql_stmt_error(kvp_stmt));
			
			//	Prepare a buffer to return
			Vector<Nullable<String>> buffer;
			
			//	Loop, execute, and fetch
			int success;
			for (;;) {
			
				//	Fetch
				if ((success=mysql_stmt_fetch(kvp_stmt))==1) throw std::runtime_error(
					mysql_stmt_error(
						kvp_stmt
					)
				);
			
				//	We're done if there's no
				//	more data
				if (success==MYSQL_NO_DATA) break;
				
				//	Current value
				Nullable<String> value;
				
				//	No further processing needed
				//	if the database value is null
				if (!is_null) {
				
					//	If it's the empty string all
					//	we need to do is default construct
					//	a string and we're good
					if (real_length==0) {
					
						value.Construct();
					
					//	Otherwise we need to extract the
					//	value from the database
					} else {
					
						//	Allocate a sufficiently-sized
						//	buffer to hold the raw data
						//	from the database
						Word buffer_len=Word(
							SafeInt<
								decltype(real_length)
							>(
								real_length
							)
						);
						Vector<Byte> from_db(buffer_len);
						
						//	Wire buffer into the result bind
						result.buffer=to_char(from_db);
						result.buffer_length=real_length;
						
						//	Retrieve actual data
						if (mysql_stmt_fetch_column(
							kvp_stmt,
							&result,
							0,
							0
						)!=0) throw std::runtime_error(mysql_stmt_error(kvp_stmt));
						
						//	Success, butter in data,
						//	set the count
						from_db.SetCount(buffer_len);
						
						//	Decode the string
						value.Construct(
							UTF8().Decode(
								from_db.begin(),
								from_db.end()
							)
						);
						
						//	Make sure the result bind
						//	is reset to a consistent/sane
						//	state
						result.buffer=nullptr;
						result.buffer_length=0;
					
					}
				
				}
				
				//	Add the extracted value
				//	to our list of values
				buffer.Add(std::move(value));
				
			}
			
			++requests;
			
			//	All values extracted, return
			return buffer;
		
		});
		
		//	Wait for database operation
		//	to complete
		handle->Wait();
		
		//	Did the worker succeed?
		if (handle->Success()) return std::move(
			*(handle->Result<Vector<Nullable<String>>>())
		);
		
		//	Failure
		throw std::runtime_error(mysql_failed);
	
	}
	
	
	void MySQLDataProvider::DeleteValues (const String & key, const String & value) {
	
		//	Dispatch worker to delete
		//	from the database
		auto handle=pool.Enqueue([&] () {
		
			//	Setup parameters
			MYSQL_BIND param [2];
			memset(param,0,sizeof(MYSQL_BIND)*2);
			
			//	Parameter #1 - Key
			param[0].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> key_encoded(db_encode(key));
			param[0].buffer=to_char(key_encoded);
			param[0].buffer_length=key_encoded.Length();
			
			//	Parameter #2 - Value
			param[1].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> value_encoded(db_encode(value));
			param[1].buffer=to_char(value_encoded);
			param[1].buffer_length=value_encoded.Length();
			
			//	Is the server still there?
			keep_alive();
			
			//	Regenerate the prepared statement
			//	if necessary
			prepare_stmt(kvp_delete_pair_stmt,kvp_delete_pair_query);
			
			//	Bind and execute
			if (
				(mysql_stmt_bind_param(kvp_delete_pair_stmt,param)!=0) ||
				(mysql_stmt_execute(kvp_delete_pair_stmt)!=0)
			) throw std::runtime_error(mysql_stmt_error(kvp_delete_pair_stmt));
			
			++requests;
		
		});
		
		//	Wait for worker to finish
		handle->Wait();
		
		//	Throw on error
		if (!handle->Success()) throw std::runtime_error(mysql_failed);
	
	}
	
	
	void MySQLDataProvider::InsertValue (const String & key, const String & value) {
	
		//	Dispatch worker to insert
		//	into the database
		auto handle=pool.Enqueue([&] () {
		
			//	Setup parameters
			MYSQL_BIND param [2];
			memset(param,0,sizeof(MYSQL_BIND)*2);
			
			//	Parameter #1 - Key
			param[0].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> key_encoded(db_encode(key));
			param[0].buffer=to_char(key_encoded);
			param[0].buffer_length=key_encoded.Length();
			
			//	Parameter #2 - Value
			param[1].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> value_encoded(db_encode(value));
			param[1].buffer=to_char(value_encoded);
			param[1].buffer_length=value_encoded.Length();
			
			//	Is the server still there?
			keep_alive();
			
			//	Regenerate the prepared statement
			//	if necessary
			prepare_stmt(kvp_insert_stmt,kvp_insert_query);
			
			//	Bind and execute
			if (
				(mysql_stmt_bind_param(kvp_insert_stmt,param)!=0) ||
				(mysql_stmt_execute(kvp_insert_stmt)!=0)
			) throw std::runtime_error(mysql_stmt_error(kvp_insert_stmt));
			
			++requests;
		
		});
			
		//	Wait for worker to finish
		handle->Wait();
		
		//	Throw on error
		if (!handle->Success()) throw std::runtime_error(mysql_failed);
	
	}
	
	
	void MySQLDataProvider::WriteChatLog (const String & from, const Vector<String> & to, const String & message, const Nullable<String> & notes) {
	
		//	We tell the worker thread to
		//	log and then we proceed
		pool.Enqueue([=] () {
		
			//	Setup parameters
			MYSQL_BIND param [4];
			memset(param,0,sizeof(MYSQL_BIND)*4);
			
			//	Parameter #1 - Sender
			param[0].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> from_encoded(db_encode(from));
			param[0].buffer=to_char(from_encoded);
			param[0].buffer_length=from_encoded.Length();
			
			//	Parameter #2 - Recipients
			param[1].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> to_encoded;
			my_bool to_is_null=to.Count()==0;
			if (to_is_null) {
			
				param[1].is_null=&to_is_null;
			
			} else {
			
				String to_str;
				for (Word i=0;i<to.Count();++i) {
				
					if (i!=0) to_str << ", ";
					
					to_str << to[i];
				
				}
				
				to_encoded=db_encode(to_str);
				
				param[1].buffer=to_char(to_encoded);
				param[1].buffer_length=to_encoded.Length();
			
			}
			
			//	Parameter #3 - Message
			param[2].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> message_encoded(db_encode(message));
			param[2].buffer=to_char(message_encoded);
			param[2].buffer_length=message_encoded.Length();
			
			//	Parameter #4 - Notes (can be null)
			param[3].buffer_type=MYSQL_TYPE_STRING;
			Vector<Byte> notes_encoded;
			my_bool notes_is_null=notes.IsNull();
			if (notes_is_null) {
			
				param[3].is_null=&notes_is_null;
			
			} else {
			
				notes_encoded=db_encode(*notes);
				param[3].buffer=to_char(notes_encoded);
				param[3].buffer_length=notes_encoded.Length();
			
			}
			
			//	Is the server still there?
			keep_alive();
			
			//	Regenerate prepared statement if necessary
			prepare_stmt(chat_log_stmt,chat_log_query);
			
			//	Bind and execute
			if (
				(mysql_stmt_bind_param(chat_log_stmt,param)!=0) ||
				(mysql_stmt_execute(chat_log_stmt)!=0)
			) throw std::runtime_error(mysql_stmt_error(chat_log_stmt));
			
			++requests;
		
		});
	
	}
	
	
	auto MySQLDataProvider::GetInfo () const -> InfoType {
	
		InfoType info;
		
		//	Dispatch worker
		auto handle=pool.Enqueue([&] () {
		
			auto & kvps=info.Item<1>();
			
			//	Number of requests executed
			kvps.EmplaceBack(
				requests_label,
				requests
			);
			//	Number of reconnects
			kvps.EmplaceBack(
				reconnects_label,
				reconnects
			);
			
			//	Get information from the pool
			auto pool_info=pool.GetInfo();
			
			//	Number of nanoseconds spent
			//	executing queries
			kvps.EmplaceBack(
				elapsed_label,
				String::Format(
					ns_template,
					pool_info.WorkerInfo[0].Running
				)
			);
			//	Average time per query
			kvps.EmplaceBack(
				avg_running_label,
				String::Format(
					ns_template,
					pool_info.WorkerInfo[0].Running/requests
				)
			);
			//	Queued tasks
			kvps.EmplaceBack(
				queued_label,
				pool_info.Queued
			);
			
			//	About client
			kvps.EmplaceBack(
				client_v,
				String::Format(
					client_info_template,
					mysql_get_client_info(),
					(mysql_thread_safe()==1) ? yes : no
				)
			);
			
			//	About server
			kvps.EmplaceBack(
				server_v,
				String::Format(
					server_info_template,
					mysql_get_server_info(&conn)
				)
			);
			
			//	Host
			kvps.EmplaceBack(
				host_label,
				mysql_get_host_info(&conn)
			);
			
			//	Protocol
			kvps.EmplaceBack(
				proto_label,
				mysql_get_proto_info(&conn)
			);
			
			//	Thread ID
			kvps.EmplaceBack(
				thread_label,
				mysql_thread_id(&conn)
			);
		
		});
		
		handle->Wait();
		
		if (!handle->Success()) throw std::runtime_error(mysql_failed);
		
		info.Item<0>()=name;
		
		return info;
	
	}


}

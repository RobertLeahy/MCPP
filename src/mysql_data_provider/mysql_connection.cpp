#include <mysql_data_provider/mysql_data_provider.hpp>
#include <stdexcept>


namespace MCPP {


	static const char * mysql_init_failed="mysql_init failed";


	static inline Nullable<Vector<Byte>> param_process (const Nullable<String> & param) {
	
		Nullable<Vector<Byte>> retr;
		
		if (param.IsNull()) return retr;
		
		retr.Construct(
			UTF8().Encode(*param)
		);
		//	Null terminator
		retr->Add(0);
		
		return retr;
	
	}
	
	
	static inline const char * to_conn (const Nullable<Vector<Byte>> & arg) noexcept {
	
		return arg.IsNull() ? nullptr : reinterpret_cast<const char *>(arg->begin());
	
	}
	
	
	static inline void destroy_stmt (MYSQL_STMT * & stmt) noexcept {
	
		if (stmt!=nullptr) {
		
			mysql_stmt_close(stmt);
			
			stmt=nullptr;
		
		}
	
	}
	
	
	inline void MySQLConnection::destroy_stmts () noexcept {
	
		destroy_stmt(get_bin_stmt);
		destroy_stmt(update_bin_stmt);
		destroy_stmt(insert_bin_stmt);
		destroy_stmt(delete_bin_stmt);
		destroy_stmt(log_stmt);
		destroy_stmt(chat_stmt);
		destroy_stmt(get_setting_stmt);
		destroy_stmt(update_setting_stmt);
		destroy_stmt(insert_setting_stmt);
		destroy_stmt(delete_setting_stmt);
		destroy_stmt(get_kvp_stmt);
		destroy_stmt(insert_kvp_stmt);
		destroy_stmt(delete_key_stmt);
		destroy_stmt(delete_kvp_stmt);
	
	}
	
	
	inline void MySQLConnection::connect (
		const Nullable<String> & host,
		Nullable<UInt16> port,
		const Nullable<String> & username,
		const Nullable<String> & password,
		const Nullable<String> & database,
		bool reconnect
	) {
	
		//	Clean up statements and connection
		//	(if necessary)
		if (reconnect) {
		
			destroy_stmts();
			
			mysql_close(&conn);
		
		}
		
		//	Initialize connection handle
		if (mysql_init(&conn)==nullptr) throw std::runtime_error(mysql_init_failed);
		
		//	Convert arguments
		auto u=param_process(username);
		auto p=param_process(password);
		auto d=param_process(database);
		auto h=param_process(host);
		
		//	Set charset to UTF-8 for Unicode
		//	support
		mysql_options(
			&conn,
			MYSQL_SET_CHARSET_NAME,
			"utf8mb4"
		);
		
		if (mysql_real_connect(
			&conn,
			to_conn(h),
			to_conn(u),
			to_conn(p),
			to_conn(d),
			port.IsNull() ? 0 : *port,
			nullptr,
			CLIENT_MULTI_STATEMENTS|CLIENT_FOUND_ROWS
		)==nullptr) throw std::runtime_error(mysql_error(&conn));
	
	}
	
	
	MySQLConnection::MySQLConnection (
		const Nullable<String> & host,
		Nullable<UInt16> port,
		const Nullable<String> & username,
		const Nullable<String> & password,
		const Nullable<String> & database
	)	:	get_bin_stmt(nullptr),
			update_bin_stmt(nullptr),
			insert_bin_stmt(nullptr),
			delete_bin_stmt(nullptr),
			log_stmt(nullptr),
			chat_stmt(nullptr),
			get_setting_stmt(nullptr),
			update_setting_stmt(nullptr),
			insert_setting_stmt(nullptr),
			delete_setting_stmt(nullptr),
			get_kvp_stmt(nullptr),
			insert_kvp_stmt(nullptr),
			delete_key_stmt(nullptr),
			delete_kvp_stmt(nullptr)
	{
	
		connect(
			host,
			port,
			username,
			password,
			database,
			false
		);
	
	}
	
	
	MySQLConnection::~MySQLConnection () noexcept {
	
		//	Destroy statements
		destroy_stmts();
		
		//	Close connection
		mysql_close(&conn);
	
	}
	
	
	bool MySQLConnection::KeepAlive (
		const Nullable<String> & host,
		Nullable<UInt16> port,
		const Nullable<String> & username,
		const Nullable<String> & password,
		const Nullable<String> & database
	) {
	
		//	Ping the server to see if it's
		//	still there
		if (mysql_ping(&conn)==0) return false;
		
		//	Server is gone
		
		connect(
			host,
			port,
			username,
			password,
			database,
			false
		);
		
		return true;
	
	}
	
	
	void MySQLConnection::raise (MYSQL_STMT * stmt) {
	
		throw std::runtime_error(
			mysql_stmt_error(
				stmt
			)
		);
	
	}
	
	
	void MySQLConnection::prepare_stmt (MYSQL_STMT * & stmt, const char * query) {
	
		if (stmt!=nullptr) return;
		
		if ((stmt=mysql_stmt_init(&conn))==nullptr) throw std::runtime_error(
			mysql_error(&conn)
		);
		
		try {
		
			if (mysql_stmt_prepare(
				stmt,
				query,
				strlen(query)
			)!=0) raise(stmt);
		
		} catch (...) {
		
			destroy_stmt(stmt);
			
			throw;
		
		}
	
	}


}

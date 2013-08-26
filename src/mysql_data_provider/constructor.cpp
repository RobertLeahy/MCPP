#include <mysql_data_provider/mysql_data_provider.hpp>
#include <stdexcept>


namespace MCPP {


	static const char * mysql_init_failed="mysql_init failed";
	static const char * mysql_library_init_failed="mysql_library_init failed";


	static inline Nullable<Vector<Byte>> param_process (const Nullable<String> & param) {
	
		Nullable<Vector<Byte>> retr;
		
		if (!param.IsNull()) {
		
			retr.Construct(
				UTF8().Encode(*param)
			);
			//	C string (for MySQL
			//	C API) is null-terminated
			retr->Add(0);
			
		}
		
		return retr;
	
	}
	
	
	static inline const char * to_conn (const Nullable<Vector<Byte>> & arg) noexcept {
	
		return arg.IsNull() ? nullptr : reinterpret_cast<const char *>(arg->begin());
	
	}
	
	
	inline void MySQLDataProvider::destroy_stmts () noexcept {
	
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
	
	
	void MySQLDataProvider::connect (bool reconnect) {
	
		//	Clean up statements
		destroy_stmts();
		
		//	Close connection if we're
		//	reconnecting
		if (reconnect) mysql_close(&conn);
	
		//	Initialize connection handle
		if (mysql_init(&conn)==nullptr) throw std::runtime_error(mysql_init_failed);
		
		//	Set charset to UTF-8 for Unicode
		//	support
		mysql_options(
			&conn,
			MYSQL_SET_CHARSET_NAME,
			"utf8mb4"
		);
		
		Timer timer(Timer::CreateAndStart());
		
		//	Connect
		if (mysql_real_connect(
			&conn,
			to_conn(host),
			to_conn(username),
			to_conn(password),
			to_conn(database),
			port.IsNull() ? 0 : *port,
			nullptr,
			CLIENT_MULTI_STATEMENTS|CLIENT_FOUND_ROWS
		)==nullptr) {
		
			timer.Stop();
			connecting+=timer.ElapsedNanoseconds();
		
			throw std::runtime_error(mysql_error(&conn));
			
		}
		
		timer.Stop();
		connecting+=timer.ElapsedNanoseconds();
		
		if (reconnect) ++reconnects;
	
	}


	MySQLDataProvider::MySQLDataProvider (
		const Nullable<String> & host,
		Nullable<UInt16> port,
		const Nullable<String> & username,
		const Nullable<String> & password,
		const Nullable<String> & database
	)	:	username(param_process(username)),
			password(param_process(password)),
			database(param_process(database)),
			host(param_process(host)),
			port(std::move(port)),
			get_bin_stmt(nullptr),
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
			delete_kvp_stmt(nullptr),
			reconnects(0),
			requests(0),
			waited(0),
			waiting(0),
			executing(0),
			connecting(0)
	{
	
		//	Initialize libmysqlclient
		if (mysql_library_init(
			0,
			nullptr,
			nullptr
		)!=0) throw std::runtime_error(mysql_library_init_failed);
		
		try {
			
			//	Connect to the database
			connect(false);
			
		} catch (...) {
		
			//	Make sure the library is
			//	cleaned up if we cannot
			//	connect
			mysql_library_end();
			
			throw;
		
		}
	
	}
	
	
	MySQLDataProvider::~MySQLDataProvider () noexcept {
	
		//	Kill statements
		destroy_stmts();
		
		//	Kill connection
		mysql_close(&conn);
		
		//	Kill library
		mysql_library_end();
	
	}


}

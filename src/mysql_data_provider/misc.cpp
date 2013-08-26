#include <mysql_data_provider/mysql_data_provider.hpp>
#include <stdexcept>


namespace MCPP {


	static const char * mysql_thread_init_failed="mysql_thread_init failed";


	void MySQLDataProvider::thread_init () {
	
		if (mysql_thread_init()!=0) throw std::runtime_error(mysql_thread_init_failed);
	
	}
	
	
	void MySQLDataProvider::thread_end () {
	
		mysql_thread_end();
	
	}


	void MySQLDataProvider::destroy_stmt (MYSQL_STMT * & stmt) noexcept {
	
		if (stmt!=nullptr) {
		
			mysql_stmt_close(stmt);
			
			stmt=nullptr;
		
		}
	
	}


	void MySQLDataProvider::prepare_stmt (MYSQL_STMT * & stmt, const char * query) {
	
		if (stmt==nullptr) {
		
			if ((stmt=mysql_stmt_init(&conn))==nullptr) throw std::runtime_error(
				mysql_error(&conn)
			);
			
			try {
			
				if (mysql_stmt_prepare(
					stmt,
					query,
					strlen(query)
				)!=0) throw std::runtime_error(mysql_stmt_error(stmt));
			
			} catch (...) {
			
				//	Make sure statement doesn't
				//	leak
				destroy_stmt(stmt);
				
				throw;
			
			}
		
		}
	
	}
	
	
	void MySQLDataProvider::keep_alive () {
	
		//	Ping the server to see if it's
		//	still there
		if (mysql_ping(&conn)!=0) {
		
			//	Server is gone
			
			++reconnects;
			
			//	Reconnect
			connect(true);
		
		}
	
	}
	
	
	void MySQLDataProvider::raise (MYSQL_STMT * stmt) {
	
		throw std::runtime_error(
			mysql_stmt_error(
				stmt
			)
		);
	
	}


}

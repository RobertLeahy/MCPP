#include <mysql_data_provider/mysql_data_provider.hpp>
#include <stdexcept>
#include <string>


namespace MCPP {


	namespace MySQL {
	
	
		void PreparedStatement::destroy () noexcept {
		
			if (handle!=nullptr) {
			
				mysql_stmt_close(handle);
			
				handle=nullptr;
			
			}
		
		}
		
		
		static const char * stmt_allocate="Could not allocate prepared statement";
	
	
		PreparedStatement::PreparedStatement (Connection & conn, const String & text) {
		
			//	Allocate a statement handle
			if ((handle=mysql_stmt_init(conn))==nullptr) throw std::runtime_error(stmt_allocate);
			
			//	Clean up the handle if something
			//	goes wrong
			try {
			
				//	Prepare the statement
				auto c_str=text.ToCString();
				if (mysql_stmt_prepare(
					handle,
					c_str.begin(),
					safe_cast<unsigned long>(c_str.Count())
				)!=0) Raise();
			
			} catch (...) {
			
				destroy();
				
				throw;
			
			}
		
		}
		
		
		PreparedStatement::PreparedStatement (PreparedStatement && other) noexcept : handle(other.handle) {
		
			other.handle=nullptr;
		
		}
		
		
		PreparedStatement & PreparedStatement::operator = (PreparedStatement && other) noexcept {
		
			if (&other!=this) {
			
				destroy();
				
				handle=other.handle;
				other.handle=nullptr;
			
			}
			
			return *this;
		
		}
		
		
		PreparedStatement::~PreparedStatement () noexcept {
		
			destroy();
		
		}
		
		
		static const char * generic_error="MySQL prepared statement error";
		
		
		void PreparedStatement::Raise () {
		
			std::string str;
			try {
			
				str=mysql_stmt_error(handle);
			
			} catch (...) {
			
				throw std::runtime_error(generic_error);
			
			}
			
			throw std::runtime_error(std::move(str));
		
		}
		
		
		void PreparedStatement::Execute () {
		
			if (mysql_stmt_execute(handle)!=0) Raise();
		
		}
		
		
		bool PreparedStatement::Fetch () {
		
			switch (mysql_stmt_fetch(handle)) {
			
				default:
					return true;
				case MYSQL_NO_DATA:
					return false;
				case 1:
					Raise();
			
			}
		
		}
		
		
		void PreparedStatement::Fetch (MYSQL_BIND & bind, Word column) {
		
			if (mysql_stmt_fetch_column(
				handle,
				&bind,
				safe_cast<unsigned long>(column),
				0
			)!=0) Raise();
		
		}
		
		
		void PreparedStatement::Complete () {
		
			while (Fetch());
		
		}
	
	
	}


}

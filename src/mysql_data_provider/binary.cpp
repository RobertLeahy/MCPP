#include <mysql_data_provider/mysql_data_provider.hpp>


namespace MCPP {


	static const char * get_query="SELECT `value` FROM `binary` WHERE `key`=?";
	static const char * insert_query="INSERT INTO `binary` (`value`,`key`) VALUES (?,?)";
	static const char * update_query="UPDATE `binary` SET `value`=? WHERE `key`=?";
	static const char * delete_query="DELETE FROM `binary` WHERE `key`=?";


	bool MySQLConnection::GetBinary (const String & key, void * ptr, Word * len) {
	
		//	Create input bind
		MySQLBind<String> param(key);
		
		//	Create output bind
		MYSQL_BIND result;
		memset(&result,0,sizeof(result));
		
		result.buffer_type=MYSQL_TYPE_BLOB;
		result.buffer=ptr;
		unsigned long real_len=*len;
		result.buffer_length=real_len;
		result.length=&real_len;
			
		//	Regenerate prepared statement if
		//	necessary
		prepare_stmt(
			get_bin_stmt,
			get_query
		);
		
		//	Bind and execute
		if (
			(mysql_stmt_bind_param(
				get_bin_stmt,
				param
			)!=0) ||
			(mysql_stmt_bind_result(
				get_bin_stmt,
				&result
			)!=0) ||
			(mysql_stmt_execute(get_bin_stmt)!=0)
		) raise(get_bin_stmt);
				
		//	Fetch data from the database
		int success=mysql_stmt_fetch(get_bin_stmt);
		
		if (success==1) raise(get_bin_stmt);
		
		//	There was no data
		if (success==MYSQL_NO_DATA) return false;
		
		//	Loop to finish up the result
		//	set
		result.buffer=nullptr;
		result.buffer_length=0;
		result.length=nullptr;
		do {
		
			success=mysql_stmt_fetch(get_bin_stmt);
			
			if (success==1) raise(get_bin_stmt);
		
		} while (success!=MYSQL_NO_DATA);
		
		*len=Word(
			SafeInt<unsigned long>(
				real_len
			)
		);
		
		return true;
	
	}
	
	
	bool MySQLDataProvider::GetBinary (const String & key, void * ptr, Word * len) {
	
		return execute([&] (std::unique_ptr<MySQLConnection> & conn) {	return conn->GetBinary(key,ptr,len);	});
	
	}
	
	
	void MySQLConnection::SaveBinary (const String & key, const void * ptr, Word len) {
	
		MySQLBindWrapper<String> wrap(key);
	
		//	Create input bind
		MYSQL_BIND param [2];
		memset(
			param,
			0,
			sizeof(param)
		);
		
		param[0].buffer_type=MYSQL_TYPE_BLOB;
		param[0].buffer=const_cast<void *>(ptr);
		param[0].buffer_length=len;
		wrap.Bind(param[1]);
			
		//	Prepare statement, bind,
		//	and execute update statement
		prepare_stmt(
			update_bin_stmt,
			update_query
		);
		
		if (
			(mysql_stmt_bind_param(
				update_bin_stmt,
				param
			)!=0) ||
			(mysql_stmt_execute(update_bin_stmt)!=0)
		) raise(update_bin_stmt);
			
		//	Check to see if the value already
		//	existed and was updated
		if (mysql_stmt_affected_rows(update_bin_stmt)==0) {
		
			//	The value did not exist,
			//	INSERT
			
			//	Prepare statement, bind,
			//	and execute insert statement
			prepare_stmt(
				insert_bin_stmt,
				insert_query
			);
			
			if (
				(mysql_stmt_bind_param(
					insert_bin_stmt,
					param
				)!=0) ||
				(mysql_stmt_execute(insert_bin_stmt)!=0)
			) raise(insert_bin_stmt);
			
		}
	
	}
	
	
	void MySQLDataProvider::SaveBinary (const String & key, const void * ptr, Word len) {
	
		return execute([&] (std::unique_ptr<MySQLConnection> & conn) {	return conn->SaveBinary(key,ptr,len);	});
	
	}
	
	
	void MySQLConnection::DeleteBinary (const String & key) {
	
		//	Create bind
		MySQLBind<String> param(key);
		
		//	Prepare statement
		prepare_stmt(
			delete_bin_stmt,
			delete_query
		);
		
		//	Bind and execute
		if (
			(mysql_stmt_bind_param(
				delete_bin_stmt,
				param
			)!=0) ||
			(mysql_stmt_execute(delete_bin_stmt)!=0)
		) raise(delete_bin_stmt);
	
	}
	
	
	void MySQLDataProvider::DeleteBinary (const String & key) {
	
		execute([&] (std::unique_ptr<MySQLConnection> & conn) {	conn->DeleteBinary(key);	});
	
	}


}

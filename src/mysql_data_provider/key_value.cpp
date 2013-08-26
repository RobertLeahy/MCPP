#include <mysql_data_provider/mysql_data_provider.hpp>


namespace MCPP {


	static const char * insert_query="INSERT INTO `keyvaluestore` (`key`,`value`) VALUES (?,?)";
	static const char * delete_key_query="DELETE FROM `keyvaluestore` WHERE `key`=?";
	static const char * delete_query="DELETE FROM `keyvaluestore` WHERE `key`=? AND `value`=?";
	static const char * get_query="SELECT `value` FROM `keyvaluestore` WHERE `key`=?";


	void MySQLDataProvider::InsertValue (const String & key, const String & value) {
	
		MySQLBind<String,String> param(key,value);
		
		acquire([&] () {
		
			//	Make sure prepared statement
			//	has been created and bind
			//	parameters
			prepare_stmt(
				insert_kvp_stmt,
				insert_query
			);
			if (mysql_stmt_bind_param(
				insert_kvp_stmt,
				param
			)!=0) raise(insert_kvp_stmt);
			
			//	INSERT
			execute([&] () {	if (mysql_stmt_execute(insert_kvp_stmt)!=0) raise(insert_kvp_stmt);	});
		
		});
	
	}
	
	
	void MySQLDataProvider::DeleteValues (const String & key, const String & value) {
	
		MySQLBind<String,String> param(key,value);
		
		acquire([&] () {
		
			//	Make sure prepared statement
			//	has been created anb bind
			//	parameters
			prepare_stmt(
				delete_kvp_stmt,
				delete_query
			);
			if (mysql_stmt_bind_param(
				delete_kvp_stmt,
				param
			)!=0) raise(delete_kvp_stmt);
			
			//	DELETE
			execute([&] () {	if (mysql_stmt_execute(delete_kvp_stmt)!=0) raise(delete_kvp_stmt);	});
		
		});
	
	}
	
	
	void MySQLDataProvider::DeleteValues (const String & key) {
	
		MySQLBind<String> param(key);
		
		acquire([&] () {
		
			//	Make sure prepared statement
			//	has been created anb bind
			//	parameters
			prepare_stmt(
				delete_key_stmt,
				delete_key_query
			);
			if (mysql_stmt_bind_param(
				delete_key_stmt,
				param
			)!=0) raise(delete_key_stmt);
			
			//	DELETE
			execute([&] () {	if (mysql_stmt_execute(delete_key_stmt)!=0) raise(delete_key_stmt);	});
		
		});
	
	}
	
	
	Vector<String> MySQLDataProvider::GetValues (const String & key) {
	
		MySQLBind<String> param(key);
		
		MYSQL_BIND result;
		memset(
			&result,
			0,
			sizeof(result)
		);
		result.buffer_type=MYSQL_TYPE_STRING;
		unsigned long real_len;
		result.length=&real_len;
		
		return acquire([&] () {
		
			//	Prepare statement
			//	and bind
			prepare_stmt(
				get_kvp_stmt,
				get_query
			);
			if (
				(mysql_stmt_bind_param(
					get_kvp_stmt,
					param
				)!=0) ||
				(mysql_stmt_bind_result(
					get_kvp_stmt,
					&result
				)!=0)
			) raise(get_kvp_stmt);
			
			return execute([&] () {
			
				Vector<String> retr;
				
				if (mysql_stmt_execute(get_kvp_stmt)!=0) raise(get_kvp_stmt);
				
				//	Loop for each row
				int success;
				while ((success=mysql_stmt_fetch(get_kvp_stmt))!=MYSQL_NO_DATA) {
				
					//	Check for error
					if (success==1) raise(get_kvp_stmt);
					
					//	Check data length
					if (real_len==0) {
					
						//	Empty string
						retr.EmplaceBack();
					
					} else {
					
						//	Prepare a buffer
						Word buffer_len=Word(SafeInt<decltype(real_len)>(real_len));
						Vector<Byte> buffer(buffer_len);
						
						result.buffer=static_cast<Byte *>(buffer);
						result.buffer_length=real_len;
						
						if (mysql_stmt_fetch_column(
							get_kvp_stmt,
							&result,
							0,
							0
						)!=0) raise(get_kvp_stmt);
						
						//	SUCCESS, there are now
						//	buffer_len bytes in the
						//	buffer, so set the count
						//	accordingly
						buffer.SetCount(buffer_len);
						
						//	Decode and insert into
						//	collection
						retr.EmplaceBack(
							UTF8().Decode(
								buffer.begin(),
								buffer.end()
							)
						);
						
						//	Reset bind
						result.buffer=nullptr;
						result.buffer_length=0;
					
					}
				
				}
				
				return retr;
			
			});
		
		});
	
	}


}

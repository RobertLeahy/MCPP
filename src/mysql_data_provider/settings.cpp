#include <mysql_data_provider/mysql_data_provider.hpp>


namespace MCPP {


	static const char * get_query="SELECT `value` FROM `settings` WHERE `setting`=?";
	static const char * update_query="UPDATE `settings` SET `value`=? WHERE `setting`=?";
	static const char * insert_query="INSERT INTO `settings` (`value`,`setting`) VALUES (?,?)";
	static const char * delete_query="DELETE FROM `settings` WHERE `setting`=?";


	Nullable<String> MySQLDataProvider::GetSetting (const String & setting) {
	
		//	Input bind
		MySQLBind<String> param(setting);
		
		//	Output bind
		MYSQL_BIND result;
		memset(
			&result,
			0,
			sizeof(result)
		);
		result.buffer_type=MYSQL_TYPE_STRING;
		my_bool is_null;
		result.is_null=&is_null;
		unsigned long real_len;
		result.length=&real_len;
		
		return acquire([&] () {
		
			//	Regenerate prepared statement
			//	and bind input/output buffers
			prepare_stmt(
				get_setting_stmt,
				get_query
			);
			if (
				(mysql_stmt_bind_param(
					get_setting_stmt,
					param
				)!=0) ||
				(mysql_stmt_bind_result(
					get_setting_stmt,
					&result
				)!=0)
			) raise(get_setting_stmt);
			
			//	Execute
			return execute([&] () {
			
				int success;
				if (
					(mysql_stmt_execute(get_setting_stmt)!=0) ||
					((success=mysql_stmt_fetch(get_setting_stmt))==1)
				) raise(get_setting_stmt);
				
				//	Value to return
				Nullable<String> value;
				
				//	Check to see if data exists
				if (!(
					(success==MYSQL_NO_DATA) ||
					is_null
				)) {
				
					//	There is data, but how
					//	much?
					
					if (real_len==0) {
					
						//	The empty string
						
						value.Construct();
					
					} else {
					
						//	Non-empty string
						
						//	Allocate sufficiently-sized
						//	buffer
						Word buffer_len=Word(SafeInt<decltype(real_len)>(real_len));
						Vector<Byte> buffer(buffer_len);
						
						//	Wire buffer into result bind
						result.buffer=static_cast<Byte *>(buffer);
						result.buffer_length=real_len;
						
						//	Retrieve actual data
						if (mysql_stmt_fetch_column(
							get_setting_stmt,
							&result,
							0,
							0
						)!=0) raise(get_setting_stmt);
						
						//	SUCCESS
						buffer.SetCount(buffer_len);
						
						//	Decode
						value.Construct(
							UTF8().Decode(
								buffer.begin(),
								buffer.end()
							)
						);
						
						result.buffer=nullptr;
						result.buffer_length=0;
					
					}
				
				}
				
				//	Flush all results out
				while (success!=MYSQL_NO_DATA)
				if ((success=mysql_stmt_fetch(get_setting_stmt))==1)
				raise(get_setting_stmt);
				
				return value;
			
			});
		
		});
	
	}
	
	
	void MySQLDataProvider::SetSetting (const String & setting, const Nullable<String> & value) {
	
		//	Deleting a setting is the same
		//	as NULLing it, so reroute requests
		//	to NULL a setting through to delete
		if (value.IsNull()) {
		
			DeleteSetting(setting);
		
			return;
		
		}
		
		//	Prepare a bind
		MySQLBind<String,String> param(*value,setting);
		
		acquire([&] () {
		
			//	Prepare statement and
			//	bind for update
			prepare_stmt(
				update_setting_stmt,
				update_query
			);
			if (mysql_stmt_bind_param(
				update_setting_stmt,
				param
			)!=0) raise(update_setting_stmt);
			
			//	Attempt UPDATE
			execute([&] () {	if (mysql_stmt_execute(update_setting_stmt)!=0) raise(update_setting_stmt);	});
			
			//	Check to see if the value already
			//	existed and was updated
			if (mysql_stmt_affected_rows(update_setting_stmt)==0) {
			
				//	The value did not exist,
				//	INSERT
				
				//	Prepare statement and bind
				//	for insert statement
				prepare_stmt(
					insert_setting_stmt,
					insert_query
				);
				if (mysql_stmt_bind_param(
					insert_setting_stmt,
					param
				)!=0) raise(insert_setting_stmt);
				
				//	Attempt INSERT
				execute([&] () {	if (mysql_stmt_execute(insert_setting_stmt)!=0) raise(insert_setting_stmt);	});
			
			}
		
		});
	
	}
	
	
	void MySQLDataProvider::DeleteSetting (const String & setting) {
	
		//	Prepare a bind
		MySQLBind<String> param(setting);
		
		acquire([&] () {
		
			//	Prepare and bind
			prepare_stmt(
				delete_setting_stmt,
				delete_query
			);
			if (mysql_stmt_bind_param(
				delete_setting_stmt,
				param
			)!=0) raise(delete_setting_stmt);
			
			//	DELETE
			execute([&] () {	if (mysql_stmt_execute(delete_setting_stmt)!=0) raise(delete_setting_stmt);	});
		
		});
	
	}


}

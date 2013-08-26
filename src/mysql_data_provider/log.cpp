#include <mysql_data_provider/mysql_data_provider.hpp>


namespace MCPP {


	static const char * query="INSERT INTO `log` (`type`,`text`) VALUES (?,?)";
	static const char * chat_query="INSERT INTO `chat_log` (`from`,`to`,`message`,`notes`) VALUES (?,?,?,?)";


	void MySQLDataProvider::WriteLog (const String & log, Service::LogType type) {
	
		MySQLBind<String,String> param(
			log,
			DataProvider::GetLogType(type)
		);
		
		acquire([&] () {
		
			//	Regenerate prepared statement
			//	if necessary and bind params
			prepare_stmt(
				log_stmt,
				query
			);
			if (mysql_stmt_bind_param(
				log_stmt,
				param
			)!=0) raise(log_stmt);
			
			//	Execute
			execute([&] () {	if (mysql_stmt_execute(log_stmt)!=0) raise(log_stmt);	});
		
		});
	
	}
	
	
	void MySQLDataProvider::WriteChatLog (const String & from, const Vector<String> & to, const String & message, const Nullable<String> & notes) {
	
		//	Create a string representation
		//	of the list of people to whom
		//	this message was sent
		Nullable<String> to_str;
		if (to.Count()!=0) {
		
			to_str.Construct();
			
			for (const auto & str : to) {
			
				if (to_str->Size()!=0) (*to_str) << ", ";
				
				(*to_str) << str;
			
			}
		
		}
	
		//	Create bind
		MySQLBind<
			String,
			Nullable<String>,
			String,
			Nullable<String>
		> param(
			from,
			to_str,
			message,
			notes
		);
		
		acquire([&] () {
		
			//	Regenerate prepared statement
			//	if necessary and bind params
			prepare_stmt(
				chat_stmt,
				chat_query
			);
			if (mysql_stmt_bind_param(
				chat_stmt,
				param
			)!=0) raise(chat_stmt);
			
			//	Execute
			execute([&] () {	if (mysql_stmt_execute(chat_stmt)!=0) raise(chat_stmt);	});
		
		});
	
	}
	


}

#include <mysql_data_provider/mysql_data_provider.hpp>


namespace MCPP {


	static const String name("MySQL Data Provider");
	
	
	static const String host_label("Host");
	static const String protocol_label("Protocol Version");
	static const String server_version("Server Version");
	static const String server_version_template("MySQL Server Version {0}");


	Vector<Tuple<String,String>> MySQLConnection::GetInfo () {
	
		Vector<Tuple<String,String>> retr;
		
		//	About server
		retr.EmplaceBack(
			server_version,
			String::Format(
				server_version_template,
				mysql_get_server_info(&conn)
			)
		);
		//	Host
		retr.EmplaceBack(
			host_label,
			mysql_get_host_info(&conn)
		);
		//	Protocol
		retr.EmplaceBack(
			protocol_label,
			mysql_get_proto_info(&conn)
		);
		
		return retr;
	
	}
	
	
	static inline UInt64 avg (UInt64 time, Word num) noexcept {
	
		return (num==0) ? 0 : (time/num);
	
	}
	
	
	static const String client_version("Client Version");
	static const String client_version_template("libmysqlclient Version {0}, Threadsafe: {1}");
	static const String no("No");
	static const String yes("Yes");
	
	
	static const String waiting_label("Waiting");
	static const String connecting_label("Connecting");
	static const String executing_label("Executing");
	static const String stat_template("{0}, {1}ns (total), {2}ns (average)");
	
	
	static const String pool_label("Connection Pool");
	static const String pool_template("{0} (current), {1} (maximum)");
	static const String unlimited("Unlimited");
	
	
	Tuple<String,Vector<Tuple<String,String>>> MySQLDataProvider::GetInfo () {
	
		//	Get information that requires
		//	a connection
		auto retr=execute([] (std::unique_ptr<MySQLConnection> & conn) {	return conn->GetInfo();	});
		
		//	About libmysqlclient
		retr.EmplaceBack(
			client_version,
			String::Format(
				client_version_template,
				mysql_get_client_info(),
				(mysql_thread_safe()==0) ? no : yes
			)
		);
		
		//	Executing
		retr.EmplaceBack(
			executing_label,
			String::Format(
				stat_template,
				Word(executed),
				UInt64(executing),
				avg(
					executing,
					executed
				)
			)
		);
		
		//	Waiting (only if applicable)
		if (pool_max!=0) retr.EmplaceBack(
			waiting_label,
			String::Format(
				stat_template,
				Word(waited),
				UInt64(waiting),
				avg(
					waiting,
					waited
				)
			)
		);
		
		//	Connecting
		retr.EmplaceBack(
			connecting_label,
			String::Format(
				stat_template,
				Word(connected),
				UInt64(connecting),
				avg(
					connecting,
					connected
				)
			)
		);
		
		//	Connection pool information
		lock.Acquire();
		Word curr=pool_curr;
		lock.Release();
		
		retr.EmplaceBack(
			pool_label,
			String::Format(
				pool_template,
				curr,
				(pool_max==0) ? unlimited : pool_max
			)
		);
	
		return Tuple<String,Vector<Tuple<String,String>>>(
			name,
			std::move(retr)
		);
	
	}


}

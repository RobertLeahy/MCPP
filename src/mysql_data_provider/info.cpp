#include <mysql_data_provider/mysql_data_provider.hpp>


namespace MCPP {


	static const String name("MySQL Data Provider");


	static const String requests_label("Queries");
	static const String reconnects_label("Reconnects");
	static const String executing_label("Executing");
	static const String ns_template("{0}ns");
	static const String client_version("Client Version");
	static const String client_version_template("libmysqlclient Version {0}, Threadsafe: {1}");
	static const String server_version("Server Version");
	static const String server_version_template("MySQL Server Version {0}");
	static const String host_label("Host");
	static const String protocol_label("Protocol Version");
	static const String thread_label("Server Thread ID");
	static const String yes("Yes");
	static const String no("No");
	static const String lock_acquired_label("Locks Acquired");
	static const String connecting_label("Connecting");
	static const String waiting_label("Waiting");
	static const String avg_connecting_label("Average Time Per Connect");
	static const String avg_executing_label("Average Time Per Query");
	static const String avg_waiting_label("Average Time Per Lock");
	
	
	static inline UInt64 avg (UInt64 time, Word num) noexcept {
		
		return (num==0) ? 0 : (time/num);
	
	}


	Tuple<String,Vector<Tuple<String,String>>> MySQLDataProvider::GetInfo () {
	
		Vector<Tuple<String,String>> kvps;
	
		acquire([&] () {
			
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
			
			//	Number of times lock has
			//	been acquired
			kvps.EmplaceBack(
				lock_acquired_label,
				waited
			);
			
			//	Number of nanoseconds spent
			//	reconnecting
			kvps.EmplaceBack(
				connecting_label,
				String::Format(
					ns_template,
					connecting
				)
			);
			
			//	Number of nanoseconds spent
			//	executing queries
			kvps.EmplaceBack(
				executing_label,
				String::Format(
					ns_template,
					executing
				)
			);
			
			//	Number of nanoseconds spent
			//	waiting for the lock
			kvps.EmplaceBack(
				waiting_label,
				String::Format(
					ns_template,
					waiting
				)
			);
			
			//	Average number of nanoseconds
			//	spent connecting per connect
			//	or reconnect
			kvps.EmplaceBack(
				avg_connecting_label,
				String::Format(
					ns_template,
					avg(
						connecting,
						//	We add one to account for
						//	the initial connect
						reconnects+1
					)
				)
			);
			
			//	Average number of nanoseconds
			//	spent per query
			kvps.EmplaceBack(
				avg_executing_label,
				String::Format(
					ns_template,
					avg(
						executing,
						requests
					)
				)
			);
			
			//	Average number of nanoseconds
			//	spent per lock
			kvps.EmplaceBack(
				avg_waiting_label,
				String::Format(
					ns_template,
					avg(
						waiting,
						waited
					)
				)
			);
			
			//	About client
			kvps.EmplaceBack(
				client_version,
				String::Format(
					client_version_template,
					mysql_get_client_info(),
					(mysql_thread_safe()==0) ? no : yes
				)
			);
			
			//	About server
			kvps.EmplaceBack(
				server_version,
				String::Format(
					server_version_template,
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
				protocol_label,
				mysql_get_proto_info(&conn)
			);
			
			//	Thread ID
			kvps.EmplaceBack(
				thread_label,
				mysql_thread_id(&conn)
			);
		
		});
		
		return Tuple<String,Vector<Tuple<String,String>>>(
			name,
			std::move(kvps)
		);
	
	}


}

#include <mysql_data_provider/mysql_data_provider.hpp>
#include <cstdlib>
#include <memory>


namespace MCPP {


	namespace MySQL {
	
	
		DataProvider::LogEntry::LogEntry (String text, Service::LogType type) noexcept : Text(std::move(text)), Type(type) {	}
		
		
		template <typename T, typename... Args>
		auto DataProvider::prepare (Connection & conn, T && callback, Args &&... args) -> decltype(
			callback(std::forward<Args>(args)...)
		) {
		
			//	Make sure the connection is connected
			auto timer=Timer::CreateAndStart();
			if (conn.Ready()) {
			
				connecting+=timer.ElapsedNanoseconds();
				++connected;
			
			}
			
			//	Execute
			
			timer=Timer::CreateAndStart();
			auto timer_guard=AtExit([&] () {
			
				executing+=timer.ElapsedNanoseconds();
				++executed;
			
			});
			
			try {
			
				return callback(std::forward<Args>(args)...);
			
			} catch (...) {
			
				timer_guard.Disengage();
				
				throw;
			
			}
		
		}
		
		
		template <typename T, typename... Args>
		auto DataProvider::execute (T && callback, Args &&... args) -> decltype(
			callback(
				std::declval<Connection &>(),
				std::forward<Args>(args)...
			)
		) {
		
			//	Get a connection
			auto guard=pool.Get();
			auto & conn=guard.Get();
			
			return prepare(
				conn,
				std::forward<T>(callback),
				conn,
				std::forward<Args>(args)...
			);
		
		}
		
		
		template <typename... Args>
		void DataProvider::perform (const String & query, Args &&... args) {
		
			auto param=MakeBind(std::forward<Args>(args)...);
			
			execute([&] (Connection & conn) {
			
				auto & stmt=conn.Get(query);
				
				stmt.Parameters(param);
				stmt.Execute();
			
			});
		
		}
		
		
		static const String log_query("INSERT INTO `log` (`text`,`type`) VALUES (?,?)");
		
		
		void DataProvider::write_log (const String & text, Service::LogType type) {
		
			perform(
				log_query,
				text,
				MCPP::DataProvider::GetLogType(type)
			);
		
		}
	
	
		void DataProvider::worker () noexcept {
		
			try {
			
				for (;;) {
				
					auto entry=lock.Execute([&] () {
					
						while ((log.Count()==0) && !stop) wait.Sleep(lock);
						
						Nullable<LogEntry> entry;
						
						if (log.Count()==0) return entry;
						
						entry.Construct(std::move(log[0]));
						log.Delete(0);
						
						return entry;
					
					});
					
					if (entry.IsNull()) break;
					
					write_log(
						entry->Text,
						entry->Type
					);
				
				}
			
			} catch (...) {
			
				std::abort();
			
			}
		
		}
	
	
		class ConnectionFactory {
		
		
			private:
			
			
				Nullable<String> host;
				Nullable<String> username;
				Nullable<String> password;
				Nullable<String> database;
				Nullable<UInt16> port;
				
				
			public:
			
			
				ConnectionFactory (
					Nullable<String> host,
					Nullable<String> username,
					Nullable<String> password,
					Nullable<String> database,
					Nullable<UInt16> port
				) noexcept
					:	host(std::move(host)),
						username(std::move(username)),
						password(std::move(password)),
						database(std::move(database)),
						port(std::move(port))
				{	}
				
				
				std::unique_ptr<Connection> operator () () const {
				
					return std::unique_ptr<Connection>(new Connection(
						host,
						username,
						password,
						database,
						port
					));
				
				}
		
		};
	
	
		DataProvider::DataProvider (
			Nullable<String> host,
			Nullable<String> username,
			Nullable<String> password,
			Nullable<String> database,
			Nullable<UInt16> port,
			Word max
		)	:	stop(false),
				pool(
					ConnectionFactory(
						std::move(host),
						std::move(username),
						std::move(password),
						std::move(database),
						std::move(port)
					),
					max
				)
		{
		
			connecting=0;
			connected=0;
			executing=0;
			executed=0;
		
			thread=Thread([this] () mutable {	worker();	});
		
		}
		
		
		DataProvider::~DataProvider () noexcept {
		
			lock.Execute([&] () {
			
				stop=true;
				
				wait.WakeAll();
			
			});
			
			thread.Join();
		
		}
		
		
		template <typename... Args>
		void add (DataProviderInfo & info, const String & name, const String & str, Args &&... args) {
		
			info.Data.Add(
				DataProviderDatum{
					name,
					(sizeof...(Args)==0) ? str : String::Format(str,std::forward<Args>(args)...)
				}
			);
		
		}
		
		
		static UInt64 avg (UInt64 time, Word num) {
		
			return (num==0) ? 0 : (time/safe_cast<UInt64>(num));
		
		}
		
		
		static const String name("MySQL Data Provider");
		
		static const String protocol_version_label("Protocol Version");
		
		static const String server_version_label("Server Version");
		static const String server_version_template("MySQL Server Version {0}");
		
		static const String host_label("Host");
		
		static const String client_version_label("Client Version");
		static const String client_version_template("libmysqlclient Version {0}, Threadsafe: {1}");
		static const String yes("Yes");
		static const String no("No");
		
		static const String stat_template("{0}, {1}ns (total), {2}ns (average)");
		static const String connecting_label("Connecting");
		static const String executing_label("Executing");
		
		static const String pool_label("Connection Pool");
		static const String pool_template("{0} (current), {1} (maximum)");
		static const String unlimited("Unlimited");
		
		
		DataProviderInfo DataProvider::GetInfo () {
		
			DataProviderInfo retr;
			retr.Name=name;
			
			//	Get information that requires a connection
			execute([&] (Connection & conn) {
			
				add(
					retr,
					protocol_version_label,
					mysql_get_proto_info(conn)
				);
				
				add(
					retr,
					server_version_label,
					server_version_template,
					mysql_get_server_info(conn)
				);
				
				add(
					retr,
					host_label,
					mysql_get_host_info(conn)
				);
			
			});
			
			add(
				retr,
				client_version_label,
				client_version_template,
				mysql_get_client_info(),
				(mysql_thread_safe()==0) ? no : yes
			);
			
			Word connected=this->connected;
			UInt64 connecting=this->connecting;
			add(
				retr,
				connecting_label,
				stat_template,
				connected,
				connecting,
				avg(connecting,connected)
			);
			
			Word executed=this->executed;
			UInt64 executing=this->executing;
			add(
				retr,
				executing_label,
				stat_template,
				executed,
				executing,
				avg(executing,executed)
			);
			
			auto maximum=pool.Maximum();
			add(
				retr,
				pool_label,
				pool_template,
				pool.Count(),
				(maximum==0) ? unlimited : maximum
			);
			
			return retr;
		
		}
		
		
		void DataProvider::WriteLog (const String & text, Service::LogType type) {
		
			lock.Execute([&] () {
			
				log.EmplaceBack(text,type);
				
				wait.Wake();
			
			});
		
		}
		
		
		static const String to_separator(", ");
		static const String chat_log_query("INSERT INTO `chat_log` (`from`,`to`,`message`,`notes`) VALUES (?,?,?,?)");
		
		
		void DataProvider::WriteChatLog (const String & from, const Vector<String> & to, const String & message, const Nullable<String> & notes) {
		
			//	Prepare the "to" string
			Nullable<String> to_str;
			for (auto & str : to) {
			
				if (to_str.IsNull()) to_str.Construct();
				else *to_str << to_separator;
				
				*to_str << str;
			
			}
			
			perform(
				chat_log_query,
				from,
				to_str,
				message,
				notes
			);
		
		}
		
		
		static const String get_binary_query("SELECT `value` FROM `binary` WHERE `key`=?");
		
		
		Nullable<Vector<Byte>> DataProvider::GetBinary (const String & key) {
		
			//	Prepare binds
			auto param=MakeBind(key);
			auto result=MakeBind(Output<Nullable<Vector<Byte>>>{});
			
			//	Execute
			return execute([&] (Connection & conn) {
			
				auto & stmt=conn.Get(get_binary_query);
				
				stmt.Parameters(param);
				stmt.Results(result);
				stmt.Execute();
				
				if (!stmt.Fetch()) return Nullable<Vector<Byte>>{};
				
				auto retr=result.Get(stmt);
				
				stmt.Complete();
				
				return retr;
			
			});
		
		}
		
		
		bool DataProvider::GetBinary (const String & key, void * ptr, Word * len) {
		
			auto param=MakeBind(key);
			Blob blob(ptr,*len);
			auto result=MakeBind(blob);
			
			return execute([&] (Connection & conn) {
			
				auto & stmt=conn.Get(get_binary_query);
				
				stmt.Parameters(param);
				stmt.Results(result);
				stmt.Execute();
				
				if (!stmt.Fetch()) return false;
				
				*len=safe_cast<Word>(blob.Length);
				
				stmt.Complete();
				
				return true;
			
			});
		
		}
		
		
		static const String save_binary_query("REPLACE INTO `binary` (`key`,`value`) VALUES (?,?)");
		
		
		void DataProvider::SaveBinary (const String & key, const void * ptr, Word len) {
		
			Blob blob(ptr,len);
			perform(save_binary_query,key,blob);
		
		}
		
		
		static const String delete_binary_query("DELETE FROM `binary` WHERE `key`=?");
		
		
		void DataProvider::DeleteBinary (const String & key) {
		
			perform(
				delete_binary_query,
				key
			);
		
		}
		
		
		static const String retrieve_setting_query("SELECT `value` FROM `settings` WHERE `setting`=?");
		
		
		Nullable<String> DataProvider::RetrieveSetting (const String & setting) {
		
			//	Prepare binds
			auto param=MakeBind(setting);
			auto result=MakeBind(Output<Nullable<String>>{});
			
			return execute([&] (Connection & conn) {
			
				auto & stmt=conn.Get(retrieve_setting_query);
				
				stmt.Parameters(param);
				stmt.Results(result);
				stmt.Execute();
				
				if (!stmt.Fetch()) return Nullable<String>{};
				
				auto retr=result.Get(stmt);
				
				stmt.Complete();
				
				return retr;
			
			});
		
		}
		
		
		static const String set_setting_query("REPLACE INTO `settings` (`setting`,`value`) VALUES (?,?)");
		
		
		void DataProvider::SetSetting (const String & setting, const Nullable<String> & value) {
		
			//	Re-route this call to delete
			//	if setting a setting to null
			if (value.IsNull()) {
			
				DeleteSetting(setting);
				
				return;
			
			}
			
			perform(
				set_setting_query,
				setting,
				*value
			);
		
		}
		
		
		static const String delete_setting_query("DELETE FROM `settings` WHERE `setting`=?");
		
		
		void DataProvider::DeleteSetting (const String & setting) {
		
			perform(
				delete_setting_query,
				setting
			);
		
		}
		
		
		static const String insert_kvp_query("INSERT INTO `keyvaluestore` (`key`,`value`) VALUES (?,?)");
		
		
		void DataProvider::InsertValue (const String & key, const String & value) {
		
			perform(
				insert_kvp_query,
				key,
				value
			);
		
		}
		
		
		static const String delete_single_kvp_query("DELETE FROM `keyvaluestore` WHERE `key`=? AND `value`=?");
		
		
		void DataProvider::DeleteValues (const String & key, const String & value) {
		
			perform(
				delete_single_kvp_query,
				key,
				value
			);
		
		}
		
		
		static const String delete_multi_kvp_query("DELETE FROM `keyvaluestore` WHERE `key`=?");
		
		
		void DataProvider::DeleteValues (const String & key) {
		
			perform(
				delete_multi_kvp_query,
				key
			);
		
		}
		
		
		static const String get_kvp_query("SELECT `value` FROM `keyvaluestore` WHERE `key`=?");
		
		
		Vector<String> DataProvider::GetValues (const String & key) {
		
			//	Bind
			auto param=MakeBind(key);
			auto result=MakeBind(Output<String>{});
			
			//	Execute
			return execute([&] (Connection & conn) {
			
				auto & stmt=conn.Get(get_kvp_query);
				
				stmt.Parameters(param);
				stmt.Results(result);
				stmt.Execute();
				
				//	Extract results
				Vector<String> retr;
				while (stmt.Fetch()) retr.Add(result.Get(stmt));
				
				return retr;
			
			});
		
		}
	
	
	}


}

/**
 *	\file
 */
 
 
#pragma once


#include <data_provider.hpp>
#include <memory>
#include <utility>
#include <functional>
#include <type_traits>
#include <cstring>
#include <atomic>
#ifdef ENVIRONMENT_WINDOWS
#include <mysql.h>
#include <errmsg.h>
#else
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#endif


namespace MCPP {


	template <typename T>
	class MySQLBindWrapper {
	
	
		private:
		
		
			T & ref;
	
	
		public:
		
		
			MySQLBindWrapper (T & ref) noexcept : ref(ref) {	}
			
			
			void Bind (MYSQL_BIND & bind) noexcept {
			
				switch (sizeof(T)) {
				
					case 1:
					
						bind.buffer_type=MYSQL_TYPE_TINY;
						break;
						
					case 2:
					
						bind.buffer_type=MYSQL_TYPE_SHORT;
						break;
						
					case 4:
					
						bind.buffer_type=MYSQL_TYPE_LONG;
						break;
						
					case 8:
					default:
					
						bind.buffer_type=MYSQL_TYPE_LONGLONG;
						break;
				
				}
				
				bind.buffer=&ref;
				bind.buffer_length=sizeof(T);
				bind.is_unsigned=std::is_unsigned<T>::value;
			
			}
	
	
	};
	
	
	template <>
	class MySQLBindWrapper<String> {
	
	
		private:
		
		
			Vector<Byte> buffer;
			
			
		public:
		
		
			MySQLBindWrapper (const String & str) : buffer(UTF8().Encode(str)) {	}
			
			
			void Bind (MYSQL_BIND & bind) noexcept {
			
				bind.buffer_type=MYSQL_TYPE_STRING;
				bind.buffer=static_cast<Byte *>(buffer);
				bind.buffer_length=buffer.Count();
			
			}
	
	
	};
	
	
	template <>
	class MySQLBindWrapper<Nullable<String>> {
	
	
		private:
		
		
			Vector<Byte> buffer;
			my_bool is_null;
			
			
		public:
		
		
			MySQLBindWrapper (const Nullable<String> & str) {
			
				if (str.IsNull()) {
				
					is_null=true;
				
				} else {
				
					is_null=false;
					buffer=UTF8().Encode(*str);
				
				}
			
			}
			
			
			void Bind (MYSQL_BIND & bind) noexcept {
			
				bind.buffer_type=MYSQL_TYPE_STRING;
				
				if (is_null) {
				
					bind.is_null=&is_null;
				
				} else {
				
					bind.buffer=static_cast<Byte *>(buffer);
					bind.buffer_length=buffer.Count();
				
				}
			
			}
	
	
	};
	
	
	template <typename... Args>
	class MySQLBind {
	
	
		private:
		
		
			MYSQL_BIND binds [sizeof...(Args)];
			Tuple<
				MySQLBindWrapper<Args>...
			> wrappers;
			
			
			template <Word i>
			typename std::enable_if<
				i<sizeof...(Args)
			>::type helper () noexcept {
			
				wrappers.template Item<i>().Bind(binds[i]);
				
				helper<i+1>();
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i>=sizeof...(Args)
			>::type helper () const noexcept {	}
			
			
		public:
		
		
			template <typename... Args2>
			MySQLBind (Args2 &&... args) noexcept(
				std::is_nothrow_constructible<
					Tuple<MySQLBindWrapper<Args>...>,
					Args2 &&...
				>::value
			) : wrappers(std::forward<Args2>(args)...) {
			
				memset(
					binds,
					0,
					sizeof(binds)
				);
				
				helper<0>();
			
			}
			
			
			operator MYSQL_BIND * () noexcept {
			
				return binds;
			
			}
	
	
	};


	class MySQLConnection {
	
	
		private:
		
		
			//	Connection
			mutable MYSQL conn;
			
			
			//	Prepared statements
			MYSQL_STMT * get_bin_stmt;
			MYSQL_STMT * update_bin_stmt;
			MYSQL_STMT * insert_bin_stmt;
			MYSQL_STMT * delete_bin_stmt;
			
			MYSQL_STMT * log_stmt;
			MYSQL_STMT * chat_stmt;
			
			MYSQL_STMT * get_setting_stmt;
			MYSQL_STMT * update_setting_stmt;
			MYSQL_STMT * insert_setting_stmt;
			MYSQL_STMT * delete_setting_stmt;
			
			MYSQL_STMT * get_kvp_stmt;
			MYSQL_STMT * insert_kvp_stmt;
			MYSQL_STMT * delete_key_stmt;
			MYSQL_STMT * delete_kvp_stmt;
			
			
			//	Private methods
			inline void destroy_stmts () noexcept;
			inline void connect (
				const Nullable<String> &,
				Nullable<UInt16>,
				const Nullable<String> &,
				const Nullable<String> &,
				const Nullable<String> &,
				bool
			);
			void prepare_stmt (MYSQL_STMT * &, const char *);
			[[noreturn]]
			static void raise (MYSQL_STMT *);
	
	
		public:
		
		
			MySQLConnection () = delete;
			MySQLConnection (const MySQLConnection &) = delete;
			MySQLConnection (MySQLConnection &&) = delete;
			MySQLConnection & operator = (const MySQLConnection &) = delete;
			MySQLConnection & operator = (MySQLConnection &&) = delete;
			MySQLConnection (
				const Nullable<String> &,
				Nullable<UInt16>,
				const Nullable<String> &,
				const Nullable<String> &,
				const Nullable<String> &
			);
			~MySQLConnection () noexcept;
			
			
			//	Returns true if a reconnection occurred,
			//	false otherwise.
			bool KeepAlive (
				const Nullable<String> &,
				Nullable<UInt16>,
				const Nullable<String> &,
				const Nullable<String> &,
				const Nullable<String> &
			);
			
			
			Vector<Tuple<String,String>> GetInfo ();
			
			
			void WriteLog (const String &, Service::LogType);
			void WriteChatLog (const String &, const Vector<String> &, const String &, const Nullable<String> &);
			
			
			Nullable<Vector<Byte>> GetBinary (const String &);
			bool GetBinary (const String &, void *, Word *);
			void SaveBinary (const String &, const void *, Word);
			void DeleteBinary (const String &);
			
			
			Nullable<String> GetSetting (const String &);
			void SetSetting (const String &, const Nullable<String> &);
			void DeleteSetting (const String &);
			
			
			void InsertValue (const String &, const String &);
			void DeleteValues (const String &, const String &);
			void DeleteValues (const String &);
			Vector<String> GetValues (const String &);
	
	
	};
	
	
	class MySQLDataProvider : public DataProvider {
	
	
		private:
		
		
			std::atomic<Word> executed;
			std::atomic<UInt64> executing;
			std::atomic<Word> connected;
			std::atomic<UInt64> connecting;
			std::atomic<Word> waited;
			std::atomic<UInt64> waiting;
		
		
			void tasks_func () noexcept;
			//	Gets a connection, blocking until
			//	one becomes available if necessary
			std::unique_ptr<MySQLConnection> get ();
			//	Returns a connection to the connection
			//	pool
			void done (std::unique_ptr<MySQLConnection> &);
			//	Instructs the connection pool manager that
			//	the held connection underwent an error,
			//	and was cleaned up
			void done () noexcept;
			//	Enqueues a logging task
			void enqueue (std::function<void (std::unique_ptr<MySQLConnection> &)>);
			//	Checks to see if the connection is
			//	still open, and, if it's not,
			//	attempts to reconnect
			void keep_alive (std::unique_ptr<MySQLConnection> &);
			template <typename T, typename... Args>
			auto execute (T && callback, Args &&... args) -> typename std::enable_if<
				std::is_same<
					decltype(
						callback(
							std::declval<
								std::unique_ptr<MySQLConnection> &
							>(),
							std::forward<Args>(args)...
						)
					),
					void
				>::value
			>::type {
			
				auto conn=get();
				
				Timer timer;
				
				try {
				
					keep_alive(conn);
				
					timer=Timer::CreateAndStart();
					
				} catch (...) {
				
					done(conn);
					
					throw;
				
				}
				
				try {
				
					callback(
						conn,
						std::forward<Args>(args)...
					);
				
				} catch (...) {
				
					done();
					
					throw;
				
				}
				
				try {
				
					executing+=timer.ElapsedNanoseconds();
				
				} catch (...) {
				
					done(conn);
					
					throw;
				
				}
				
				++executed;
				
				done(conn);
			
			}
			template <typename T, typename... Args>
			auto execute (T && callback, Args &&... args) -> typename std::enable_if<
				!std::is_same<
					decltype(
						callback(
							std::declval<
								std::unique_ptr<MySQLConnection> &
							>(),
							std::forward<Args>(args)...
						)
					),
					void
				>::value,
				decltype(
					callback(
						std::declval<
							std::unique_ptr<MySQLConnection> &
						>(),
						std::forward<Args>(args)...
					)
				)
			>::type {
			
				auto conn=get();
				
				Timer timer;
				
				try {
				
					keep_alive(conn);
				
					timer=Timer::CreateAndStart();
					
				} catch (...) {
				
					done(conn);
					
					throw;
				
				}
				
				Nullable<
					decltype(
						callback(
							std::declval<
								std::unique_ptr<MySQLConnection> &
							>(),
							std::forward<Args>(args)...
						)
					)
				> retr;
				
				try {
				
					retr.Construct(
						callback(
							conn,
							std::forward<Args>(args)...
						)
					);
				
				} catch (...) {
				
					done();
					
					throw;
				
				}
				
				try {
				
					executing+=timer.ElapsedNanoseconds();
				
				} catch (...) {
				
					done(conn);
					
					throw;
				
				}
				
				++executed;
				
				done(conn);
				
				return *retr;
			
			}
		
		
			Vector<std::function<void (std::unique_ptr<MySQLConnection> &)>> tasks;
			Mutex tasks_lock;
			CondVar tasks_wait;
			bool tasks_stop;
			Thread tasks_thread;
			
			
			Vector<std::unique_ptr<MySQLConnection>> pool;
			Word pool_curr;
			Word pool_max;
			Mutex lock;
			CondVar wait;
			
			
			Nullable<String> username;
			Nullable<String> password;
			Nullable<String> host;
			Nullable<String> database;
			Nullable<UInt16> port;
			
			
		public:
		
		
			MySQLDataProvider () = delete;
			MySQLDataProvider (const MySQLDataProvider &) = delete;
			MySQLDataProvider (MySQLDataProvider &&) = delete;
			MySQLDataProvider & operator = (const MySQLDataProvider &) = delete;
			MySQLDataProvider & operator = (MySQLDataProvider &&) = delete;
			MySQLDataProvider (
				Nullable<String>,
				Nullable<UInt16>,
				Nullable<String>,
				Nullable<String>,
				Nullable<String>,
				Word
			);
			virtual ~MySQLDataProvider () noexcept;
			
			
			virtual Tuple<String,Vector<Tuple<String,String>>> GetInfo () override;
			
			
			virtual void WriteLog (const String &, Service::LogType) override;
			virtual void WriteChatLog (const String &, const Vector<String> &, const String &, const Nullable<String> &) override;
			
			
			virtual Nullable<Vector<Byte>> GetBinary (const String &) override;
			virtual bool GetBinary (const String &, void *, Word *) override;
			virtual void SaveBinary (const String &, const void *, Word) override;
			virtual void DeleteBinary (const String &) override;
			
			
			virtual Nullable<String> GetSetting (const String &) override;
			virtual void SetSetting (const String &, const Nullable<String> &) override;
			virtual void DeleteSetting (const String &) override;
			
			
			virtual void InsertValue (const String &, const String &) override;
			virtual void DeleteValues (const String &, const String &) override;
			virtual void DeleteValues (const String &) override;
			virtual Vector<String> GetValues (const String &) override;
	
	
	};


}
 
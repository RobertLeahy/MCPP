/**
 *	\file
 */
 
 
#pragma once


#include <data_provider.hpp>
#include <type_traits>
#include <cstring>
#include <utility>
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


	class MySQLDataProvider : public DataProvider {
	
	
		private:
		
		
			//	Connection
			mutable MYSQL conn;
			//	Guards the connection against
			//	unsynchronized access
			mutable Mutex lock;
			
			
			//	Connection information for
			//	reconnects
			Nullable<Vector<Byte>> username;
			Nullable<Vector<Byte>> password;
			Nullable<Vector<Byte>> database;
			Nullable<Vector<Byte>> host;
			Nullable<UInt16> port;
			
			
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
			
			
			//	Statistics
			Word reconnects;
			Word requests;
			Word waited;
			UInt64 waiting;
			UInt64 executing;
			UInt64 connecting;
			
			
			//	Private methods
			
			//	Connects or reconnects to the
			//	database
			void connect (bool);
			//	Cleans up all prepared statements
			inline void destroy_stmts () noexcept;
			//	Checks to see if the connection to
			//	the database is still up, if not
			//	it reconnects it
			void keep_alive ();
			//	Prepares a prepared statement given
			//	some query text
			void prepare_stmt (MYSQL_STMT * &, const char *);
			//	Destroys a prepared statement
			static void destroy_stmt (MYSQL_STMT * &) noexcept;
			//	Throws an error pending on a prepared
			//	statement
			static void raise (MYSQL_STMT *);
			//	Calls mysql_thread_init as necessary
			static void thread_init ();
			//	Acquires a lock on the connection
			template <typename T>
			auto acquire (T && callback) -> typename std::enable_if<
				std::is_same<
					decltype(callback()),
					void
				>::value
			>::type {
			
				thread_init();
			
				Timer timer(Timer::CreateAndStart());
				
				lock.Acquire();
				
				try {
				
					timer.Stop();
					waiting+=timer.ElapsedNanoseconds();
					++waited;
					
					keep_alive();
					
					callback();
				
				} catch (...) {
				
					lock.Release();
					
					throw;
				
				}
				
				lock.Release();
			
			}
			template <typename T>
			auto acquire (T && callback) -> typename std::enable_if<
				!std::is_same<
					decltype(callback()),
					void
				>::value,
				decltype(callback())
			>::type {
			
				thread_init();
			
				Timer timer(Timer::CreateAndStart());
				
				lock.Acquire();
				
				Nullable<decltype(callback())> retr;
				try {
				
					timer.Stop();
					waiting+=timer.ElapsedNanoseconds();
					++waited;
					
					keep_alive();
					
					retr.Construct(callback());
				
				} catch (...) {
				
					lock.Release();
					
					throw;
				
				}
				
				lock.Release();
				
				return *retr;
			
			}
			//	Performs statistics maintenance
			//	necessary around the executing of
			//	a SQL query
			template <typename T>
			auto execute (T && callback) -> typename std::enable_if<
				std::is_same<
					decltype(callback()),
					void
				>::value
			>::type {
			
				Timer timer(Timer::CreateAndStart());
				
				callback();
				
				timer.Stop();
				executing+=timer.ElapsedNanoseconds();
				++requests;
			
			}
			template <typename T>
			auto execute (T && callback) -> typename std::enable_if<
				!std::is_same<
					decltype(callback()),
					void
				>::value,
				decltype(callback())
			>::type {
			
				Timer timer(Timer::CreateAndStart());
				
				auto retr=callback();
				
				timer.Stop();
				executing+=timer.ElapsedNanoseconds();
				++requests;
				
				return retr;
			
			}
		
		
		public:
		
		
			MySQLDataProvider () = delete;
			MySQLDataProvider (
				const Nullable<String> &,
				Nullable<UInt16>,
				const Nullable<String> &,
				const Nullable<String> &,
				const Nullable<String> &
			);
			virtual ~MySQLDataProvider () noexcept;
			
			
			virtual Tuple<String,Vector<Tuple<String,String>>> GetInfo () override;
			
			
			virtual void WriteLog (const String &, Service::LogType) override;
			virtual void WriteChatLog (const String &, const Vector<String> &, const String &, const Nullable<String> &) override;
			
			
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

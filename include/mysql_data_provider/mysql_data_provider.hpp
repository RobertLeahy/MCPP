#pragma once


#include <rleahylib/rleahylib.hpp>
#include <data_provider.hpp>
#include <hash.hpp>
#include <safeint.hpp>
#include <scope_guard.hpp>
#include <pool.hpp>
#include <atomic>
#include <cstring>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#ifdef ENVIRONMENT_WINDOWS
#include <mysql.h>
#include <errmsg.h>
#else

#endif


namespace MCPP {


	namespace MySQL {
		
		
		class Connection;
		
		
		template <typename...>
		class Bind;
		
		
		//	A prepared statement associated with some
		//	connection and only valid for the lifetime
		//	of that connection (does not persist between
		//	reconnects)
		class PreparedStatement {
		
		
			private:
			
			
				MYSQL_STMT * handle;
				
				
				void destroy () noexcept;
		
		
			public:
			
			
				PreparedStatement () = delete;
				PreparedStatement (const PreparedStatement &) = delete;
				PreparedStatement & operator = (const PreparedStatement &) = delete;
				
				
				PreparedStatement (Connection &, const String &);
				PreparedStatement (PreparedStatement &&) noexcept;
				PreparedStatement & operator = (PreparedStatement &&) noexcept;
				~PreparedStatement () noexcept;
				
				
				[[noreturn]]
				void Raise ();
				
				
				template <typename... Args>
				void Parameters (Bind<Args...> & bind) {
				
					if (mysql_stmt_bind_param(handle,bind)!=0) Raise();
				
				}
				
				
				template <typename... Args>
				void Results (Bind<Args...> & bind) {
				
					if (mysql_stmt_bind_result(handle,bind)!=0) Raise();
				
				}
				
				
				void Execute ();
				
				
				bool Fetch ();
				void Fetch (MYSQL_BIND &, Word);
				void Complete ();
		
		
		};
		
		
		//	A single connection to a MySQL database
		class Connection {
		
		
			private:
			
			
				Nullable<MYSQL> handle;
				
				
				std::unordered_map<String,PreparedStatement> statements;
				
				
				Nullable<String> host;
				Nullable<String> username;
				Nullable<String> password;
				Nullable<String> database;
				Nullable<UInt16> port;
				
				
				MYSQL * get () noexcept;
				void destroy () noexcept;
				template <typename T>
				void set (enum mysql_option, const T &);
				void connect ();
				bool check ();
				
				
			public:
			
			
				Connection () = delete;
				Connection (const Connection &) = delete;
				Connection (Connection &&) = delete;
				Connection & operator = (const Connection &) = delete;
				Connection & operator = (Connection &&) = delete;
				
				
				Connection (
					Nullable<String> host,
					Nullable<String> username,
					Nullable<String> password,
					Nullable<String> database,
					Nullable<UInt16> port
				);
				~Connection () noexcept;
				
				
				//	Returns true if a reconnect occurred,
				//	false otherwise
				bool Ready ();
				
				
				[[noreturn]]
				void Raise();
				
				
				operator MYSQL * () noexcept;
				
				
				PreparedStatement & Get (const String &);
		
		
		};
		
		
		class DataProvider : public MCPP::DataProvider {
		
		
			private:
			
			
				class LogEntry {
				
				
					public:
					
					
						String Text;
						Service::LogType Type;
						
						
						LogEntry (String, Service::LogType) noexcept;
				
				
				};
				
				
				//	Logging thread
				Vector<LogEntry> log;
				mutable Mutex lock;
				mutable CondVar wait;
				bool stop;
				Thread thread;
			
			
				//	Pool of connections
				Pool<Connection> pool;
				
				
				//	Statistics
				
				//	Time spent connecting and
				//	number of times connected
				std::atomic<UInt64> connecting;
				std::atomic<Word> connected;
				//	Time spent executing and number
				//	of queries executed
				std::atomic<UInt64> executing;
				std::atomic<Word> executed;
				
				
				template <typename T, typename... Args>
				auto prepare (Connection &, T && callback, Args &&... args) -> decltype(
					callback(std::forward<Args>(args)...)
				);
				template <typename T, typename... Args>
				auto execute (T && callback, Args &&... args) -> decltype(
					callback(
						std::declval<Connection &>(),
						std::forward<Args>(args)...
					)
				);
				template <typename... Args>
				void perform (const String &, Args &&...);
				void write_log (const String &, Service::LogType);
				void worker () noexcept;
				
				
			public:
			
			
				DataProvider (
					Nullable<String> host,
					Nullable<String> username,
					Nullable<String> password,
					Nullable<String> database,
					Nullable<UInt16> port,
					Word max
				);
				~DataProvider () noexcept;
				
				
				virtual DataProviderInfo GetInfo () override;
				virtual void WriteLog (const String &, Service::LogType) override;
				virtual void WriteChatLog (const String &, const Vector<String> &, const String &, const Nullable<String> &) override;
				virtual Nullable<Vector<Byte>> GetBinary (const String &) override;
				virtual bool GetBinary (const String &, void *, Word *) override;
				virtual void SaveBinary (const String &, const void *, Word) override;
				virtual void DeleteBinary (const String &) override;
				virtual Nullable<String> RetrieveSetting (const String &) override;
				virtual void SetSetting (const String &, const Nullable<String> &) override;
				virtual void DeleteSetting (const String &) override;
				virtual void InsertValue (const String &, const String &) override;
				virtual void DeleteValues (const String &, const String &) override;
				virtual void DeleteValues (const String &) override;
				virtual Vector<String> GetValues (const String &) override;
		
		
		};
		
		
		template <typename>
		class Output {	};
		
		
		class Blob {
		
		
			public:
			
			
				const void * Pointer;
				unsigned long Length;
				
				
				Blob (const void *, Word);
		
		
		};
		
		
		Vector<Byte> GetBuffer (MYSQL_BIND &, PreparedStatement &, Word);
		
		
		template <typename T>
		class Binder {
		
		
			public:
			
			
				void Initialize (MYSQL_BIND & bind, T & obj) noexcept {
				
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
					
					bind.buffer=&obj;
					bind.buffer_length=sizeof(T);
					bind.is_unsigned=std::is_unsigned<T>::value;
				
				}
		
		
		};
		
		
		template <>
		class Binder<String> {
		
		
			private:
			
			
				Vector<Byte> buffer;
				
				
			public:
			
			
				void Initialize (MYSQL_BIND & bind, const String & str) {
				
					bind.buffer_type=MYSQL_TYPE_STRING;
					buffer=UTF8().Encode(str);
					bind.buffer=buffer.begin();
					bind.buffer_length=safe_cast<unsigned long>(buffer.Count());
				
				}
		
		
		};
		
		
		template <>
		class Binder<Output<String>> {
		
		
			private:
			
			
				Vector<Byte> buffer;
				std::unique_ptr<unsigned long> ptr;
				
				
			public:
			
			
				void Initialize (MYSQL_BIND & bind, const Output<String> &) {
				
					ptr=std::unique_ptr<unsigned long>(new unsigned long());
					bind.length=ptr.get();
					
					bind.buffer_type=MYSQL_TYPE_STRING;
				
				}
				
				
				template <Word i>
				String Get (MYSQL_BIND & bind, PreparedStatement & stmt) {
				
					auto buffer=GetBuffer(bind,stmt,i);
					return UTF8().Decode(
						buffer.begin(),
						buffer.end()
					);
				
				}
		
		
		};
		
		
		template <>
		class Binder<Output<Vector<Byte>>> {
		
		
			private:
			
			
				std::unique_ptr<unsigned long> ptr;
		
		
			public:
			
			
				void Initialize (MYSQL_BIND & bind, const Output<Vector<Byte>> &) {
				
					ptr=std::unique_ptr<unsigned long>(new unsigned long());
					bind.length=ptr.get();
				
					bind.buffer_type=MYSQL_TYPE_BLOB;
				
				}
				
				
				template <Word i>
				Vector<Byte> Get (MYSQL_BIND & bind, PreparedStatement & stmt) {
				
					return GetBuffer(bind,stmt,i);
				
				}
		
		
		};
		
		
		template <typename T>
		class Binder<Nullable<T>> {
		
		
			private:
			
			
				Binder<T> inner;
				
				
			public:
			
			
				void Initialize (MYSQL_BIND & bind, const Nullable<T> & obj) noexcept(
					noexcept(std::declval<Binder<T> &>().Initialize(bind,*obj))
				) {
				
					if (obj.IsNull()) bind.buffer_type=MYSQL_TYPE_NULL;
					else inner.Initialize(bind,*obj);
				
				}
			
			
				void Initialize (MYSQL_BIND & bind, Nullable<T> & obj) noexcept(
					noexcept(std::declval<Binder<T> &>().Initialize(bind,*obj))
				) {
				
					if (obj.IsNull()) bind.buffer_type=MYSQL_TYPE_NULL;
					else inner.Initialize(bind,*obj);
				
				}
		
		
		};
		
		
		template <typename T>
		class Binder<Output<Nullable<T>>> {
		
		
			private:
			
			
				Binder<Output<T>> inner;
				std::unique_ptr<my_bool> ptr;
				
				
			public:
			
			
				void Initialize (MYSQL_BIND & bind, const Output<Nullable<T>> &) {
				
					ptr=std::unique_ptr<my_bool>(new my_bool());
					bind.is_null=ptr.get();
					
					inner.Initialize(bind,Output<T>{});
				
				}
				
				
				template <Word i>
				Nullable<T> Get (MYSQL_BIND & bind, PreparedStatement & stmt) noexcept(
					noexcept(std::declval<Binder<Output<T>> &>().template Get<i>(bind,stmt))
				) {
				
					Nullable<T> retr;
					
					if (!(*ptr)) retr.Construct(inner.template Get<i>(bind,stmt));
					
					return retr;
				
				}
		
		
		};
		
		
		template <>
		class Binder<Blob> {
		
		
			public:
			
			
				void Initialize (MYSQL_BIND & bind, Blob & blob) noexcept {
				
					bind.buffer_type=MYSQL_TYPE_BLOB;
					bind.buffer=const_cast<void *>(blob.Pointer);
					bind.buffer_length=blob.Length;
					bind.length=&blob.Length;
				
				}
		
		
		};
		
		
		template <typename... Args>
		class Bind {
		
		
			private:
			
			
				typedef Tuple<Binder<Args>...> type;
			
			
				MYSQL_BIND binds [sizeof...(Args)];
				type binders;
				
				
				template <Word>
				static void initialize () noexcept {	}
				
				
				template <Word i, typename T, typename... Next>
				void initialize (T && t, Next &&... args) {
				
					binders.template Item<i>().Initialize(binds[i],std::forward<T>(t));
					
					initialize<i+1>(std::forward<Next>(args)...);
				
				}
				
				
			public:
			
			
				Bind () noexcept {
				
					std::memset(
						binds,
						0,
						sizeof(binds)
					);
				
				}
			
			
				template <typename... BindTypes>
				void Initialize (BindTypes &&... args) {
				
					initialize<0>(std::forward<BindTypes>(args)...);
				
				}
				
				
				template <Word i=0>
				auto Get (PreparedStatement & stmt) -> decltype(std::declval<type &>().template Item<i>().template Get<i>(std::declval<MYSQL_BIND &>(),stmt)) {
					
					return binders.template Item<i>().template Get<i>(binds[i],stmt);
				
				}
				
				
				operator MYSQL_BIND * () noexcept {
				
					return binds;
				
				}
		
		
		};
		
		
		template <typename T>
		using GetBindArg=typename std::decay<T>::type;
		
		
		template <typename... Args>
		Bind<GetBindArg<Args>...> MakeBind (Args &&... args) {
		
			Bind<GetBindArg<Args>...> bind;
			bind.Initialize(std::forward<Args>(args)...);
			
			return bind;
		
		}
	
	
	}
	

}

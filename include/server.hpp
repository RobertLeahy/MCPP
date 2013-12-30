/**
 *	\file
 */


#pragma once


#include <rleahylib/rleahylib.hpp>
#include <chat_provider.hpp>
#include <command_interpreter.hpp>
#include <data_provider.hpp>
#include <event.hpp>
#include <mod_loader.hpp>
#include <network.hpp>
#include <packet_router.hpp>
#include <thread_pool.hpp>
#include <typedefs.hpp>
#include <atomic>
#include <exception>
#include <functional>
#include <type_traits>
#include <unordered_set>
#include <utility>


namespace RLeahyLib {


	inline bool operator == (const Tuple<UInt32,MCPP::ProtocolState,MCPP::ProtocolDirection> & a, const Tuple<UInt32,MCPP::ProtocolState,MCPP::ProtocolDirection> & b) noexcept {
	
		return (
			(a.Item<0>()==b.Item<0>()) &&
			(a.Item<1>()==b.Item<1>()) &&
			(a.Item<2>()==b.Item<2>())
		);
	
	}


}


namespace std {


	template <>
	struct hash<Tuple<UInt32,MCPP::ProtocolState,MCPP::ProtocolDirection>> {
	
	
		private:
		
		
			template <typename T>
			size_t get (T value) const noexcept {
			
				union {
					size_t out;
					typename std::decay<T>::type in;
				};
				
				out=0;
				in=value;
				
				return out;
			
			}
	
	
		public:
		
		
			size_t operator () (const Tuple<UInt32,MCPP::ProtocolState,MCPP::ProtocolDirection> & obj) const noexcept {
			
				size_t retr=23;
				
				retr*=31;
				retr+=get(obj.Item<0>());
				retr*=31;
				retr+=get(obj.Item<1>());
				retr*=31;
				retr+=get(obj.Item<2>());
				
				return retr;
			
			}
	
	
	};


}


namespace MCPP {


	class Server {
	
	
		private:
		
		
			//	State
			
			//	Is the server running?
			bool running;
			//	Must be held to change the server's state
			Mutex state_lock;
			
			
			//	Data
			
			//	Data provider
			DataProvider * data;
		
		
			//	Components
			Nullable<ConnectionHandler> connections;
			Nullable<ThreadPool> pool;
			Nullable<ModuleLoader> mods;
			
			
			//	Handlers for the front-end
			CommandInterpreter * interpreter;
			ChatProvider * provider;
			
			
			//	Logging
			
			//	Whether debug logging shall be
			//	performed at all
			std::atomic<bool> debug;
			std::atomic<bool> log_all_packets;
			mutable RWLock logged_packets_lock;
			std::unordered_set<
				Tuple<
					UInt32,
					ProtocolState,
					ProtocolDirection
				>
			> logged_packets;
			//	Module verbosity
			std::atomic<bool> verbose_all;
			mutable RWLock verbose_lock;
			std::unordered_set<String> verbose;
			
			//	Shutdown synchronization
			Word num_shutdowns;
			Mutex shutdown_lock;
			CondVar shutdown_wait;
			
			
			//					//
			//	PRIVATE METHODS	//
			//					//
			inline void server_startup();
			void bind_to (IPAddress, UInt16);
			void get_default_binds ();
			LocalEndpoint get_endpoint (IPAddress, UInt16);
			void get_bind (const String &);
			void get_binds ();
			inline void load_mods ();
			inline void cleanup_events () noexcept;
			inline void stop_impl ();
			inline void panic_impl (std::exception_ptr except=std::exception_ptr()) noexcept;
		
		
		public:
		
		
			/**
			 *	Retrieves a reference to a valid
			 *	instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance
			 *		of this class.
			 */
			static Server & Get () noexcept;
			/**
			 *	Destroys the valid instance of this
			 *	class, causing Get to create a new
			 *	instance the next time it is invoked.
			 *
			 *	Not thread safe.
			 */
			static void Destroy () noexcept;
			/**
			 *	The date and time the server was built
			 *	in this format: \"MMM DD YYYY HH:MM:SS\".
			 */
			static const String BuildDate;
			/**
			 *	The compiler the server was built with.
			 */
			static const String CompiledWith;
			/**
			 *	The major version number associated with
			 *	this version of Minecraft++.
			 */
			static const Word MajorVersion;
			/**
			 *	The minor version number associated with
			 *	this version of Minecraft++.
			 */
			static const Word MinorVersion;
			/**
			 *	The patch number associated with this
			 *	version of Minecraft++.
			 */
			static const Word Patch;
			/**
			 *	The proper name which identifies
			 *	Minecraft++.
			 */
			static const String Name;
			
			
			/**
			 *	Wraps a callback, panicking if it throws.
			 *
			 *	\tparam T
			 *		The type of callback to invoke.
			 *	\tparam Args
			 *		The types of the arguments to pass
			 *		through to the callback.
			 *
			 *	\param [in] callback
			 *		The callback to invoke.
			 *	\param [in] args
			 *		The arguments to forward through to
			 *		\em callback.
			 */
			template <typename T, typename... Args>
			static auto PanicOnThrow (T && callback, Args &&... args) noexcept(
				noexcept(callback(std::forward<Args>(args)...))
			) -> typename std::enable_if<
				std::is_same<
					decltype(callback(std::forward<Args>(args)...)),
					void
				>::value
			>::type {
			
				try {
				
					callback(std::forward<Args>(args)...);
				
				} catch (...) {
				
					try {
					
						Server::Get().Panic(
							std::current_exception()
						);
					
					} catch (...) {	}
					
					throw;
				
				}
			
			}
			/**
			 *	Wraps a callback, panicking if it throws.
			 *
			 *	\tparam T
			 *		The type of callback to invoke.
			 *	\tparam Args
			 *		The types of the arguments to pass
			 *		through to the callback.
			 *
			 *	\param [in] callback
			 *		The callback to invoke.
			 *	\param [in] args
			 *		The arguments to forward through to
			 *		\em callback.
			 *
			 *	\return
			 *		Whatever \em callback returned.
			 */
			template <typename T, typename... Args>
			static auto PanicOnThrow (T && callback, Args &&... args) noexcept(
				noexcept(callback(std::forward<Args>(args)...))
			) -> typename std::enable_if<
				!std::is_same<
					decltype(callback(std::forward<Args>(args)...)),
					void
				>::value,
				decltype(callback(std::forward<Args>(args)...))
			>::type {
			
				try {
				
					return callback(std::forward<Args>(args)...);
				
				} catch (...) {
				
					try {
					
						Server::Get().Panic(
							std::current_exception()
						);
					
					} catch (...) {	}
					
					throw;
				
				}
			
			}
		
			
			/**
			 *	The maxiumum number of bytes which
			 *	the server shall allow to be buffered
			 *	on a client connection before
			 *	disconnecting them.
			 */
			Word MaximumBytes;
			/**
			 *	The maximum number of players which may
			 *	simultaneously be connected to this
			 *	server.
			 */
			Word MaximumPlayers;
			/**
			 *	Retrieves the server's message of the
			 *	day.
			 *
			 *	\return
			 *		The server's message of the day.
			 */
			String GetMessageOfTheDay ();
			/**
			 *	Gets the server's data provider.
			 *
			 *	\return
			 *		A reference to the server's
			 *		data provider
			 */
			DataProvider & Data ();
			/**
			 *	Gets the server's thread pool.
			 *
			 *	\return
			 *		A reference to the server's
			 *		thread pool.
			 */
			ThreadPool & Pool ();
			/**
			 *	Gets the server's module loader.
			 *
			 *	\return
			 *		A reference to the server's
			 *		module loader.
			 */
			ModuleLoader & Loader ();
			/**
			 *	Gets the server's connection handler.
			 *
			 *	\return
			 *		A reference to the server's connection
			 *		handler.
			 */
			ConnectionHandler & Handler ();
			/**
			 *	A list of clients connected to the
			 *	server.
			 */
			ClientList Clients;
			/**
			 *	Collection of routes to handle incoming
			 *	packets.
			 */
			PacketRouter Router;
			
		
			Server (const Server &) = delete;
			Server (Server &&) = delete;
			Server & operator = (const Server &) = delete;
			Server & operator = (Server &&) = delete;
			
			
			/**
			 *	Starts the server if it is stopped.
			 */
			void Start ();
			/**
			 *	Stops the server if it is running.
			 */
			void Stop () noexcept;
			/**
			 *	Requests a restart from the front-end.
			 *
			 *	If the front-end does not have a restart
			 *	handler, kills the program.
			 */
			void Restart () noexcept;
		
		
			/**
			 *	Creates a new instance of the MCPP
			 *	server.
			 *
			 *	Fails if there is already an
			 *	instance.
			 */
			Server ();
			/**
			 *	Shuts the server down and cleans
			 *	it up.
			 */
			~Server () noexcept;
			
			
			/**
			 *	Writes to the log specific to this
			 *	instance, or the platform default
			 *	if that does not exist or has not
			 *	been loaded.
			 *
			 *	\param [in] message
			 *		The message to log.
			 *	\param [in] type
			 *		The type of message to log.
			 */
			void WriteLog (const String & message, Service::LogType type) noexcept;
			/**
			 *	Writes to the server's chat log.
			 *
			 *	\param [in] from
			 *		The sender of the message to
			 *		log.
			 *	\param [in] to
			 *		A list of recipients to whom
			 *		the message was sent.  If
			 *		empty a broadcast is assumed.
			 *	\param [in] message
			 *		The message-in-question.
			 *	\param [in] notes
			 *		Any notes associated with the
			 *		message, null if there are no
			 *		such notes.
			 */
			void WriteChatLog (const String & from, const Vector<String> & to, const String & message, const Nullable<String> & notes) noexcept;
			
			
			/**
			 *	Should be called when an irrecoverable
			 *	error is experienced.
			 */
			void Panic () noexcept;
			/**
			 *	Should be called when an irrecoverable
			 *	error is experienced.
			 *
			 *	\param [in] reason
			 *		A reason string which describes
			 *		the irrecoverable error.
			 */
			void Panic (const String & reason) noexcept;
			/**
			 *	Should be called when an irrecoverable
			 *	error is experienced.
			 *
			 *	\param [in] except
			 *		A std::exception_ptr object which
			 *		points to the exception representing
			 *		the irrecoverable error.
			 */
			void Panic (std::exception_ptr except) noexcept;
			
			
			/**
			 *	Enables or disables debug logging.
			 *
			 *	\param [in] debug
			 *		\em true to enable debug logging,
			 *		\em false to disable debug
			 *		logging.
			 */
			void SetDebug (bool debug) noexcept;
			/**
			 *	Enables or disables the debug
			 *	logging of all packets.
			 *
			 *	\param [in] log_all_packets
			 *		\em true if all packets should
			 *		be logged, \em false otherwise.
			 */
			void SetDebugAllPackets (bool log_all_packets) noexcept;
			/**
			 *	Enables the debug logging of a
			 *	certain packet.
			 *
			 *	\param [in] packet_id
			 *		The ID of the packet to log.
			 *	\param [in] state
			 *		The protocol state associated
			 *		with the packet.
			 *	\param [in] direction
			 *		The direction associated with
			 *		the packet.
			 */
			void SetDebugPacket (UInt32 packet_id, ProtocolState state, ProtocolDirection direction);
			/**
			 *	Disables the logging of a certain
			 *	packet.
			 *
			 *	\param [in] packet_id
			 *		The ID of the packet to log.
			 *	\param [in] state
			 *		The protocol state associated
			 *		with the packet.
			 *	\param [in] direction
			 *		The direction associated with
			 *		the packet.
			 */
			void UnsetDebugPacket (UInt32 packet_id, ProtocolState state, ProtocolDirection direction) noexcept;
			/**
			 *	Enables or disables debug logging
			 *	of all keys.
			 *
			 *	\param [in] verbose_all
			 *		\em true if all keys should
			 *		be logged, \em false otherwise.
			 */
			void SetVerboseAll (bool verbose_all) noexcept;
			/**
			 *	Enables the debug logging of a 
			 *	certain key.
			 *
			 *	\param [in] verbose
			 *		The key which shall be debug
			 *		logged.
			 */
			void SetVerbose (String verbose);
			/**
			 *	Disables the debug logging of a
			 *	certain key.
			 *
			 *	\param [in] verbose
			 *		The key which shall no longer
			 *		be debug logged.
			 */
			void UnsetVerbose (const String & verbose);
			/**
			 *	Whether the packet should be logged.
			 *
			 *	\param [in] type
			 *		The type of the packet.
			 *	\param [in] state
			 *		The protocol state associated
			 *		with the packet.
			 *	\param [in] direction
			 *		The direction the packet is moving.
			 *
			 *	\return
			 *		\em true if the packet should be
			 *		logged, \em false otherwise.
			 */
			bool LogPacket (UInt32 type, ProtocolState state, ProtocolDirection direction) const noexcept;
			/**
			 *	Whether the module or component given
			 *	should be verbose.
			 *
			 *	\param [in] key
			 *		The key of the module or component
			 *		in question.
			 *
			 *	\return
			 *		\em true if the module or component
			 *		should be verbose, \em false
			 *		otherwise.
			 */
			bool IsVerbose (const String & key) const;
			
			
			/**
			 *	Sets the server's command interpreter.
			 *
			 *	\param [in] interpreter
			 *		A pointer to the new command interpreter
			 *		to use.
			 */
			void SetCommandInterpreter (CommandInterpreter * interpreter) noexcept;
			/**
			 *	Gets the server's command interpreter, or
			 *	throws if the server has no command
			 *	interpreter.
			 *
			 *	\return
			 *		A reference to a command interpreter.
			 */
			CommandInterpreter & GetCommandInterpreter () const;
			/**
			 *	Sets the server's chat provider.
			 *
			 *	\param [in] provider
			 *		A pointer to the new chat provider to
			 *		use.
			 */
			void SetChatProvider (ChatProvider * provider) noexcept;
			/**
			 *	Gets the server's chat provider, or throws
			 *	if the server has no chat provider.
			 *
			 *	\return
			 *		A reference to a chat provider.
			 */
			ChatProvider & GetChatProvider () const;
			/**
			 *	Retrieves a string which identifies the
			 *	server software.
			 *
			 *	\return
			 *		A string identifying the server
			 *		software.
			 */
			String GetName ();
			
			
			
			//			//
			//	EVENTS	//
			//			//
			
			
			/**
			 *	Invoked whenever a new connection
			 *	is formed, passed the IP address and
			 *	port from which the remote client is
			 *	connecting.  A return value of \em true
			 *	specifies that the connection should
			 *	be accepted, whereas \em false will
			 *	cause the server to immediately close
			 *	the connection.
			 */
			Event<bool (IPAddress, UInt16, IPAddress, UInt16)> OnAccept;
			/**
			 *	Invoked whenever a connection is accepted
			 *	into the server, passed the Connection
			 *	object which represents the connection
			 *	and which can be used to send data over
			 *	it.
			 */
			Event<void (SmartPointer<Client>)> OnConnect;
			/**
			 *	Invoked whenever a connection is ended,
			 *	passed the Connection object which
			 *	represents the connection which is about
			 *	to be terminated as well as the reason
			 *	the connection was ended.
			 */
			Event<void (SmartPointer<Client>, const String &)> OnDisconnect;
			/**
			 *	Invoked whenever the server's log is
			 *	written.
			 */
			Event<void (const String &, Service::LogType)> OnLog;
			/**
			 *	Invoked whenever the server's chat log
			 *	is written.
			 */
			Event<void (const String &, const Vector<String> &, const String &, const Nullable<String> &)> OnChatLog;
			/**
			 *	Invoked before and after the server installs
			 *	modules.
			 *
			 *	Useful if the front-end wishes to attach to
			 *	some functionality in the server, basically
			 *	useless otherwise.
			 *
			 *	Passed a boolean which is \em true if the
			 *	callback is being invoked before modules are
			 *	installed, \em false if the callback is being
			 *	invoked after modules are installed.
			 */
			Event<void (bool)> OnInstall;
			/**
			 *	Invoked when any client is logged in.
			 *
			 *	This callback is guaranteed to have ended
			 *	before any further packets from that
			 *	client are dispatched to handlers.
			 */
			Event<void (SmartPointer<Client>)> OnLogin;
			/**
			 *	Invoked before the server shuts down.
			 */
			Event<void ()> OnShutdown;
			/**
			 *	Invoked whenever data is received
			 *	on a connection.  Passed the connection
			 *	on which the data was received, and
			 *	all data currently in that connection's
			 *	buffer.
			 */
			ReceiveType OnReceive;
			/**
			 *	Invoked when the server panics.
			 */
			std::function<void (std::exception_ptr)> OnPanic;
			/**
			 *	Invoked to request a restart.
			 */
			std::function<void ()> OnRestart;
	
	
	};


}

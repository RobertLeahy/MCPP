/**
 *	\file
 */


#pragma once


namespace MCPP {


	/**
	 *	\cond
	 */
	 
	 
	class Server;
	
	
	/**
	 *	\endcond
	 */


}


#include <rleahylib/rleahylib.hpp>
#include <functional>
#include <utility>
#include <thread_pool.hpp>
#include <listen_handler.hpp>
#include <connection_manager.hpp>
#include <data_provider.hpp>
#include <event.hpp>
#include <typedefs.hpp>
#include <http_handler.hpp>


namespace MCPP {


	class Server : public Service {
	
	
		private:
		
		
			//	State
			
			//	Is the server running?
			volatile bool running;
			//	Must be held to change the server's state
			Mutex state_lock;
			
			
			//	Data
			
			//	Data provider
			DataProvider * data;
			
			
			//	Settings
			
			//	Maximum number of bytes that
			//	will be allowed to be buffered
			//	from a certain client before
			//	disconnecting them
			Word max_bytes;
		
		
			//	Components
			Nullable<ConnectionManager> connections;
			Nullable<ThreadPool> pool;
			Nullable<ListenHandler> socket_server;
			
			
			//	For interactive
			
			Mutex console_lock;
			bool is_interactive;
			
			
			//					//
			//	PRIVATE METHODS	//
			//					//
			inline void server_startup();
			
			
		protected:
		
		
			virtual void OnStart (const Vector<const String> & args) override;
			virtual void OnStop () override;
		
		
		public:
		
		
			/**
			 *	Determines whether the server is running
			 *	in interactive mode or not.
			 *
			 *	\return
			 *		\em true if the server is running
			 *		in interactive mode, \em false
			 *		otherwise.
			 */
			bool IsInteractive () const noexcept;
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
			 *	Starts the server in interactive mode.
			 *
			 *	\param [in] args
			 *		The command line arguments to pass
			 *		to the server.
			 */
			void StartInteractive (const Vector<const String> & args);
		
		
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
			void WriteLog (const String & message, Service::LogType type);
			
			
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
			Event<bool (IPAddress, UInt16)> OnAccept;
			/**
			 *	Invoked whenever a connection is accepted
			 *	into the server, passed the Connection
			 *	object which represents the connection
			 *	and which can be used to send data over
			 *	it.
			 */
			Event<void (SmartPointer<Connection>)> OnConnect;
			/**
			 *	Invoked whenever a connection is ended,
			 *	passed the Connection object which
			 *	represents the connection which is about
			 *	to be terminated as well as the reason
			 *	the connection was ended.
			 */
			Event<void (SmartPointer<Connection>, const String &)> OnDisconnect;
			/**
			 *	Invoked whenever the server's log is
			 *	written.
			 */
			Event<void (const String &, Service::LogType)> OnLog;
			/**
			 *	Invoked whenever data is received
			 *	on a connection.  Passed the connection
			 *	on which the data was received, and
			 *	all data currently in that connection's
			 *	buffer.
			 */
			ReceiveCallback OnReceive;
	
	
	};
	
	
	/**
	 *	The currently running/extant instance of the
	 *	MCPP server, or \em null if there is no
	 *	running/extant instance.
	 */
	extern Nullable<Server> RunningServer;


}

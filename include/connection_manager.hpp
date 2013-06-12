/**
 *	\file
 */
 
 
#pragma once


#include <functional>
#include <stdexcept>
#include <rleahylib/rleahylib.hpp>
#include <atomic>


namespace MCPP {


	/**
	 *	\cond
	 */


	class SendHandle;
	class Connection;
	class ConnectionHandler;
	class ConnectionManager;
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	The type of function invoked when
	 *	a connection is made.
	 */
	typedef std::function<void (SmartPointer<Connection>)> ConnectCallback;
	/**
	 *	The type of function invoked when
	 *	a connection is terminated.
	 */
	typedef std::function<void (SmartPointer<Connection>, const String &)> DisconnectCallback;
	/**
	 *	The type of function invoked when
	 *	data is received.
	 */
	typedef std::function<void (SmartPointer<Connection>, Vector<Byte> &)> ReceiveCallback;
	
	
	


}


#include <typedefs.hpp>
#include <thread_pool.hpp>


namespace MCPP {


	/**
	 *	The states that a SendHandle
	 *	may be in.
	 */
	enum class SendState {
	
		Pending,	/**<	The data is pending in the send queue.	*/
		Sending,	/**<	The data is being sent.	*/
		Sent,		/**<	The data has been sent completely.	*/
		Failed		/**<	The data has not been sent and never will be.	*/
	
	};
	
	
	/**
	 *	The type of callback that may be
	 *	invoked when a send operation
	 *	completes.
	 */
	typedef std::function<void (SendState)> SendCallback;


	/**
	 *	A handle which represents an asynchronous
	 *	send event.
	 */
	class SendHandle {
	
	
		friend class Connection;
		friend class ConnectionHandler;
		
		
		private:
		
		
			Mutex lock;
			CondVar wait;
			SendState state;
			std::atomic<Word> sent;
			Vector<SendCallback> callbacks;
			
			
		public:
			
			
			SendHandle ();
		
		
			SendHandle (const SendHandle &) = delete;
			SendHandle (SendHandle &&) = delete;
			SendHandle & operator = (const SendHandle &) = delete;
			SendHandle & operator = (SendHandle &&) = delete;
			
			
			SendState State () noexcept;
			SendState Wait () noexcept;
			Word Sent () noexcept;
			void AddCallback (SendCallback callback);
	
	
	};


	/**
	 *	Represents a connection.
	 */
	class Connection {
	
	
		friend class ConnectionHandler;
	
	
		private:
		
		
			//	Connection
			Socket socket;
			IPAddress ip;
			UInt16 port;
			
			//	Receive
			Vector<Byte> recv;
			Vector<Byte> recv_alt;
			bool recv_task;
			Mutex recv_lock;
			CondVar recv_wait;
			
			//	Send
			Vector<
				Tuple<
					Vector<Byte>,
					SmartPointer<SendHandle>
				>
			> send_queue;
			//Vector<Vector<Byte>> send_queue;
			Mutex send_lock;
			Mutex * send_msg_lock;
			CondVar * send_msg_signal;
			
			//	Connected to a handler?
			Mutex connected_lock;
			
			//	Last call to socket did not
			//	completely send data
			bool saturated;
			
			//	Disconnect handling
			Mutex reason_lock;
			String reason;
			volatile bool disconnect_flag;
			
			
			void connect (Mutex * lock, CondVar * signal) noexcept;
			void disconnect () noexcept;
			
			
		public:
		
		
			Connection () = delete;
			Connection (const Connection &) = delete;
			Connection (Connection &&) = delete;
			Connection & operator = (const Connection &) = delete;
			Connection & operator = (Connection &&) = delete;
		
		
			/**
			 *	Creates a new connection object.
			 *
			 *	\param [in] socket
			 *		The connected socket.
			 *	\param [in] ip
			 *		The remote IP to which
			 *		\em socket is connected.
			 *	\param [in] port
			 *		The remote port to which
			 *		\em socket is connected.
			 *	\param [in] connect
			 *		A callback to be invoked once
			 *		this object is considered
			 *		connected and ready to be used.
			 *	\param [in] disconnect
			 *		A callback to be invoked before this
			 *		object is destroyed.
			 */
			Connection (
				Socket socket,
				const IPAddress & ip,
				UInt16 port
			);
			
			
			/**
			 *	Cleans up a connection.
			 */
			~Connection () noexcept;
			
			
			/**
			 *	Queues data to be sent over
			 *	the connection.
			 *
			 *	\param [in] buffer
			 *		A buffer to be queued
			 *		to be sent.
			 */
			SmartPointer<SendHandle> Send (Vector<Byte> buffer);
			
			
			/**
			 *	Retrieves the IP address to which this
			 *	connection is connected.
			 *
			 *	\return
			 *		The remote IP.
			 */
			const IPAddress & IP () const noexcept;
			/**
			 *	Retrieves the port to which this connection
			 *	is connected.
			 *
			 *	\return
			 *		The remote port.
			 */
			UInt16 Port () const noexcept;
			
			
			/**
			 *	Instructs the connection handler responsible
			 *	for this socket to close the connection.
			 */
			void Disconnect () noexcept;
			/**
			 *	Instructs the connection handler responsible
			 *	for this socket to close the connection.
			 *
			 *	\param [in] reason
			 *		The reason this connection should be
			 *		closed.
			 */
			void Disconnect (const String & reason) noexcept;
			/**
			 *	Instructs the connection handler responsible
			 *	for this socket to close the connection.
			 *
			 *	\param [in] reason
			 *		The reason this connection should be
			 *		closed.
			 */
			void Disconnect (String && reason) noexcept;
	
	
	};
	
	
	/**
	 *	A pair of threads which handle sends and
	 *	receives on a certain set of sockets.
	 */
	class ConnectionHandler {
	
	
		private:
		
		
			//	Thread control
			volatile bool stop;
			volatile bool normal_shutdown;
			Barrier sync;
			
			//	Connections
			Vector<
				SmartPointer<
					Connection
				>
			> connections;
			RWLock connections_lock;
			
			//	Send
			Nullable<Thread> send;
			Mutex send_lock;
			CondVar send_sleep;
			
			//	Receive
			Nullable<Thread> recv;
			
			//	Parent
			ConnectionManager * parent;
			
			
			//	Worker functions
			static void send_thread (void *);
			void send_thread_impl ();
			static void recv_thread (void *);
			void recv_thread_impl ();
			inline void purge_connections (Vector<SmartPointer<Connection>> &);
		
		
		public:
		
		
			ConnectionHandler () = delete;
			ConnectionHandler (const ConnectionHandler &) = delete;
			ConnectionHandler (ConnectionHandler &&) = delete;
			ConnectionHandler & operator = (const ConnectionHandler &) = delete;
			ConnectionHandler & operator = (ConnectionHandler &&) = delete;
		
		
			/**
			 *	Creates a new connection handler.
			 *
			 *	\param [in] parent
			 *		The ConnectionManager which encloses
			 *		this ConnectionHandler.
			 *	\param [in] pool
			 *		The ThreadPool to use for asynchronous
			 *		tasks.
			 *	\param [in] recv_callback
			 *		The callback to invoke when data is
			 *		received.
			 *	\param [in] log
			 *		The callback for use in logging.
			 *	\param [in panic
			 *		The callback to invoke when a catastrophic
			 *		failure occurs.
			 */
			ConnectionHandler (ConnectionManager & parent);
		
		
			/**
			 *	Cleans up a connection handler.
			 */
			~ConnectionHandler () noexcept;
			
			
			/**
			 *	Retrieves the number of
			 *	connections associated with this
			 *	handler.
			 *
			 *	\return
			 *		Number of associated connections.
			 */
			Word Count () noexcept;
			
			
			/**
			 *	Retrieves the maximum number of
			 *	connections which may be associated
			 *	with this handler.
			 *
			 *	\return
			 *		Maximum number of connections.
			 */
			Word Max () noexcept;
			
			
			/**
			 *	Retrieves the number of available
			 *	connection slots in this handler.
			 *
			 *	\return
			 *		Number of available slots.
			 */
			Word Available () noexcept;
	
	
	};
	
	
	/**
	 *	Handles all connections to the server
	 *	by abstracting read, write, and other
	 *	connection management.
	 */
	class ConnectionManager {
	
	
		friend class ConnectionHandler;
	
	
		private:
		
		
			//	List of handlers
			Vector<SmartPointer<ConnectionHandler>> handlers;
			//	Lock to protect list of handlers
			Mutex handlers_lock;
			
			//	Synchronization for
			//	cleanup
			Mutex cleanup_lock;
			Word cleanup_count;
			CondVar cleanup_wait;
			
			//	Connections waiting to be picked
			//	up by a handler
			Vector<SmartPointer<Connection>> pending;
			//	Lock to be held to grab
			//	pending connections
			Mutex pending_lock;
			
			//	Callbacks
			ConnectCallback connect;
			DisconnectCallback disconnect;
			PanicType panic;
			ReceiveCallback recv;
			LogType log;
			
			//	Thread pool
			ThreadPool * pool;
			
			
			Nullable<SmartPointer<Connection>> Dequeue ();
			
			
		public:
		
		
			ConnectionManager () = delete;
			ConnectionManager (const ConnectionManager &) = delete;
			ConnectionManager (ConnectionManager &&) = delete;
			ConnectionManager & operator = (const ConnectionManager &) = delete;
			ConnectionManager & operator = (ConnectionManager &&) = delete;
		
		
			/**
			 *	Creates a new ConnectionManager.
			 *
			 *	\param [in] connect
			 *		A callback to be invoked whenever
			 *		a new connection is added.
			 *	\param [in] disconnect
			 *		A callback to be invoked whenever
			 *		a connection is closed.
			 *	\param [in] recv
			 *		A callback to be invoked whenever
			 *		data is received.
			 *	\param [in] log
			 *		A callback to be used for logging.
			 *	\param [in] panic
			 *		A callback to be invoked when and if
			 *		a catastrophic failure occurs.
			 *	\param [in] pool
			 *		The ThreadPool to use for asynchronous
			 *		processing.
			 */
			ConnectionManager (
				ConnectCallback connect,
				DisconnectCallback disconnect,
				ReceiveCallback recv,
				LogType log,
				PanicType panic,
				ThreadPool & pool
			);
		
		
			/**
			 *	Cleans up a ConnectionManager, stopping
			 *	all associated threads and closing all
			 *	associated connections.
			 */
			~ConnectionManager () noexcept;
			
			
			/**
			 *	Adds a connection to be managed by this
			 *	connection manager.
			 *
			 *	\param [in] socket
			 *		The socket to use for send and
			 *		receive operations on this connection.
			 *	\param [in] ip
			 *		The remote IP to which \em socket
			 *		is connected.
			 *	\param [in] port
			 *		The remote port to which \em socket
			 *		is connected.
			 */
			void Add (Socket socket, const IPAddress & ip, UInt16 port);
	
	
	};


}

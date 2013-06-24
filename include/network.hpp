/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <thread_pool.hpp>
#include <atomic>
#include <unordered_map>
#include <winsock2.h>


namespace MCPP {


	/**
	 *	\cond
	 */
	
	
	//	Forward declarations
	class Connection;
	class ConnectionHandler;
	
	
	/**
	 *	\endcond
	 */


	//	Callback types
	
	
	/**
	 *	The type of function invoked
	 *	when a connection is terminated.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	1.	The connection that was disconnected.
	 *	2.	The reason it was disconnected.
	 */
	typedef std::function<void (SmartPointer<Connection>, const String &)> DisconnectCallback;
	/**
	 *	The type of function invoked when data
	 *	is received on a connection.
	 *
	 *	This function is guaranteed not to be
	 *	invoked for the same connection until
	 *	this function returns.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	1.	The connection on which the data
	 *		was received.
	 *	2.	All data received on this connection.
	 *		This buffer will not be automatically
	 *		cleared, and will be available with more
	 *		data the next time this function is invoked
	 *		if none is removed.
	 */
	typedef std::function<void (SmartPointer<Connection>, Vector<Byte> &)> ReceiveCallback;
	/**
	 *	The type of function invoked when a connection
	 *	is formed.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	1.	The IP address of the remote end of this
	 *		connection.
	 *	2.	The port of the remote end of this connection.
	 *
	 *	<B>Return:</B>
	 *
	 *	\em true if the connection should be accepted,
	 *	\em false otherwise.
	 */
	typedef std::function<bool (IPAddress, UInt16)> AcceptCallback;
	/**
	 *	The type of function invoked when a connection is formed
	 *	and is ready to process sends and receives.
	 *
	 *	Receive events for this connection are guaranteed not
	 *	to be generated until this function returns.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	1.	The connection.
	 */
	typedef std::function<void (SmartPointer<Connection>)> ConnectCallback;
	/**
	 *	The type of function which may be invoked to write
	 *	to the log.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	1.	The message to log.
	 *	2.	The type of this log entry.
	 */
	typedef std::function<void (const String &, Service::LogType)> LogCallback;
	/**
	 *	The type of function invoked when
	 *	an irrecoverable error occurs.
	 */
	typedef std::function<void ()> PanicCallback;


	/**
	 *	Represents a network endpoint.
	 */
	class Endpoint {
	
	
		private:
		
		
			IPAddress ip;
			UInt16 port;
			
			
		public:
		
		
			Endpoint () = delete;
			/**
			 *	Creates a new endpoint which
			 *	encapsulated a given IP and
			 *	port.
			 *
			 *	\param [in] ip
			 *		The IP to encapsulate.
			 *	\param [in] port
			 *		The port to encapsulate.
			 */
			Endpoint (IPAddress ip, UInt16 port) noexcept;
			
			
			/**
			 *	Retrieves the IP associated with
			 *	this endpoint.
			 *
			 *	\return
			 *		The IP address associated with
			 *		this endpoint.
			 */
			IPAddress IP () const noexcept;
			/**
			 *	Retrieves the port associated with
			 *	this endpoint.
			 *
			 *	\return
			 *		The port associated with this
			 *		endpoint.
			 */
			UInt16 Port () const noexcept;
	
	
	};
	
	
	/**
	 *	The states that a SendHandle
	 *	may be in.
	 */
	enum class SendState {
	
		Pending,	/**<	The data is being sent or waiting to be sent.	*/
		Sent,		/**<	The data has been sent completely.	*/
		Failed		/**<	The data has not been sent and never will be.	*/
	
	};
	
	
	/**
	 *	The type of callback that may be invoked when
	 *	a send operation completes.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	1.	The state the send operation ended in.
	 */
	typedef std::function<void (SendState)> SendCallback;
	
	
	/**
	 *	\cond
	 */
	 
	 
	enum class NetworkCommand {
	
		//	Sent by a thread enqueuing
		//	a send
		BeginSend,
		//	Set by the worker thread when
		//	it begins the send, so it can
		//	differentiate the I/O completion
		//	packet it receives at the end
		//	of the send
		CompleteSend,
		//	Tells the worker that it may begin
		//	a receive operation.
		BeginReceive,
		//	Set in a worker-initiated receive,
		//	tells the worker that this is data
		//	received on the associated socket.
		CompleteReceive,
		//	Socket should be closed, connection
		//	should be destroyed.
		Disconnect
	
	};
	 
	 
	class OverlappedData : public WSAOVERLAPPED {
	
	
		public:
		
		
			OverlappedData () = delete;
			OverlappedData (NetworkCommand command) noexcept;
		
		
			//	The command that this overlapped
			//	structure corresponds to for the
			//	given connection.
			NetworkCommand Command;
	
	
	};
	 
	 
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Allows an asynchronous send to be waited
	 *	on.
	 */
	class SendHandle {
	
	
		friend class Connection;
		friend class ConnectionHandler;
	
	
		private:
		
		
			//	Overlapped structure
			OverlappedData overlapped;
			//	The bytes this send handle
			//	represents
			Vector<Byte> send;
			//	WSABUF structure for I/O
			WSABUF buf;
			//	The result of this send
			//	operation
			SendState state;
			//	Callbacks to be invoked when
			//	this send operation completes
			Vector<SendCallback> callbacks;
			
			
			//	Wait structures
			mutable Mutex lock;
			mutable CondVar wait;
			
			
		public:
		
		
			/**
			 *	\cond
			 */
			
			
			SendHandle (Vector<Byte> send) noexcept;
			
			
			/**
			 *	\endcond
			 */
			 
			 
			SendHandle () = delete;
			SendHandle (const SendHandle &) = delete;
			SendHandle (SendHandle &&) = delete;
			SendHandle & operator = (const SendHandle &) = delete;
			SendHandle & operator = (SendHandle &&) = delete;


			/**
			 *	Retrieves the current state of the
			 *	send operation.
			 *
			 *	\return
			 *		The send operation's state as of
			 *		this function invocation.
			 */
			SendState State () const noexcept;
			/**
			 *	Waits for this send operation to complete.
			 *
			 *	When this function returns the send operation
			 *	is guaranteed to have succeeded or failed.
			 *
			 *	\return
			 *		The status the send operation ended with.
			 */
			SendState Wait () const noexcept;
			/**
			 *	Adds a callback which shall be invoked when
			 *	the send operation completes, and which
			 *	shall be passed the final state of the
			 *	send operation.
			 *
			 *	\param [in] callback
			 *		A callback to enqueue.
			 */
			void AddCallback (SendCallback callback);
	
	
	};

	
	class Connection {
	
	
		friend class ConnectionHandler;
	
	
		private:
		
		
			//	The disconnect request (if any)
			//	that was sent to the connection
			//	handler
			//
			//	Keeps the memory alive.
			SmartPointer<OverlappedData> disconnect_request;
		
		
			//	Shall be invoked only once when
			//	the connection is disconnected
			DisconnectCallback disconnect_callback;
			
			
			//	Synchronization for all connection
			//	methods
			Mutex lock;
			
			
			//	The connection
			SOCKET socket;
			Endpoint endpoint;
			
			
			//	The connection handler handling
			//	this connection
			ConnectionHandler & handler;
			
			
			//	I/O completion port
			HANDLE iocp;
			
			
			//	Sent and received
			std::atomic<UInt64> sent;
			std::atomic<UInt64> received;
			
			
			//	Pending send operations
			std::unordered_map<
				const SendHandle *,
				SmartPointer<SendHandle>
			> sends;
			
			
			//	Receive buffer
			Vector<Byte> recv;
			WSABUF recv_buf;
			OverlappedData overlapped;
			DWORD recv_flags;
			
			
			//	Whether this connection has an
			//	outstanding receive event
			bool pending_recv;
			
			
			//	Whether this connection has been
			//	shutdown
			bool is_shutdown;
			
			
			//	Thread pool to use to dispatch the
			//	disconnect callback
			ThreadPool & pool;
			
			
			//
			//	PRIVATE METHODS
			//
			
			
			//	Drives the disconnect process
			SmartPointer<Connection> disconnect ();
			
			
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			Connection (
				SOCKET socket,
				Endpoint endpoint,
				HANDLE iocp,
				ConnectionHandler & handler
			);
			
			
			/**
			 *	\endcond
			 */
			 
			 
			/**
			 *	Releases all resources held by this
			 *	connection and fails all pending
			 *	send operations.
			 */
			~Connection () noexcept;
		
		
			/**
			 *	Disconnects this connection.
			 */
			void Disconnect ();
			/**
			 *	Disconnects this connection for a specified
			 *	reason.
			 *
			 *	\param [in] reason
			 *		The reason this client is being
			 *		disconnected.
			 */
			void Disconnect (String reason);
			/**
			 *	Sends a buffer of bytes over the connection.
			 *
			 *	\param [in] buffer
			 *		A buffer of bytes which shall be sent.
			 *
			 *	\return
			 *		A handle which represents this send,
			 *		and which may be used to wait for it to
			 *		complete and monitor its status.
			 */
			SmartPointer<SendHandle> Send (Vector<Byte> buffer);
			/**
			 *	Retrieves the remote IP address to which this
			 *	connection is connected.
			 *
			 *	\return
			 *		The remote IP address.
			 */
			IPAddress IP () const noexcept;
			/**
			 *	Retrieves the remote port to which this
			 *	connection is connected.
			 *
			 *	\return
			 *		The remote port number.
			 */
			UInt16 Port () const noexcept;
			/**
			 *	Retrieves the number of bytes sent
			 *	over this connection.
			 *
			 *	\return
			 *		The number of bytes sent.
			 */
			UInt64 Sent () const noexcept;
			/**
			 *	Retrieves the number of bytes received
			 *	on this connection.
			 *
			 *	\return
			 *		The number of bytes received.
			 */
			UInt64 Received () const noexcept;
	
	
	};
	
	
	class ConnectionHandler {
	
	
		friend class Connection;
	
	
		private:
		
		
			//	CALLBACKS
			
			//	Disconnect callback
			DisconnectCallback disconnect_callback;
			//	Receive callback
			ReceiveCallback recv;
			//	Connect callback
			ConnectCallback connect_callback;
			//	Connection filtering callback
			AcceptCallback accept_callback;
			//	Logging callback
			LogCallback log_callback;
			//	Panic callback
			PanicCallback panic_callback;
			
			
			//	Thread pool
			ThreadPool & pool;
		
		
			//	List of connections this
			//	handler is responsible for
			//
			//	As connections are accepted,
			//	they are added.  As connections
			//	error or are disconnected, they
			//	are removed.
			std::unordered_map<
				const Connection *,
				SmartPointer<Connection>
			> connections;
			Mutex connections_lock;
			
			
			//	List of overlapped structures
			//	we've had to create
			std::unordered_map<
				const OverlappedData *,
				SmartPointer<OverlappedData>
			> overlapped;
			
			
			//	Completion port
			HANDLE iocp;
			
			
			//	Used during shutdown to
			//	keep object valid until
			//	receive callbacks end
			std::atomic<Word> running_async;
			Mutex async_lock;
			CondVar async_wait;
			
			
			//	Bound sockets
			Vector<SOCKET> bound;
			
			
			//	Barrier to synchronize
			//	startup
			Barrier barrier;
			
			
			//	Threads
			
			//	Worker thread
			Thread worker;
			//	Accept/listen thread
			Thread accept;
			
			
			//	This variable is used to relay
			//	startup information
			bool handshake;
			
			
			//	Shutdown event for accept
			//	thread
			WSAEVENT stop;
			
			
			//
			//	PRIVATE METHODS
			//
			
			
			inline void kill (Connection *);
			inline void end_async () noexcept;
			void worker_func () noexcept;
			void accept_func () noexcept;
			static int accept_filter (LPWSABUF,LPWSABUF,LPQOS,LPQOS,LPWSABUF,LPWSABUF,GROUP FAR *,DWORD_PTR) noexcept;
			
			
		public:
		
		
			ConnectionHandler () = delete;
			ConnectionHandler (const ConnectionHandler &) = delete;
			ConnectionHandler (ConnectionHandler &&) = delete;
			ConnectionHandler & operator = (const ConnectionHandler &) = delete;
			ConnectionHandler & operator = (ConnectionHandler &&) = delete;
		
		
			/**
			 *	Creates and starts a new ConnectionHandler.
			 *
			 *	\param [in] binds
			 *		The local endpoints that the ConnectionHandler
			 *		should bind to.
			 *	\param [in] accept_callback
			 *		The callback that should be invoked to filter
			 *		incoming connections before they are permitted.
			 *	\param [in] connect_callback
			 *		The callback that should be invoked when a
			 *		connection is permitted.
			 *	\param [in] disconnect_callback
			 *		The callback that should be invoked when a
			 *		connection ends.
			 *	\param [in] recv
			 *		The callback that should be invoked when
			 *		data is received.
			 *	\param [in] log_callback
			 *		The callback that should be invoked when the
			 *		ConnectionHandler has to write to a log.
			 *	\param [in] panic_callback
			 *		The callback that should be invoked when
			 *		something goes wrong from which the
			 *		ConnectionHandler cannot recover.
			 *	\param [in] pool
			 *		A ThreadPool the ConnectionHandler can
			 *		use to dispatch asynchronous events.
			 */
			ConnectionHandler (
				const Vector<Endpoint> & binds,
				AcceptCallback accept_callback,
				ConnectCallback connect_callback,
				DisconnectCallback disconnect_callback,
				ReceiveCallback recv,
				LogCallback log_callback,
				PanicCallback panic_callback,
				ThreadPool & pool
			);
			
			
			/**
			 *	Ends all threads, closes all connections,
			 *	and releases all resources associated
			 *	with this connection handler.
			 */
			~ConnectionHandler () noexcept;
	
	
	};
	

}

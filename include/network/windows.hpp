/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <network.hpp>
#include <thread_pool.hpp>
#include <mswsock.h>
#include <windows.h>
#include <Ws2tcpip.h>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <utility>


namespace MCPP {


	/**
	 *	\cond
	 */


	namespace NetworkImpl {
	
	
		//	Helper function -- raises the last system
		//	error
		[[noreturn]]
		void Raise ();
		//	Helper function -- raises the last WinSock
		//	error
		[[noreturn]]
		void RaiseWSA ();
		//	Helper function -- raises a system error
		//	given an error code
		[[noreturn]]
		void Raise (DWORD);
		//	Helper function -- captures the system error
		//	given as an exception pointer
		std::exception_ptr GetError (DWORD) noexcept;
		//	Helper function -- gets the string associated
		//	with a system error code
		String GetErrorMessage (DWORD);
		//	Helper function -- makes a socket given an
		//	address family (either IPv6 or IPv4)
		SOCKET MakeSocket (bool);
	
	
		class CompletionCommand;
		
		
		//	A packet which may be dequeued from
		//	a completion port and represents a
		//	completed I/O operation.
		class Packet {
		
		
			public:
		
				
				//	True if this packet represents a
				//	successful I/O operation, false
				//	otherwise.
				bool Result;
				//	The operating system error code if
				//	this packet represents failure,
				//	0 otherwise.
				DWORD Error;
				//	The number of bytes associated with
				//	this I/O operation, 0 otherwise.
				DWORD Count;
				//	The command that was issued to start
				//	the I/O operation whose completion
				//	this packet represents.
				CompletionCommand * Command;
				//	The data associated with the handle
				//	I/O was performed on.
				void * Data;
		
		
		};
		
		
		//	Initializes and cleans up WinSock using
		//	RAII techniques
		class Initializer {
		
		
			public:
			
			
				Initializer ();
				~Initializer () noexcept;
		
		
		};
		
		
		//	A completion port from which completed
		//	I/O packets may be extracted
		class CompletionPort {
		
		
			private:
			
			
				HANDLE iocp;
		
		
			public:
			
			
				CompletionPort ();
				~CompletionPort () noexcept;
				
				
				//	Attaches a socket and associated data to
				//	the completion port
				void Attach (SOCKET, void *);
				//	Dequeues an I/O completion packet
				Packet Get ();
				//	Manually posts a message to the completion
				//	port
				void Post ();
		
		
		};
		
		
		class ReferenceManager;
		
		
		//	Provides an RAII guard on reference manager
		//	reference acquisitions
		class ReferenceManagerHandle {
		
		
			friend class ReferenceManager;
		
		
			private:
		
		
				ReferenceManager * manager;
				
				
				ReferenceManagerHandle (ReferenceManager &) noexcept;
				
				
				void destroy () noexcept;
				
				
			public:
			
			
				ReferenceManagerHandle (const ReferenceManagerHandle &) noexcept;
				ReferenceManagerHandle (ReferenceManagerHandle &&) noexcept;
				ReferenceManagerHandle & operator = (const ReferenceManagerHandle &) noexcept;
				ReferenceManagerHandle & operator = (ReferenceManagerHandle &&) noexcept;
				~ReferenceManagerHandle () noexcept;
		
		
		};
		
		
		//	Manages references by providing a handle on
		//	which objects may wait for completion
		class ReferenceManager {
		
		
			private:
			
			
				mutable Mutex lock;
				mutable CondVar wait;
				Word count;
		
		
			public:
			
			
				ReferenceManager () noexcept;
			
			
				~ReferenceManager () noexcept;
			
			
				//	Manually acquires a reference
				void Begin () noexcept;
				//	Manually release a reference
				//	Returns true if the last reference
				//	was manually released
				void End () noexcept;
				//	Acquires a handle which guards an acquired
				//	reference, release it as soon as it goes
				//	out of scope
				ReferenceManagerHandle Get () noexcept;
				//	Waits for the reference count to reach
				//	zero
				void Wait () const noexcept;
				//	Resets the count to zero
				void Reset () noexcept;
				//	Retrieves the current count
				Word Count () const noexcept;
		
		
		};
		
		
		//	The types of commands that may be
		//	issued on a completion port
		enum class CommandType {
		
			Send,
			Receive,
			Accept,
			Connect
		
		};
		
		
		//	All commands inherit this class to
		//	store command-specific data
		//
		//	A pointer to a Command object is
		//	guaranteed to be a pointer to an
		//	OVERLAPPED object as well
		class CompletionCommand {
		
		
			private:
			
			
				//	This structure must start with an
				//	OVERLAPPED structure
				OVERLAPPED overlapped;
		
		
			public:
			
			
				CompletionCommand (CommandType) noexcept;
			
			
				//	The type of command this represents
				CommandType Type;
		
		
		};
		
		
		//	A command to receive data on a given
		//	connection
		class ReceiveCommand : public CompletionCommand {
		
		
			private:
			
			
				WSABUF buf;
				DWORD flags;
		
		
			public:
			
			
				ReceiveCommand () noexcept;
			
			
				//	Received bytes
				Vector<Byte> Buffer;
				
				
				//	Makes a WSARecv request, queuing
				//	up an asynchronous receive using
				//	this object and the buffer within
				//
				//	Returns zero on success, an OS
				//	error code otherwise.
				DWORD Dispatch (SOCKET);
		
		
		};
		
		
		//	A command to send data on a given connection
		class SendCommand : public CompletionCommand {
		
		
			private:
			
			
				WSABUF buf;
				
				
			public:
			
			
				SendCommand (Vector<Byte>) noexcept;
			
			
				Vector<Byte> Buffer;
				
				
				//	Makes a WSASend request, queuing
				//	up an asynchronous send using
				//	this object and the buffer within
				//
				//	Returns zero on success, an OS error
				//	code otherwise
				DWORD Dispatch (SOCKET) noexcept;
		
		
		};
		
		
		//	Contains information about an accepted
		//	connection
		class AcceptData {
		
		
			private:
			
			
				SOCKET socket;
				
				
				void cleanup () noexcept;

		
			public:
			
			
				AcceptData (SOCKET) noexcept;
				~AcceptData () noexcept;
				
				
				AcceptData (AcceptData &&) noexcept;
				AcceptData & operator = (AcceptData &&) noexcept;
				AcceptData (const AcceptData &) noexcept;
				AcceptData & operator = (const AcceptData &) noexcept;
			
			
				//	Gets the socket this AcceptData
				//	structure is carrying.  Ends the
				//	AcceptData structure's responsibility
				//	for managing that socket's lifetime
				SOCKET Get () noexcept;
			
			
				IPAddress RemoteIP;
				UInt16 RemotePort;
				IPAddress LocalIP;
				UInt16 LocalPort;
		
		
		};
		
		
		//	A command to accept an incoming connection on
		//	a given socket
		class AcceptCommand : public CompletionCommand {
		
		
			private:
			
			
				//	Actual socket to be accepted on
				SOCKET socket;
				//	Whether the underlying listening socket
				//	is IPv6 or IPv4
				bool is_v6;
				//	Buffer of bytes for AcceptEx to store
				//	addresses in
				Byte buffer [(sizeof(struct sockaddr_storage)+16)*2];
				//	Implementation artifact -- this is
				//	ignored
				DWORD num;
				//	Listening socket -- preserved for updating
				//	accepting socket's context
				SOCKET listening;
				
				
				void cleanup () noexcept;
				
				
			public:
			
			
				AcceptCommand (IPAddress) noexcept;
				
				
				~AcceptCommand () noexcept;
				
				
				//	Dispatches an asynchronous accept
				void Dispatch (SOCKET);
				//	Retrieves data about a completed receive
				AcceptData Get ();
		
		
		};
		
		
		//	A command to connect to a remote endpoint
		class ConnectCommand : public CompletionCommand {
		
		
			private:
			
			
				//	Remote endpoint
				struct sockaddr_storage addr;
				
				
			public:
			
			
				ConnectCommand () noexcept;
				
				
				//	Dispatches an asynchronous connect
				void Dispatch (SOCKET, IPAddress, UInt16);
		
		
		};
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Represents an endpoint on which a connection
	 *	handler is listening for incoming connections.
	 */
	class ListeningSocket {
	
	
		friend class ConnectionHandler;
	
	
		private:
		
		
			//	The listening socket
			SOCKET socket;
			
			
			//	The connection handler this is associated
			//	with
			ConnectionHandler & handler;
			
			
			//	The endpoint to which this socket
			//	is bound
			LocalEndpoint ep;
		
		
			//	Synchronizes shutting down
			Mutex lock;
			CondVar wait;
			
			
			//	Pending accept operations
			std::unordered_map<NetworkImpl::CompletionCommand *,std::unique_ptr<NetworkImpl::AcceptCommand>> pending;
			
			
			//	Completes an asynchronous operation
			void Complete (NetworkImpl::Packet);
			//	Attaches this listening socket to
			//	the completion port
			void Attach ();
			//	Dispatches an asynchronous accept
			//	operation, should be called once
			//	per worker thread
			void Dispatch ();
		
		
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			ListeningSocket (LocalEndpoint, ConnectionHandler &);
			
			
			/**
			 *	\endcond
			 */
			
			
			/**
			 *	Destroys the listening socket and all
			 *	associated resources.
			 */
			~ListeningSocket () noexcept;
			
			
			/**
			 *	Shuts the listening socket down.
			 *
			 *	Once this function returns all pending
			 *	incoming connections are guaranteed to have
			 *	been processed.
			 */
			void Shutdown () noexcept;
	
	
	};
	
	
	/**
	 *	Represents a single send across a single
	 *	connection, and allows for that send to be
	 *	monitored.
	 */
	class SendHandle {
	
	
		friend class Connection;
	
	
		private:
		
		
			//	A pointer to a send handle is
			//	guaranteed to be a pointer to
			//	a SendCommand, which is in turn
			//	guaranteed to be a pointer to
			//	an OVERLAPPED structure
			NetworkImpl::SendCommand Command;
			
			
			//	Current state of the send
			SendState state;
			mutable Mutex lock;
			mutable CondVar wait;
			Vector<std::function<void (SendState)>> callbacks;
			
			
			//	Marks the current send as having failed,
			//	and dispatches all waiting callbacks
			//	synchronously
			void Fail () noexcept;
			//	Marks the current send as having succeeded,
			//	and dispatches all waiting callbacks
			//	asynchronously using the provided thread
			//	pool
			void Complete (ThreadPool &);
			//	Simply sets the state
			//	As this does not invoke callbacks, it
			//	is assumed that this object is not
			//	yet referenced by more than one thread,
			//	and therefore this operation is not
			//	thread safe
			void SetState (SendState) noexcept;
			
			
		public:
		
		
			/**
			 *	\cond
			 */
			 
			 
			//	Creates a send handle, passing the provided argument
			//	through to the constructor of the underlying
			//	send command
			SendHandle (Vector<Byte>) noexcept;
			 
			 
			/**
			 *	\endcond
			 */
			 
			
			/**
			 *	Waits for the send to complete.
			 *
			 *	\return
			 *		The state in which the send completed.
			 */
			SendState Wait () const noexcept;
			/**
			 *	Adds a callback to be invoked once the
			 *	send completes.
			 *
			 *	The callback will be invoked immediately if
			 *	the send has already completed.
			 *
			 *	\param [in] callback
			 *		A callback which shall be invoked when the
			 *		send has completed.
			 */
			void Then (std::function<void (SendState)> callback);
			/**
			 *	Determines the send's current state.
			 *
			 *	\return
			 *		The send's current state.
			 */
			SendState State () const noexcept;
	
	
	};
	
	
	/**
	 *	Represents a connection to a single remote
	 *	host.
	 */
	class Connection {
	
	
		friend class ConnectionHandler;
		friend class ListeningSocket;
	
	
		private:
		
		
			//	This connection's socket
			SOCKET socket;
			
			
			//	The handler this connection
			//	is associated with
			ConnectionHandler & handler;
			
			
			//	Whether this connection's socket
			//	is shutdown or not
			//
			//	For certain reasons, this is protected
			//	by the send lock
			bool is_shutdown;
			
			
			//	The remote IP and port to
			//	which this socket is connected
			IPAddress remote_ip;
			UInt16 remote_port;
			//	The local IP and port from
			//	which this socket is connected
			//	(only relevant for sockets
			//	created by accepting on a
			//	listening socket)
			IPAddress local_ip;
			UInt16 local_port;
			
			
			//	Statistics
			std::atomic<Word> sent;
			std::atomic<Word> received;
			
			
			//	Callbacks
			DisconnectType disconnect;
			ReceiveType receive_callback;
			ConnectType connect_callback;
			
			
			//	Pending sends
			std::unordered_map<NetworkImpl::CompletionCommand *,SmartPointer<SendHandle>> sends;
			Mutex sends_lock;
			
			
			//	Pending operations
			std::atomic<Word> pending;
			
			
			//	Receive command
			NetworkImpl::ReceiveCommand recv;
			
			
			//	Connect command
			NetworkImpl::ConnectCommand conn;
			
			
			//	Reason (if any) this connection
			//	was closed
			bool set_reason;
			Nullable<String> reason;
			std::exception_ptr ex;
			Mutex reason_lock;
			
			
			//	Shuts the socket down
			void shutdown () noexcept;
			
			
			//	Completes an event by cleaning
			//	up
			void complete (DWORD code=0);
			
			
			//	Completion routines
			void receive (NetworkImpl::Packet);
			void send (NetworkImpl::Packet);
			void connect (NetworkImpl::Packet);
			
			
			//	Completes some pending asynchronous
			//	I/O operation against this connection
			void Complete (NetworkImpl::Packet);
			
			
			//	Dispatches an asynchronous connection
			//	attempt
			void Connect ();
			
			
			//	Attaches this connection to the completion
			//	port
			void Attach ();
			
			
			//	Begins the receive loop by queuing up a
			//	receive
			void Begin ();
			
			
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			Connection (SOCKET, IPAddress, UInt16, IPAddress, UInt16, ConnectionHandler &, ReceiveType, DisconnectType, ConnectType connect_callback=ConnectType());
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	Destroys this connection, closing its
			 *	socket and cleaning up all associated
			 *	resources.
			 */
			~Connection () noexcept;
			
			
			/**
			 *	Sends data across a connected connection.
			 *
			 *	\param [in] buffer
			 *		The data to sent.
			 *
			 *	\return
			 *		A handle through which the send may be
			 *		monitored.
			 */
			SmartPointer<SendHandle> Send (Vector<Byte> buffer);
			
			
			/**
			 *	Retrieves the remote IP associated with
			 *	this connection.
			 *
			 *	\return
			 *		The remote IP associated with this
			 *		connection.
			 */
			IPAddress IP () const noexcept;
			/** 
			 *	Retrieves the remote port associated with
			 *	this connection.
			 *
			 *	\return
			 *		The remote port associated with this
			 *		connection.
			 */
			UInt16 Port () const noexcept;
			/**
			 *	Retrieves the number of bytes sent over
			 *	this connection.
			 *
			 *	\return
			 *		The number of bytes sent over this
			 *		connection.
			 */
			Word Sent () const noexcept;
			/**
			 *	Retrieves the number of bytes received on
			 *	this connection.
			 *
			 *	\return
			 *		The number of bytes received on this
			 *		connection.
			 */
			Word Received () const noexcept;
			/**
			 *	Retrieves the number of sends pending on
			 *	this connection.
			 *
			 *	\return
			 *		The number of send operations pending on
			 *		this connection.
			 */
			Word Pending () const noexcept;
			/**
			 *	Disconnects this connection for no reason.
			 *
			 *	This function completing is not a guarantee
			 *	that callbacks related to this connection will
			 *	no longer be fired, but rather indicates that
			 *	the socket has been shutdown.
			 */
			void Disconnect () noexcept;
			/**
			 *	Disconnects this connection for some reason.
			 *
			 *	This function completing is not a guarantee
			 *	that callbacks related to this connection will
			 *	no longer be fired, but rather indicates that
			 *	the socket has been shutdown.
			 *
			 *	\param [in] reason
			 *		The reason for which the connection has
			 *		been terminated.  If the internal reason
			 *		for this socket being disconnected has
			 *		already been set either by another call
			 *		to Disconnect, or by an error internal to
			 *		the connection handler, this parameter is
			 *		silently ignored.
			 */
			void Disconnect (String reason) noexcept;
	
	
	};
	
	
	/**
	 *	A component which provides high-performance,
	 *	asynchronous networking capabilities.
	 */
	class ConnectionHandler {
	
	
		friend class Connection;
		friend class ListeningSocket;
	
	
		private:
		
		
			//	CONSTRUCTION ORDER IS VERY
			//	IMPORTANT
			
			
			//	FIRST WE INITIALIZE WINSOCK
			NetworkImpl::Initializer init;
			
			
			//	SECOND WE CREATE AN I/O COMPLETION PORT
			NetworkImpl::CompletionPort Port;
			
			
			//	THEN WE CREATE COLLECTIONS
		
		
			//	Connections
			std::unordered_map<Connection *,SmartPointer<Connection>> connections;
			Mutex connections_lock;
			
			//	Listening sockets
			std::unordered_map<ListeningSocket *,SmartPointer<ListeningSocket>> listening;
			Mutex listening_lock;
			
			
			//	Worker threads
			Vector<Thread> workers;
			
			
			//	This construction order insures that
			//	everything is cleaned up in the required
			//	order (since destructors fire from
			//	bottom to top).
			//
			//	This ensures that the I/O Completion Port
			//	is cleaned up after all the connections,
			//	and that WinSock is cleaned up after all
			//	connections are shut down.
			
			
			//	Reference manager prevents destruction
			//	until it's safe to do so
			NetworkImpl::ReferenceManager Manager;
			
			
			//	Thread Pool
			ThreadPool & Pool;
			
			
			//	Statistics
			
			//	Number of bytes sent
			std::atomic<Word> Sent;
			//	Number of bytes received
			std::atomic<Word> Received;
			//	Number of incoming connections
			std::atomic<Word> Incoming;
			//	Number of outgoing connections
			std::atomic<Word> Outgoing;
			//	Number of accepted connections
			std::atomic<Word> Accepted;
			//	Number of disconnected connections
			std::atomic<Word> Disconnected;
			
			
			//	Startup co-ordination
			
			enum class StartupResult {
			
				//	Still waiting for constructor to finish
				None,
				//	Startup failed, workers should exit at once
				Failed,
				//	Startup succeeded, workers should start
				Succeeded
			
			};
			
			Mutex lock;
			CondVar wait;
			StartupResult startup;
			
			
			//	Panic callback
			PanicType panic;
			
			
			//	Worker thread function
			void worker ();
			
			
			//	Removes connections and listening sockets
			void Remove (const Connection *) noexcept;
			void Remove (const ListeningSocket *) noexcept;
			//	Retieves connections and listening sockets
			SmartPointer<Connection> Get (const Connection *) noexcept;
			SmartPointer<ListeningSocket> Get (const ListeningSocket *) noexcept;
			//	Adds connections and listening sockets
			Connection * Add (SmartPointer<Connection>);
			ListeningSocket * Add (SmartPointer<ListeningSocket>);
			
			
			//	Enqueues a task to run in the thread pool
			//	associated with the connection handler
			//
			//	The connection handler is guaranteed to
			//	exist at the same address until this
			//	asynchronous task ends
			template <typename T, typename... Args>
			void Enqueue (T && callback, Args &&... args);
			
			
			//	Panics
			void Panic (std::exception_ptr) noexcept;
			
			
		public:
		
		
			/**
			 *	Creates a new connection handler.
			 *
			 *	\param [in] pool
			 *		The thread pool that the connection
			 *		handler will use to dispatch asynchronous
			 *		callbacks.
			 *	\param [in] num_workers
			 *		The number of worker threads that the
			 *		connection handler should create and
			 *		maintain internally to service connections.
			 *		Optional.  If not provided or set to
			 *		null defaults to the number of threads of
			 *		execution supported by this computer's
			 *		CPU.
			 *	\param [in] panic
			 *		A callback to be invoked when and if a
			 *		critical error occurs within the connection
			 *		handler.  Defaults to calling std::abort.
			 */
			ConnectionHandler (
				ThreadPool & pool,
				Nullable<Word> num_workers=Nullable<Word>(),
				PanicType panic=PanicType()
			);
		
		
			/**
			 *	Shuts down the connection handler, closing all
			 *	connections and stopping all worker threads.
			 */
			~ConnectionHandler () noexcept;
			
			
			/**
			 *	Forms an outgoing connection.
			 *
			 *	\param [in] ep
			 *		The remote endpoint to which the handler
			 *		shall attempt to connect.
			 */
			void Connect (RemoteEndpoint ep);
			
			
			/**
			 *	Begins listening on a certain local endpoint.
			 *
			 *	\param [in] ep
			 *		The local endpoint on which to listen.
			 *
			 *	\return
			 *		A handle through which the listening socket
			 *		may be manipulated.
			 */
			SmartPointer<ListeningSocket> Listen (LocalEndpoint ep);
			
			
			/**
			 *	Gets a structure which contains information
			 *	about this connection handler.
			 */
			ConnectionHandlerInfo GetInfo () const noexcept;
	
	
	};
	
	
	/**
	 *	\cond
	 */
	
	
	template <typename T, typename... Args>
	void ConnectionHandler::Enqueue (T && callback, Args &&... args) {
	
		//	Get a handle that will insure the
		//	connection handler stays alive
		auto handle=Manager.Get();
		
		//	Run callback in pool
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		Pool.Enqueue([
			handle=std::move(handle),
			callback=std::bind(
				std::forward<T>(callback),
				std::forward<Args>(args)...
			)
		] () mutable {	callback();	});
		#pragma GCC diagnostic pop
	
	}
	
	
	/**
	 *	\endcond
	 */


}
 
/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <network.hpp>
#include <thread_pool.hpp>
#ifdef linux
#include <sys/epoll.h>
#else
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif
#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
 
 
namespace MCPP {


	/**
	 *	\cond
	 */


	namespace NetworkImpl {
	
	
		//	Determines if a network call failed
		//	because of an actual failure, or
		//	simply because it would block
		bool WouldBlock () noexcept;
		
		
		//	Raises an exception appropriate for
		//	the last system error that occurred
		[[noreturn]]
		void Raise ();
		
		
		//	Makes a socket non blocking
		void SetNonBlocking (int);
		
		
		//	Makes a socket blocking
		void SetBlocking (int);
		
		
		class ControlSocket {
		
		
			private:
		
		
				//	Master end uses socket 0,
				//	Slave end uses socket 1
				int sockets [2];
				Mutex lock;
				
				
			public:
			
		
				ControlSocket ();
				~ControlSocket () noexcept;
				
				
				//	Retrieves messages from the control
				//	socket
				void Get ();
				//	Posts messages to the control socket
				void Put ();
				//	Retrieves the file descriptor associated
				//	with this control socket, which may be
				//	associated with a notifier so that inter
				//	thread communications may be efficiently
				//	received
				int Wait () const noexcept;
		
		
		};
		
		
		class Notification {
		
		
			private:
			
			
				struct
				#ifdef linux
				epoll_event
				#else
				kevent
				#endif
				event;
		
		
			public:
			
			
				//	Retrieves the socket associated
				//	with a notification
				int Socket () const noexcept;
				//	Determines whether this notification
				//	indicates an error on the socket-in-question
				bool Error () const noexcept;
				//	Determines whether this notification
				//	indicates that the socket-in-question
				//	may be read
				bool Readable () const noexcept;
				//	Determines whether this notification
				//	indicates that the socket-in-question
				//	may be written
				bool Writeable () const noexcept;
		
		
		};
	
	
		class Notifier {
		
		
			private:
		
		
				int handle;
				
				
			public:
			
			
				Notifier ();
				~Notifier () noexcept;
				
				
				//	Attaches a socket to this
				//	notifier.
				void Attach (int);
				//	Updates the notifications that
				//	this notifier will provide for
				//	a given socket
				void Update (int, bool, bool);
				//	Waits until one or more notifications
				//	are available.  Returns the number
				//	of notifications dequeued
				Word Wait (Notification *, Word);
				//	Detaches a socket, preventing
				//	future notifications
				void Detach (int);
		
		
		};
		
		
		class WorkerThread {
		
		
			private:
			
			
				std::unordered_set<int> pending;
				std::unordered_set<int> ignored;
				std::unordered_map<int,SmartPointer<Connection>> connections;
				mutable Mutex lock;
				ControlSocket control;
				Notifier notifier;
				
				
			public:
			
			
				WorkerThread ();
			
			
				//	Associates a connection with this worker
				//	thread.  The thread will be sent an inter
				//	thread message which shall cause the connection
				//	to be associated with it
				void AddSocket (int, SmartPointer<Connection>);
				//	Retrieves pending inter thread communication
				//	messages.  Should be called until null is
				//	returned.  If the returned value is a socket
				//	that exists on this connection, that socket
				//	should be updated so that it may attach
				//	to this thread's notifier (it has just been
				//	added), otherwise it is a command for the
				//	worker to shut down
				Nullable<int> GetSocket ();
				//	Sends an inter thread message to tell the
				//	worker to wake up and update the file
				//	descriptor provided
				void Put (int);
				
				
				//	Issues a command to this worker thread
				//	that it should shut down
				void Shutdown ();
				
				
				//	Retrieves the connection object associated
				//	with a given file descriptor
				SmartPointer<Connection> Get (int) const noexcept;
				//	Removes the connection associated with a
				//	given file descriptor from this worker
				//	thread
				void Remove (int);
				
				
				//	Determines whether a given socket is
				//	the control socket for this worker
				//	thread
				bool IsControl (int) const noexcept;
			
				
				//	Determines how many connections are
				//	associated with this worker thread
				Word Count () const noexcept;
				
				
				//	Retrieves the notifier associated with
				//	this worker thread
				Notifier & Get () noexcept;
				
				
				//	Clears the list of ignored sockets,
				//	should be called after the worker
				//	finishes processing all dequeued
				//	events
				void Clear () noexcept;
				//	Determines whether or not a given
				//	file descriptor is ignored
				bool IsIgnored (int) const noexcept;
		
		
		};
		
		
		class ListeningSocket {
		
		
			private:
			
			
				int socket;
				IPAddress ip;
				UInt16 port;
				
				
				void destroy () noexcept;
				
				
			public:
			
			
				typedef Tuple<int,IPAddress,UInt16,IPAddress,UInt16> ConnectionType;
			
			
				ListeningSocket (IPAddress, UInt16);
				~ListeningSocket () noexcept;
				
				
				ListeningSocket (const ListeningSocket &) = delete;
				ListeningSocket & operator = (const ListeningSocket &) = delete;
				ListeningSocket (ListeningSocket &&) noexcept;
				ListeningSocket & operator = (ListeningSocket &&) noexcept;
			
			
				//	Attaches this listening socket to a given
				//	notifier
				void Attach (Notifier &);
				//	Accepts an incoming connection on this
				//	listening socket.  Should be called
				//	until it return null
				Nullable<ConnectionType> Accept ();
				//	Gets the socket that this listening
				//	socket manages
				int Get () const noexcept;
		
		
		};
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	A handle to an asynchronous
	 *	send operation.
	 *
	 *	The handle may be used to monitor
	 *	the progress of the send, attach
	 *	a callback to be executed after
	 *	the send completes, or to wait until
	 *	the send completes.
	 */
	class SendHandle {
	
	
		friend class Connection;
	
	
		private:
		
		
			Vector<Byte> Buffer;
			Word Sent;
			
			
			mutable Mutex lock;
			mutable CondVar wait;
			SendState state;
			Vector<std::function<void (SendState)>> callbacks;
			
			
			//	Causes the send handle to fail
			//	synchronously
			void Fail () noexcept;
			//	Causes the send handle to succeed
			//	asynchronously
			void Complete (ThreadPool &);
			
			
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			SendHandle (Vector<Byte> buffer) noexcept;
			
			
			/**
			 *	\endcond
			 */
			
			
			/**
			 *	Determines the state the send
			 *	operation is currently in.
			 *
			 *	\return
			 *		The operation that the send
			 *		operation is currently in.
			 */
			SendState State () const noexcept;
			/**
			 *	Waits until the send operation
			 *	completes.
			 *
			 *	\return
			 *		The state the send operation
			 *		terminated in.
			 */
			SendState Wait () const noexcept;
			/**
			 *	Attaches a callback to be executed
			 *	after the send operation completes.
			 *
			 *	The callback will be passed the
			 *	state in which the send operation
			 *	terminated.
			 *
			 *	\param [in] callback
			 *		A callback to be invoked once
			 *		the send operation completes.
			 */
			void Then (std::function<void (SendState)> callback);
	
	
	};
	
	
	/**
	 *	Encapsulates a connection to a remote
	 *	host.
	 */
	class Connection {
	
	
		friend class ConnectionHandler;
	
	
		private:
		
		
			//	Socket
			int socket;
			
			
			//	Worker thread associated with
			//	this connection
			NetworkImpl::WorkerThread & worker;
			
			
			//	Whether or not the socket has
			//	been shutdown
			bool is_shutdown;
			//	Whether or not the socket has
			//	been attached to its associated
			//	notifier
			bool attached;
			
			
			//	Lock
			mutable Mutex lock;
			
			
			//	Pending sends
			Vector<SmartPointer<SendHandle>> sends;
			
			
			//	Receive buffer
			Vector<Byte> buffer;
			bool callback_in_progress;
			
			
			//	Endpoints -- local and
			//	remote
			IPAddress remote_ip;
			UInt16 remote_port;
			IPAddress local_ip;
			UInt16 local_port;
			
			
			//	Statistics
			std::atomic<Word> received;
			std::atomic<Word> sent;
			
			
			//	Reason and lock
			String reason;
			mutable Mutex reason_lock;
			
			
			//	Updates this connection by telling the
			//	notifier whether notifications for
			//	readability, writeability, both, or
			//	neither should be passed through to
			//	the associated worker thread
			void update ();
			//	Disconnects this connection by shutting
			//	down its socket, and then tells the worker
			//	thread to remove/detach/close it
			void disconnect ();
			//	Shuts this connection down by shutting down
			//	its socket
			void shutdown () noexcept;
			
			
			//	Retrieves this connection's receive
			//	buffer
			Vector<Byte> & Get () noexcept;
			//	Disconnects this connection by shutting
			//	down its socket
			void Shutdown () noexcept;
			//	Updates this connection and attaches it
			//	to the associated notifier.  If false
			//	is returned the connection should be
			//	removed from the worker as it has been
			//	disconnected
			bool Update ();
			//	Performs a non-blocking receive.  Returns
			//	the number of bytes received.  Assuming
			//	readability was reported, returning zero
			//	is an error condition
			Word Receive ();
			//	Calling Receive will cause the connection
			//	to disable notifications of readability
			//	through its associated notifier.  This
			//	must be called when such notifications
			//	should resume
			void CompleteReceive ();
			//	Performs a non-blocking send.  Given that
			//	the notifier reported readability, and
			//	this connection has pending sends, returning
			//	zero is an error condition
			Word Send (ThreadPool &);
			//	Retrieves the file descriptor associated
			//	with this connection's socket
			int Socket () const noexcept;
			//	Retrieves the reason given for this socket
			//	being disconnected (if any)
			String Reason () noexcept;
			
			
		public:
		
		
			/** 
			 *	\cond
			 */
		
		
			Connection (int, IPAddress, UInt16, NetworkImpl::WorkerThread &, IPAddress, UInt16) noexcept;
			
			
			/**
			 *	\endcond
			 */
			
			
			/**
			 *	Disconnects this connection.
			 */
			~Connection () noexcept;
		
		
			/**
			 *	Asynchronously sends data across this
			 *	connection.
			 *
			 *	\param [in] buffer
			 *		A buffer of bytes to send.
			 *
			 *	\return
			 *		A handle which may be used to track,
			 *		monitor, and respond to changes in the
			 *		status of this asynchronous send operation.
			 */
			SmartPointer<SendHandle> Send (Vector<Byte> buffer);
			
			
			/**
			 *	Disconnects this connection.
			 */
			void Disconnect ();
			/**
			 *	Disconnects this connection for a certain
			 *	reason.
			 *
			 *	\param [in] reason
			 *		A string representing the reason that
			 *		this connection is being disconnected.
			 */
			void Disconnect (String reason);
			
			
			/**
			 *	Retrieves the remote IP.
			 *
			 *	\return
			 *		An IP address which represents the
			 *		IP address to which this connection is
			 *		connected.
			 */
			IPAddress IP () const noexcept;
			/**
			 *	Retrieves the remote port.
			 *
			 *	\return
			 *		An integer which represents the port
			 *		to which this connection is connected.
			 */
			UInt16 Port () const noexcept;
			
			
			/**
			 *	Retrieves the local IP.
			 *
			 *	\return
			 *		An IP address which represents the
			 *		IP address on this machine to which
			 *		this connection is connected.
			 */
			IPAddress LocalIP () const noexcept;
			/**
			 *	Retrieves the local port.
			 *
			 *	\return
			 *		An integer which represents the port
			 *		on this machine to which this connection
			 *		is connected.
			 */
			UInt16 LocalPort () const noexcept;
			
			
			/**
			 *	Retrieves the number of bytes which have
			 *	been send across this connection.
			 *
			 *	\return
			 *		The number of bytes sent.
			 */
			Word Sent () const noexcept;
			/**
			 *	Retrieves the number of bytes which have
			 *	been received on this connection.
			 *
			 *	\return
			 *		The number of bytes received.
			 */
			Word Received () const noexcept;
			
			
			/**
			 *	Determines the number of asynchronous
			 *	send operations currently pending on this
			 *	connection.
			 *
			 *	\return
			 *		The number of send operations pending
			 *		on this connection.
			 */
			Word Pending () const noexcept;
	
	
	};
	
	
	/**
	 *	Handles connections, creating listening sockets,
	 *	accepting connections, and performing
	 *	high-performance, non-blocking, asynchronous
	 *	sends and receives.
	 */
	class ConnectionHandler {
	
	
		private:
		
		
			//	Thread pool
			ThreadPool & pool;
			
			
			//	Callbacks
			ReceiveCallback recv;
			DisconnectCallback disconnect;
			AcceptCallback accept;
			ConnectCallback connect;
			PanicCallback panic;
			
			
			//	Listening sockets
			std::unordered_map<int,NetworkImpl::ListeningSocket> listening;
			
			
			//	Worker threads
			Vector<
				Tuple<
					std::unique_ptr<NetworkImpl::WorkerThread>,
					Thread
				>
			> workers;
			
			
			//	Asynchronous callback tracking
			Word pending;
			Mutex lock;
			CondVar wait;
			
			
			//	Startup
			Mutex startup;
			CondVar startup_wait;
			bool proceed;
			bool success;
			
			
			//	Statistics
			std::atomic<Word> sent;
			std::atomic<Word> received;
			std::atomic<Word> connected;
			std::atomic<Word> accepted;
			
			
			//	Enqueues a function to run asynchronously
			//	in the thread pool, while appropriately
			//	and automatically maintaining the asynchronous
			//	counts and appropriately waking up the
			//	destructor (if it's waiting) when an asynchronous
			//	callback completes.
			//
			//	Asynchronous callbacks depend on this object
			//	existing and remaining in the same place in memory
			//	(it is immovable so this is a reasonable
			//	assumption) until they complete.  Therefore there
			//	is a count of the number of asynchronous callbacks
			//	running, and each asynchronous callback awakens
			//	a condition variable as it exits.  This way the
			//	destructor may wait for all asynchronous callbacks
			//	to complete before allowing this object to be
			//	cleaned up, thereby maintaining memory safety.
			template <typename T, typename... Args>
			void enqueue (const T &, Args &&...);
			
			
			//	Kills a connection, firing its disconnect handler,
			//	shutting down its socket, and removing it from
			//	its associated worker thread
			void kill (SmartPointer<Connection>, NetworkImpl::WorkerThread &);
			
			
			//	Perfoms various initialization tasks for
			//	the worker thread.
			//
			//	Also catches all the worker's threads
			//	exceptions and panicks on them.
			void thread_func_init (Word) noexcept;
			//	Performs accepts, sends, and receives
			//	on behalf of a number of threads which
			//	are bound to it.
			void thread_func (NetworkImpl::WorkerThread &);
			
			
			//	Panicks, passing a given exception
			//	through to the panic handler
			void do_panic (std::exception_ptr) noexcept;
			
			
			//	Selects the worker thread with the lowest
			//	number of connections
			NetworkImpl::WorkerThread & select () noexcept;
			
			
			//	Selects the listening socket that is
			//	associated with the given file
			//	descriptor
			NetworkImpl::ListeningSocket & get (int) noexcept;
			
			
			//	Performs the necessary tasks to transform
			//	a newly-accepted connection into a full
			//	client connection, including invoking
			//	the accept (destroying the connection
			//	should it be desirous) and connect
			//	callbacks
			void make (NetworkImpl::ListeningSocket::ConnectionType);
		
		
		public:
		
		
			/**
			 *	Creates a new connection handler and immediately
			 *	begins listening.
			 *
			 *	\param [in] binds
			 *		A vector of tuples which contain as their
			 *		first item an IP address, and as their
			 *		second item a port number.  These are the
			 *		local IP addresses on which the handler
			 *		will listen for incoming connections.
			 *	\param [in] accept
			 *		A callback which will be invoked to filter
			 *		incoming connections.  If it returns
			 *		\em false the connection will be ended
			 *		at once, otherwise the connection will be
			 *		allowed.
			 *	\param [in] connect
			 *		A callback which will be invoked whenever
			 *		a connection to the handler is allowed.
			 *	\param [in] disconnect
			 *		A callback which will be invoked whenever
			 *		a connection to the handler ends.
			 *	\param [in] recv
			 *		A callback which will be invoked whenever
			 *		the handler receives data on any connection.
			 *		The handler will not process subsequent
			 *		receives on that connection until this callback
			 *		has returned.
			 *	\param [in] panic
			 *		A callback which will be invoked when and
			 *		if any internal, irrecoverable errors occur
			 *		within the handler.
			 *	\param [in] pool
			 *		A reference to a thread pool that the handler
			 *		will use to dispatch asynchronous callbacks.
			 *	\param [in] num_workers
			 *		A nullable integer representing the number of
			 *		worker threads to spawn.  If null as many
			 *		threads as the current CPU has cores will be
			 *		spawned.  If zero one thread will be spawned.
			 *		Defaults to null.
			 */
			ConnectionHandler (
				const Vector<Tuple<IPAddress,UInt16>> & binds,
				AcceptCallback accept,
				ConnectCallback connect,
				DisconnectCallback disconnect,
				ReceiveCallback recv,
				PanicCallback panic,
				ThreadPool & pool,
				Nullable<Word> num_workers=Nullable<Word>()
			);
			/**
			 *	Cleans up a connection handler, closing all
			 *	connections and listening sockets, and
			 *	ending all worker threads.
			 */
			~ConnectionHandler () noexcept;
	
	
	};


}
 
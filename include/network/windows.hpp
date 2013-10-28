/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <network.hpp>
#include <thread_pool.hpp>
#include <windows.h>
#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <unordered_map>


namespace MCPP {
	
	
	/**
	 *	\cond
	 */


	namespace NetworkImpl {
	
	
		SOCKET MakeSocket (bool);
	
	
		enum class NetworkCommand {
		
			Receive,
			Send,
			Accept
		
		};
		
		
		class IOCPCommand {
		
		
			public:
			
			
				OVERLAPPED Overlapped;
				NetworkCommand Command;
				
				
				IOCPCommand (NetworkCommand) noexcept;
		
		
		};
		
		
		class IOCPPacket {
		
		
			public:
			
			
				bool Result;
				void * Conn;
				IOCPCommand * Command;
				Word Num;
		
		
		};
		
		
		class AcceptCommand : public IOCPCommand {
		
		
			private:
			
			
				SOCKET socket;
				SOCKET listening;
				Byte buffer [(sizeof(struct sockaddr_storage)+16)*2];
				DWORD num;
		
		
			public:
				
				
				AcceptCommand (SOCKET, SOCKET) noexcept;
				~AcceptCommand () noexcept;
				
				
				void Imbue (SOCKET) noexcept;
				SOCKET Get () noexcept;
				Tuple<IPAddress,UInt16,IPAddress,UInt16> GetEndpoints ();
				
				
				void Dispatch (SOCKET);
		
		
		};
		
		
		class ReceiveCommand : public IOCPCommand {
		
		
			private:
			
			
				WSABUF b;
				DWORD recv_flags;
		
		
			public:
			
			
				Vector<Byte> Buffer;
				
				
				ReceiveCommand () noexcept;
				
				
				bool Dispatch (SOCKET);
				bool Complete (IOCPPacket);
		
		
		};
		
		
		class IOCP {
		
		
			private:
			
			
				HANDLE iocp;
				
				
				void destroy () noexcept;
				
				
			public:
			
			
				IOCP ();
				~IOCP ();
				
				
				void Attach (SOCKET, void *);
				void Dispatch (IOCPCommand *, void *);
				IOCPPacket Get ();
				void Destroy () noexcept;
		
		
		};
		
		
		class ListeningSocket {
		
		
			private:
			
			
				SOCKET socket;
				IPAddress ip;
				UInt16 port;
				
				
				Vector<std::unique_ptr<AcceptCommand>> available;
				std::unordered_map<AcceptCommand *,std::unique_ptr<AcceptCommand>> pending;
				Mutex lock;
				
				
			public:
			
			
				ListeningSocket (IPAddress, UInt16, IOCP &);
				
				
				void Dispatch ();
				void Complete (AcceptCommand *);
		
		
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
	class SendHandle : private NetworkImpl::IOCPCommand {
	
	
		friend class ConnectionHandler;
		friend class Connection;
	
	
		private:
		
		
			//	Buffer that's being sent
			Vector<Byte> buffer;
			WSABUF b;
			
			
			//	Bytes successfully sent
			std::atomic<Word> sent;
			
			
			//	State of the send
			SendState state;
			mutable Mutex lock;
			mutable CondVar wait;
			Vector<std::function<void (SendState)>> callbacks;
			
			
			void Fail () noexcept;
			bool Complete (NetworkImpl::IOCPPacket, ThreadPool &);
			void Dispatch (SOCKET);
			
			
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
		
		
			//	Associated socket and
			//	I/O completion port
			SOCKET socket;
			
			
			//	Whether the socket has been
			//	shutdown or not
			bool is_shutdown;
			
			
			//	How many pending asynchronous
			//	operations are running against
			//	this connection
			Word pending;
			
			
			//	Statistics
			std::atomic<Word> sent;
			std::atomic<Word> received;
			
			
			//	Endpoints -- local and
			//	remote
			IPAddress remote_ip;
			UInt16 remote_port;
			IPAddress local_ip;
			UInt16 local_port;
			
			
			//	Receive command
			NetworkImpl::ReceiveCommand recv;
			
			
			//	Reason for disconnect (if
			//	provided)
			String reason;
			Mutex reason_lock;
			
			
			//	Collection of pending sends
			std::unordered_map<SendHandle *,SmartPointer<SendHandle>> sends;
			mutable Mutex sends_lock;
			
			
			bool Dispatch ();
			bool Complete (SendHandle *);
			bool Complete () noexcept;
			bool Kill () noexcept;
			String Reason () noexcept;
			void Send (Word) noexcept;
			void Receive (Word) noexcept;
			
			
			void disconnect () noexcept;
			
			
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			Connection (SOCKET, IPAddress, UInt16, NetworkImpl::IOCP &, IPAddress, UInt16);
			
			
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
		
		
			//	Completion port
			NetworkImpl::IOCP iocp;
			
			
			//	Thread pool
			ThreadPool & pool;
			
			
			//	Callbacks
			ReceiveCallback recv;
			DisconnectCallback disconnect;
			AcceptCallback accept;
			ConnectCallback connect;
			PanicCallback panic;
			
			
			//	Asynchronous callback tracking
			Word pending;
			Mutex lock;
			CondVar wait;
	
	
			//	Connections this handler is
			//	responsible for
			std::unordered_map<Connection *,SmartPointer<Connection>> connections;
			RWLock connections_lock;
			
			
			//	Listening sockets this handler
			//	is responsible for
			Vector<NetworkImpl::ListeningSocket> listening;
			
			
			//	Worker threads
			Vector<Thread> workers;
			
			
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
			
			
			void do_panic (std::exception_ptr) noexcept;
			void worker_init () noexcept;
			void worker_func ();
			void kill (SmartPointer<Connection> &);
			template <typename T, typename... Args>
			void enqueue (const T &, Args &&...);
			SmartPointer<Connection> get (Connection *) noexcept;
			void make (SOCKET, Tuple<IPAddress,UInt16,IPAddress,UInt16>);
			
			
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
 
/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <exception>
#include <functional>


namespace MCPP {


	/**
	 *	\cond
	 */
	 
	 
	class ListeningSocket;
	class SendHandle;
	class Connection;
	class ConnectionHandler;
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	The type of callback invoked when a critical,
	 *	irrecoverable error occurs.
	 */
	typedef std::function<void (std::exception_ptr)> PanicType;
	
	
	/**
	 *	Describes a disconnect.
	 */
	class DisconnectEvent {
	
	
		public:
		
		
			/**
			 *	The connection that was disconnect.
			 */
			SmartPointer<Connection> Conn;
			/**
			 *	If a reason for the disconnect was given,
			 *	it is contained here.
			 */
			Nullable<String> Reason;
			/**
			 *	If an error of some type caused the disconnect,
			 *	an exception representing that error is
			 *	contained here.
			 */
			std::exception_ptr Error;
	
	
	};
	
	
	/**
	 *	The type of callback invoked on a disconnect.
	 */
	typedef std::function<void (DisconnectEvent)> DisconnectType;
	
	
	/**
	 *	Describes a receive.
	 */
	class ReceiveEvent {
	
	
		public:
		
		
			/**
			 *	The connection on which data was
			 *	received.
			 */
			SmartPointer<Connection> Conn;
			/**
			 *	A buffer of received bytes.
			 *
			 *	This buffer is not cleared between
			 *	receives.  The caller is responsible
			 *	for maintaining this buffer in a
			 *	suitable state.
			 */
			Vector<Byte> & Buffer;
	
	
	};
	
	
	/**
	 *	The type of callback invoked on a receive.
	 */
	typedef std::function<void (ReceiveEvent)> ReceiveType;
	
	
	/**
	 *	Describes an accept.
	 *
	 *	An accept occurs when a client connects to
	 *	a listening socket, before the connection
	 *	is fully realized and data is sent/received.
	 *
	 *	Refusing an accept causes the client's socket
	 *	to be closed immediately, with very little
	 *	overhead.
	 */
	class AcceptEvent {
	
	
		public:
		
		
			IPAddress RemoteIP;
			UInt16 RemotePort;
			IPAddress LocalIP;
			UInt16 LocalPort;
	
	
	};
	
	
	/**
	 *	The type of callback invoked on an accept.
	 */
	typedef std::function<bool (AcceptEvent)> AcceptType;
	
	
	/**
	 *	Describes a connect.
	 *
	 *	Connects occur in one of two scenarios:
	 *
	 *	1.	A client connects to a listening socket,
	 *		and either no accept callback was provided,
	 *		or the accept callback returned \em true.
	 *	2.	A requested outbound connection succeeds.
	 */
	class ConnectEvent {
	
	
		public:
		
		
			/**
			 *	The connection.
			 *
			 *	If this is null, an error occurred,
			 *	and the next two fields should be
			 *	examined to determine the cause of
			 *	the error.
			 *
			 *	Connects to listening sockets are
			 *	guaranteed to always have this
			 *	field populated, and the next two
			 *	fields nulled.
			 */
			SmartPointer<Connection> Conn;
			/**
			 *	A string describing the error which
			 *	occurred, if any.
			 */
			Nullable<String> Reason;
			/**
			 *	The exception which caused the error
			 *	to occur, if any.
			 */
			std::exception_ptr Error;
	
	
	};
	
	
	/**
	 *	The type of callback invoked on a connect.
	 */
	typedef std::function<void (ConnectEvent)> ConnectType;


	/**
	 *	An IP/Port pair.
	 *
	 *	This class is not used directly, but is
	 *	the base for RemoteEndpoint and LocalEndpoint,
	 *	which are used in describing the endpoint
	 *	of a listening socket or connection.
	 */
	class Endpoint {
	
	
		public:
		
		
			/**
			 *	The IP address associated with this
			 *	endpoint.
			 */
			IPAddress IP;
			/**
			 *	The port associated with this endpoint.
			 */
			UInt16 Port;
	
	
	};
	
	
	/**
	 *	Contains data required to manage an outbound
	 *	connection.
	 *
	 *	LocalEndpoint is derived from this class.
	 */
	class RemoteEndpoint : public Endpoint {
	
	
		public:
		
		
			/**
			 *	A callback to be invoked when the
			 *	connection attempt succeeds or
			 *	fails.
			 *
			 *	If this object is a LocalEndpoint,
			 *	this is the connection that will be
			 *	invoked when incoming connections
			 *	have been accepted.
			 */
			ConnectType Connect;
			/**
			 *	A callback to be invoked when the
			 *	connection disconnects.
			 */
			DisconnectType Disconnect;
			/**
			 *	A callback to be invoked when data
			 *	is received on the connection.
			 */
			ReceiveType Receive;
	
	
	};
	
	
	/**
	 *	Contains data required to manage a listening
	 *	socket.
	 */
	class LocalEndpoint : public RemoteEndpoint {
	
	
		public:
		
		
			/**
			 *	A callback to be invoked immediately
			 *	upon receiving an incoming connection
			 *	attempt.
			 *
			 *	If this callback returns \em true, or
			 *	is not specified, the connection will
			 *	be accepted, otherwise the connection
			 *	will immediately be terminated.
			 */
			AcceptType Accept;
	
	
	};
	
	
	/**
	 *	Contains information about a ConnectionHandler.
	 */
	class ConnectionHandlerInfo {
	
	
		public:
		
		
			/**
			 *	Number of bytes that have been sent by
			 *	connections associated with the connection
			 *	handler.
			 */
			Word Sent;
			/**
			 *	Number of bytes that have been received
			 *	by connections associated with the connection
			 *	handler.
			 */
			Word Received;
			/**
			 *	Number of outgnow Ioing connections that have
			 *	been successfully established by the
			 *	connection handler.
			 */
			Word Outgoing;
			/**
			 *	Number of incoming connections that have
			 *	been formed on sockets the connection
			 *	handler is listening on.
			 */
			Word Incoming;
			/**
			 *	Number of incoming connections which have
			 *	been accepted and realized as Connection
			 *	objects.
			 */
			Word Accepted;
			/**
			 *	Number of connections which have been
			 *	terminated for any reason.
			 */
			Word Disconnected;
	
	
	};
	
	
	/**
	 *	The different states through which
	 *	a send transitions.
	 */
	enum class SendState {
	
		Sending,	/**<	The send is either waiting or currently being sent.	*/
		Sent,		/**<	The send completed successfully.	*/
		Failed		/**<	The send failed.	*/
	
	};


}


#ifdef ENVIRONMENT_WINDOWS
#include <network/windows.hpp>
#endif
 
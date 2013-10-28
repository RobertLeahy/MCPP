#pragma once


#include <rleahylib/rleahylib.hpp>
#include <functional>
#include <exception>


namespace MCPP {


	/**
	 *	\cond
	 */


	class Connection;
	class ConnectionHandler;
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	The type of callback which may be invoked
	 *	when a client disconnects.
	 *
	 *	Is passed the following parameters:
	 *
	 *	1.	The connection which was disconnected.
	 *	2.	The reason the connection was disconnected,
	 *		or the empty string, if no reason was given.
	 */
	typedef std::function<void (SmartPointer<Connection>, String)> DisconnectCallback;
	/**
	 *	The type of callback which may be invoked
	 *	when data is received from a client.
	 *
	 *	Is passed the following parameters:
	 *
	 *	1.	The connection on which data was received.
	 *	2.	A buffer of bytes pending processing on
	 *		the connection-in-question.  It is the callees
	 *		responsibility to remove data from this buffer
	 *		as it is consumed.  If the function does not
	 *		consume all data from the buffer, the remaining
	 *		bytes will be preserved, and the callback will
	 *		be invoked again once more data is appended.
	 */
	typedef std::function<void (SmartPointer<Connection>, Vector<Byte> &)> ReceiveCallback;
	/**
	 *	The type of callback which may be invoked
	 *	when an incoming connection is formed, but
	 *	before that connection is hooked into the
	 *	send/receive process.
	 *
	 *	Is passed the following parameters:
	 *
	 *	1.	The remote IP from which the client is
	 *		connecting.
	 *	2.	The remote port from which the client is
	 *		connectiong.
	 *	3.	The local IP to which the client is
	 *		connecting.
	 *	4.	The local port to which the client is
	 *		connecting.
	 *
	 *	If \em true is returned, the connection is
	 *	accepted, otherwise the connection is
	 *	immediately forcefully terminated.
	 */
	typedef std::function<bool (IPAddress, UInt16, IPAddress, UInt16)> AcceptCallback;
	/**
	 *	The type of callback which may be invoked
	 *	when an incoming connection is accepted.
	 *
	 *	It is guaranteed that receive events will
	 *	not be processed until this callback has
	 *	completed.
	 *
	 *	Is passed the following parameter:
	 *
	 *	1.	The newly-formed connection.
	 */
	typedef std::function<void (SmartPointer<Connection>)> ConnectCallback;
	/**
	 *	The type of callback which may be invoked
	 *	when a ConnectionHandler encounters an
	 *	irrecoverable internal error.
	 *
	 *	Is passed the following parameter:
	 *
	 *	1.	The exception which was thrown internally.
	 */
	typedef std::function<void (std::exception_ptr)> PanicCallback;
	
	
	/**
	 *	The varies states in which an
	 *	asynchronous send operation
	 *	may be.
	 */
	enum class SendState {
	
		InProgress,	/**<	Data is being sent or is waiting to be sent.	*/
		Succeeded,	/**<	Data has been completely sent.	*/
		Failed		/**<	Data could not be sent.	*/
		
	};


}


#ifdef ENVIRONMENT_WINDOWS
#include <network/windows.hpp>
#else
#include <network/posix.hpp>
#endif

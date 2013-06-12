/**
 *	\file
 */


#pragma once


/**
 *	\cond
 */


namespace MCPP {


	class ListenHandler;


}


/**
 *	\endcond
 */


#include <rleahylib/rleahylib.hpp>
#include <functional>
#include <utility>
#include <typedefs.hpp>
#include <thread_pool.hpp>


namespace MCPP {


	/**
	 *	Binds sockets to one or more binds and
	 *	listens on those binds, handling connections
	 *	as they come.
	 */
	class ListenHandler {
	
	
		public:
		
		
			/**
			 *	The type of the callback which shall be
			 *	invoked on a successful connection.
			 */
			typedef std::function<void (Socket, IPAddress, UInt16)> ConnectType;
			/**
			 *	The type of the callback which shall be
			 *	called each time a new connection is formed,
			 *	and which shall filter those connections.
			 */
			typedef std::function<bool (IPAddress, UInt16)> OnConnectType;
	
	
		private:
		
		
			String desc;
			Vector<Socket> sockets;
			ThreadPool * pool;
			OnConnectType on_connect;
			ConnectType connect;
			LogType log;
			PanicType panic;
			
			
			//	Thread control
			Nullable<Thread> thread;
			volatile bool stop;
			
			
			//	Pending tasks
			Vector<SmartPointer<ThreadPoolHandle>> pending;
			
			
			static void thread_func (void *);
			void thread_func_impl ();
		
		
		public:
		
		
			/**
			 *	Thrown from the ListenHandler constructor
			 *	when no binds were supplied or when none
			 *	of the binds supplied could be bound to.
			 */
			class NoBindsException : public std::exception {	};
		
		
			ListenHandler () = delete;
			ListenHandler (const ListenHandler &) = delete;
			ListenHandler (ListenHandler &&) = delete;
			ListenHandler & operator = (const ListenHandler &) = delete;
			ListenHandler & operator = (ListenHandler &&) = delete;
			
			
			/**
			 *	Creates and starts a new connection
			 *	handler.
			 *
			 *	\param [in] desc
			 *		A description of this handler to be
			 *		used for logging purposes.
			 *	\param [in] binds
			 *		A vector of IP addresses and ports
			 *		that shall be bound to.  If any
			 *		of these cannot be bound it shall
			 *		be logged.  If none can be bound
			 *		an exception shall be thrown and
			 *		it shall be logged.
			 *	\param [in] pool
			 *		A thread pool to use for asynchronous
			 *		processing.
			 *	\param [in] on_connect
			 *		An event that shall fire whenever
			 *		a new connection is formed.  If this
			 *		event shall return \em false the
			 *		connection shall be immediately
			 *		closed.
			 *	\param [in] connect
			 *		A function that shall be called to
			 *		handle each incoming connection which
			 *		is approved by \em on_connect.
			 *	\param [in] log
			 *		A callback that shall be used to
			 *		log events.
			 *	\param [in] panic
			 *		A function to call if something
			 *		catastrophic occurs and the handler
			 *		has to unexpectedly shut down.
			 */
			ListenHandler (
				String desc,
				const Vector<Tuple<IPAddress,UInt16>> & binds,
				ThreadPool & pool,
				OnConnectType on_connect,
				ConnectType connect,
				LogType log,
				PanicType panic
			);
			
			
			/**
			 *	Shuts down and cleans up the connection
			 *	handler.
			 */
			~ListenHandler () noexcept;
	
	
	};


}

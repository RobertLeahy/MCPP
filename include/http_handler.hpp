/**
 *	\file
 */
 

#pragma once


#include <rleahylib/rleahylib.hpp>
#include <curl/curl.h>
#include <functional>
#include <unordered_map>


namespace MCPP {


	/**
	 *	The type of callback that shall be invoked
	 *	when an HTTP request completes.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	0.	HTTP status code of the HTTP request,
	 *		or zero if the request failed.
	 */
	typedef std::function<void (Word)> HTTPStatusDone;
	/**
	 *	The type of callback that shall be invoked
	 *	when an HTTP request completes.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	0.	HTTP status code of the HTTP request,
	 *		or zero if the request failed.
	 *	1.	Content of the HTTP request.  Ignore this
	 *		parameter if the status code is zero.
	 */
	typedef std::function<void (Word, String)> HTTPStatusStringDone;
	/**
	 *	The type of callback that shall be invoked
	 *	when an HTTP request has incoming data
	 *	pending.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	0.	A buffer of bytes containing the pending
	 *		incoming data.
	 *	1.	The number of bytes in the buffer.
	 *
	 *	<B>Return Value:</B>
	 *
	 *	The number of bytes actually processed.  Returning
	 *	a value other than the number of bytes in the input
	 *	buffer is an error condition.
	 */
	typedef std::function<Word (const Byte *, Word)> HTTPWrite;
	/**
	 *	The type of callback that shall be invoked
	 *	when an HTTP request is awaiting outgoing data.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	0.	A buffer from which data shall be read.
	 *	1.	The size of the buffer.
	 *
	 *	<B>Return Value:</B>
	 *
	 *	The number of bytes queued to be sent over the HTTP
	 *	connection.  Zero signals end of file and the callback
	 *	shall not be invoked again.
	 */
	typedef std::function<Word (Byte *, Word)> HTTPRead;
	/**
	 *	The type of callback that shall be invoked for each
	 *	header in the HTTP response.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	0.	The name of the header.
	 *	1.	The value of the header.
	 */
	typedef std::function<void (const String &, const String &)> HTTPHeader;


	/**
	 *	\cond
	 */
	 
	 
	class HTTPRequest {
	
	
		private:
		
		
			SmartPointer<Encoding> encoding;
			HTTPStatusDone status_done;
			HTTPStatusStringDone status_string_done;
			HTTPWrite write;
			HTTPRead read;
			HTTPHeader header;
			Vector<Byte> buffer;
			Word max_bytes;
			CURL * handle;
			Vector<Byte> url;
	
	
		public:
		
		
			HTTPRequest () = delete;
			HTTPRequest (const HTTPRequest &) = delete;
			HTTPRequest (HTTPRequest &&) = delete;
			HTTPRequest & operator = (const HTTPRequest &) = delete;
			HTTPRequest & operator = (HTTPRequest &) = delete;
		
		
			HTTPRequest (CURL * handle, Word max_bytes, const String & url, HTTPStatusStringDone status_string_done);
			~HTTPRequest () noexcept;
		
		
			Word Write (const Byte * ptr, Word size) noexcept;
			Word Read (Byte * ptr, Word size) noexcept;
			void Done (Word status_code) noexcept;
			bool Header (const String & key, const String & value) noexcept;
			const Vector<Byte> & URL () const noexcept;
	
	
	};
	 
	 
	/**
	 *	\endcond
	 */


	/**
	 *	Provides a facility for asynchronously
	 *	making and handling HTTP and HTTP
	 *	requests.
	 */
	class HTTPHandler {
	
	
		private:
		
		
			//	Worker
			Thread thread;
			
			
			//	Worker control
			
			//	Tells worker to stop
			bool stop;
			//	Locks the list
			Mutex mutex;
			//	Wakes the worker up
			//	if the worker was previously
			//	doing nothing
			CondVar condvar;
			
			
			//	Request list
			std::unordered_map<CURL *, SmartPointer<HTTPRequest>> requests;
			
			
			//	cURL multi handle
			CURLM * multi;
			
			
			//	Maximum number of buffered bytes
			Word max_bytes;
			
			
			//	Worker functions
			static void worker_thread (void *);
			void worker_thread_impl ();
		
		
		public:
		
		
			HTTPHandler (const HTTPHandler &) = delete;
			HTTPHandler (HTTPHandler &&) = delete;
			HTTPHandler & operator = (const HTTPHandler &) = delete;
			HTTPHandler & operator = (HTTPHandler &&) = delete;
		
		
			/**
			 *	Starts up the HTTP handler.
			 *
			 *	\param [in] max_bytes
			 *		The number of bytes the handler
			 *		shall be willing to buffer on a
			 *		given connection before it
			 *		terminates the connection.  Set
			 *		to zero for unlimited.  Defaults
			 *		to zero.
			 */
			HTTPHandler (Word max_bytes=0);
			/**
			 *	Shuts down the HTTP handler.
			 */
			~HTTPHandler () noexcept;
			
			
			/**
			 *	Stops the handler, stalling all
			 *	connections and preventing any
			 *	further connections from being
			 *	formed, but keeping its resources
			 *	intact.
			 *
			 *	Returns when the handler's internal
			 *	worker thread has ended.
			 */
			void Stop () noexcept;
			
			
			
			/**
			 *	Executes an HTTP GET request.
			 *
			 *	\param [in] url
			 *		The URL to submit the request to,
			 *		complete with query string.
			 *	\param [in] callback
			 *		A callback that shall be invoked
			 *		when the request completes.
			 */
			void Get (
				const String & url,
				HTTPStatusStringDone callback
			);
	
	
	};


}
 
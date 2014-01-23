/**
 *	\file
 */
 

#pragma once


#include <rleahylib/rleahylib.hpp>
#include <promise.hpp>
#include <socketpair.hpp>
#include <curl/curl.h>
#include <cstddef>
#include <exception>
#include <memory>
#include <unordered_map>


namespace MCPP {


	/**
	 *	A single HTTP request or response
	 *	header.
	 */
	class HTTPHeader {
	
	
		public:
		
		
			/**
			 *	The key associated with this
			 *	header.
			 */
			String Key;
			/**
			 *	The value associated with this
			 *	header.
			 */
			String Value;
	
	
	};
	
	
	/**
	 *	A single HTTP cookie.
	 */
	class HTTPCookie {
	
	
		public:
		
		
			/**
			 *	The key associated with this
			 *	cookie.
			 */
			String Key;
			/**
			 *	The value associated with this
			 *	cookie.
			 */
			String Value;
	
	
	};
	
	
	/**
	 *	The response to an HTTP request.
	 */
	class HTTPResponse {
	
	
		public:
		
		
			/**
			 *	The numerical status code associated
			 *	with this response.
			 */
			Word Status;
			/**
			 *	HTTP major version number.
			 */
			Word MajorVersion;
			/**
			 *	HTTP minor version number.
			 */
			Word MinorVersion;
			/**
			 *	Server's response.
			 */
			String Response;
			/**
			 *	The headers that were sent in the
			 *	response.
			 */
			Vector<HTTPHeader> Headers;
			/**
			 *	Raw bytes representing the body of the
			 *	response.
			 */
			Vector<Byte> Body;
			
			
			/**
			 *	Attempts to retrieve the body of the
			 *	response as a string.
			 *
			 *	Scans the headers looking for a character
			 *	set.  If a character set cannot be found,
			 *	falls back to UTF-8.
			 *
			 *	\return
			 *		A string representing the body of this
			 *		response.
			 */
			String GetBody () const;
	
	
	};
	
	
	/**
	 *	HTTP verbs.
	 */
	enum class HTTPVerb {
	
		GET,
		POST,
		HEAD
	
	};
	
	
	/**
	 *	An HTTP request.
	 */
	class HTTPRequest {
	
		
		public:
		
		
			/**
			 *	Creates a new GET request.
			 */
			HTTPRequest () noexcept;
		
		
			/**
			 *	Creates a new GET request with default
			 *	settings for the specified URL.
			 *
			 *	\param [in] url
			 *		The URL to which to send this request.
			 */
			HTTPRequest (String url) noexcept;
		
		
			/**
			 *	The URL to which this request shall be
			 *	sent.
			 */
			String URL;
			/**
			 *	The HTTP verb that shall be invoked.
			 */
			HTTPVerb Verb;
			/**
			 *	A collection of headers to send.
			 */
			Vector<HTTPHeader> Headers;
			/**
			 *	The body of the request.  Only relevant
			 *	for verbs with a body.
			 */
			Vector<Byte> Body;
			
			
			/**
			 *	The referer.
			 */
			Nullable<String> Referer;
			/**
			 *	The user agent string.
			 */
			Nullable<String> UserAgent;
			/**
			 *	A collection of cookies to be sent.
			 */
			Vector<HTTPCookie> Cookies;
			
			
			/**
			 *	Whether this request requires SSL or
			 *	TLS.
			 */
			bool RequireSSL;
			/**
			 *	The maximum number of bytes which will
			 *	be accepted as part of the header before
			 *	the request will be terminated.
			 */
			Nullable<Word> MaxHeaderBytes;
			/**
			 *	The maximum number of bytes which will be
			 *	accepted as part of the body before the
			 *	request will be terminated.
			 */
			Nullable<Word> MaxBodyBytes;
			/**
			 *	Whether redirects should be followed.
			 */
			bool FollowRedirects;
			/**
			 *	If redirects are to be followed, how many
			 *	should be followed.  Null means an
			 *	unlimited number.
			 */
			Nullable<Word> MaxRedirects;
		
	
	};
	
	
	/**
	 *	An exception thrown to indicate that the HTTPHandler
	 *	shut down during the execution of a request.
	 */
	class HTTPHandlerError : public std::exception {
	
	
		public:
		
		
			virtual const char * what () const noexcept override;
	
	
	};


	/**
	 *	Provides asynchronous HTTP handling mechanisms.
	 */
	class HTTPHandler {
	
	
		public:
		
		
			typedef std::function<void (std::exception_ptr)> PanicType;
	
	
		private:
		
		
			class Request {
			
			
				private:
				
				
					CURL * handle;
				
				
					Nullable<Promise<HTTPResponse>> promise;
					
					
					HTTPResponse response;
					
					
					//	The original request
					HTTPRequest request;
					
					
					//	Encoded strings for legacy cURL
					Vector<char> url;
					Vector<char> referer;
					Vector<char> uas;
					Vector<char> cookies;
					
					
					//	Values that are actually relevant
					//	to the consumer
					Vector<Byte> request_body;
					Word headers_curr;
					std::exception_ptr ex;
					
					
					template <typename... Args>
					void set (CURLoption, Args &&...);
					
					
					static std::size_t header (void *, std::size_t, std::size_t, void *) noexcept;
					static std::size_t body (char *, std::size_t, std::size_t, void *) noexcept;
			
			
				public:
				
				
					Request (HTTPRequest);
					~Request () noexcept;
				
				
					void Complete (CURLcode) noexcept;
					
					
					Word Header (const void *, Word) noexcept;
					Word ResponseBody (const void *, Word) noexcept;
					
					
					Tuple<CURL *,Promise<HTTPResponse>> Get () const noexcept;
				
			
			};
		
		
			//	Control
			ControlSocket control;
			
			
			//	Multi handle
			CURLM * handle;
		
		
			//	Worker thread
			Thread thread;
			
			
			//	Pending requests and shutdown
			//	synchronization
			mutable Mutex lock;
			mutable CondVar wait;
			bool stop;
			std::unordered_map<
				CURL *,
				std::unique_ptr<Request>
			> requests;
			
			
			//	Called on panic
			PanicType panic;
			
			
			//	Panics
			[[noreturn]]
			void do_panic () const noexcept;
			
			
			//	Determines -- based on control socket --
			//	whether worker should stop
			bool should_stop ();
			
			
			//	Worker functions
			void worker_func () noexcept;
			void worker ();
		
		
		public:
		
		
			/**
			 *	Creates and starts a new HTTPHandler.
			 *
			 *	\param [in] panic
			 *		A callback which will be invoked when
			 *		and if something goes wrong internal
			 *		to the handler.  Optional.  Default is
			 *		to call std::abort.
			 */
			HTTPHandler (PanicType panic=PanicType{});
			/**
			 *	Shuts down and destroys an HTTPHandler.
			 */
			~HTTPHandler () noexcept;
			
			
			/**
			 *	Asynchronously performs an HTTP request.
			 *
			 *	\param [in] request
			 *		The request to perform.
			 *
			 *	\return
			 *		A promise of a future HTTP response.
			 */
			Promise<HTTPResponse> Execute (HTTPRequest request);
	
	
	};


}
 
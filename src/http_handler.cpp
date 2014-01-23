#include <http_handler.hpp>
#include <safeint.hpp>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <system_error>
#ifdef ENVIRONMENT_WINDOWS
#include <windows.h>
#else
#include <sys/time.h>
#endif


namespace MCPP {


	//	Generates cURL exceptions
	static std::runtime_error get_exception (CURLcode code) noexcept {
	
		return std::runtime_error(
			curl_easy_strerror(code)
		);
	
	}


	//	Throws cURL errors
	[[noreturn]]
	static void raise (CURLcode code) {
	
		throw get_exception(code);
	
	}
	
	
	//	Throws cURL multi errors
	[[noreturn]]
	static void raise_multi (CURLMcode code) {
	
		throw std::runtime_error(
			curl_multi_strerror(code)
		);
	
	}
	
	
	//	Throws OS socket errors
	[[noreturn]]
	static void raise_os () {
	
		throw std::system_error(
			std::error_code(
				#ifdef ENVIRONMENT_WINDOWS
				WSAGetLastError()
				#else
				errno
				#endif
				,
				std::system_category()
			)
		);
	
	}


	//	Kludge to RAII init/free cURL without risking
	//	being thread unsafe
	class cURLGuard {


		private:
		
		
			CURLcode result;


		public:
		
		
			cURLGuard () noexcept {
				
				//	Initialize cURL
				result=curl_global_init(CURL_GLOBAL_ALL);
			
			}
			
			
			~cURLGuard () noexcept {
			
				//	If cURL was initialized, kill it
				if (result==0) curl_global_cleanup();
			
			}
			
			
			//	Verifies that cURL is good to use
			//
			//	Throws if it's not
			void Okay () const {
			
				if (result!=0) raise(result);
			
			}


	};
	
	
	//	This single instance of cURLGuard ensures
	//	that cURL is started up
	static const cURLGuard libcurl;
	
	
	static const String content_type_header("content-type");
	static const Regex charset(
		"(?:^|\\s)charset\\=(\\S+)(?:$|\\s)",
		RegexOptions().SetIgnoreCase()
	);
	
	
	String HTTPResponse::GetBody () const {
	
		SmartPointer<Encoding> encoder;
		
		//	Attempt to find a content type header
		for (auto & header : Headers) if (header.Key.ToLower()==content_type_header) {
		
			//	Attempt to extract a character set
			auto match=charset.Match(header.Value);
			
			//	Attempt to get an encoder for that
			//	character set
			if (match.Success()) {
			
				encoder=Encoding::Get(match[1].Value());
				
				if (!encoder.IsNull()) break;
			
			}
		
		}
		
		auto begin=Body.begin();
		auto end=Body.end();
		//	If no encoder could be found, fall back
		//	to UTF-8
		return (encoder.IsNull()) ? UTF8().Decode(begin,end) : encoder->Decode(begin,end);
	
	}
	
	
	HTTPRequest::HTTPRequest () noexcept
		:	Verb(HTTPVerb::GET),
			RequireSSL(false),
			FollowRedirects(true)
	{	}
	
	
	HTTPRequest::HTTPRequest (String url) noexcept : HTTPRequest() {
	
		URL=std::move(url);
	
	}
	
	
	static const char * http_handler_error="HTTPHandler shut down";
	
	
	const char * HTTPHandlerError::what () const noexcept {
	
		return http_handler_error;
	
	}
	
	
	template <typename... Args>
	void HTTPHandler::Request::set (CURLoption option, Args &&... args) {
	
		auto result=curl_easy_setopt(
			handle,
			option,
			std::forward<Args>(args)...
		);
		if (result!=CURLE_OK) raise(result);
	
	}
	
	
	std::size_t HTTPHandler::Request::header (void * ptr, std::size_t size, std::size_t nmemb, void * userdata) noexcept {
	
		return reinterpret_cast<Request *>(userdata)->Header(
			ptr,
			size*nmemb
		);
	
	}
	
	
	std::size_t HTTPHandler::Request::body (char * ptr, std::size_t size, std::size_t nmemb, void * userdata) noexcept {
	
		return reinterpret_cast<Request *>(userdata)->ResponseBody(
			ptr,
			size*nmemb
		);
	
	}
	
	
	static const String http_header_template("{0}:{1}");
	static const char * slist_error("Error calling curl_slist_append");
	
	
	static struct curl_slist * get_headers (const Vector<HTTPHeader> & headers) {
	
		struct curl_slist * list=nullptr;
		
		try {
		
			for (const auto & header : headers) {
			
				//	Get the C string
				auto c_string=String::Format(
					http_header_template,
					header.Key,
					header.Value
				).ToCString();
				
				//	Add it
				if ((list=curl_slist_append(
					list,
					c_string.begin()
				))==nullptr) throw std::runtime_error(slist_error);
			
			}
			
		} catch (...) {
		
			if (list!=nullptr) curl_slist_free_all(list);
			
			throw;
		
		}
		
		return list;
	
	}
	
	
	static const String http_cookie_template("{0}={1}");
	static const String http_cookie_separator(";");
	
	
	static Vector<char> get_cookies (const Vector<HTTPCookie> & cookies) {
	
		String str;
		
		bool first=true;
		for (const auto & cookie : cookies) {
		
			if (first) first=false;
			else str << http_cookie_separator;
			
			str << String::Format(
				http_cookie_template,
				cookie.Key,
				cookie.Value
			);
		
		}
		
		return str.ToCString();
	
	}
	
	
	static const char * curl_easy_init_error="Could not create a cURL easy handle";
	
	
	HTTPHandler::Request::Request (HTTPRequest request_in)
		:	promise(Promise<HTTPResponse>{}),
			request(std::move(request_in)),
			headers_curr(0)
	{
		
		//	Encode the URL for cURL
		url=request.URL.ToCString();
		
		//	Get a cURL handle
		if ((handle=curl_easy_init())==nullptr) throw std::runtime_error(curl_easy_init_error);
		
		try {
		
			#ifdef LIBCURL_INSECURE
			set(CURLOPT_SSL_VERIFYPEER,static_cast<long>(0));
			#endif
		
			//
			//	URL
			//
			set(CURLOPT_URL,url.begin());
			
			//
			//	Verb & Body
			//
			switch (request.Verb) {
			
				//	Ignore -- no action is needed
				case HTTPVerb::GET:
				default:break;
				
				case HTTPVerb::POST:
				
					//	Tell cURL that this is a POST
					set(CURLOPT_POST,static_cast<long>(1));
					
					//	Tell cURL how long the data is going
					//	to be
					set(CURLOPT_POSTFIELDSIZE,safe_cast<long>(request.Body.Count()));
					
					//	Give cURL a pointer to the post field
					set(CURLOPT_POSTFIELDS,request.Body.begin());
					
					break;
					
					
				case HTTPVerb::HEAD:
				
					set(CURLOPT_NOBODY,static_cast<long>(1));
					
					break;
			
			}
			
			//
			//	Headers
			//
			auto headers=get_headers(request.Headers);
			if (headers!=nullptr) set(CURLOPT_HTTPHEADER,headers);
			
			//
			//	Referer
			//
			if (!request.Referer.IsNull()) {
			
				//	Encode
				referer=request.Referer->ToCString();
				
				//	Pass to cURL
				set(CURLOPT_REFERER,referer.begin());
			
			}
			
			//
			//	User Agent String
			//
			if (!request.UserAgent.IsNull()) {
			
				//	Encode
				uas=request.UserAgent->ToCString();
				
				//	Pass to cURL
				set(CURLOPT_USERAGENT,uas.begin());
			
			}
			
			//
			//	Cookies
			//
			cookies=get_cookies(request.Cookies);
			if (cookies.Count()!=0) set(CURLOPT_COOKIE,cookies.begin());
			
			//
			//	Protocols?  Require SSL?
			//
			long protos=CURLPROTO_HTTPS;
			if (!request.RequireSSL) protos|=CURLPROTO_HTTP;
			set(CURLOPT_PROTOCOLS,protos);
			
			//
			//	Follow redirects?  How many?
			//
			if (request.FollowRedirects) {
			
				long follow_redirects=1;
				set(CURLOPT_FOLLOWLOCATION,follow_redirects);
				
				if (!request.MaxRedirects.IsNull()) {
				
					auto max_redirects=safe_cast<long>(*request.MaxRedirects);
					set(CURLOPT_MAXREDIRS,max_redirects);
				
				}
			
			}
			
			//
			//	Write & Header Functions
			//
			
			set(CURLOPT_WRITEFUNCTION,&body);
			set(CURLOPT_WRITEDATA,this);
			
			set(CURLOPT_HEADERFUNCTION,&header);
			set(CURLOPT_HEADERDATA,this);
			
		} catch (...) {
		
			//	Make sure the easy handle gets
			//	destroyed
			curl_easy_cleanup(handle);
			
			throw;
		
		}
	
	}
	
	
	HTTPHandler::Request::~Request () noexcept {
	
		//	Cleanup handle
		curl_easy_cleanup(handle);
		
		//	If promise isn't already dead,
		//	kill it.
		if (!promise.IsNull()) promise->Fail(
			std::make_exception_ptr(
				HTTPHandlerError{}
			)
		);
	
	}
	
	
	void HTTPHandler::Request::Complete (CURLcode code) noexcept {
	
		//	If an exception has been thrown
		//	internally, complete with that
		if (ex) {
		
			promise->Fail(std::move(ex));
			promise.Destroy();
			
			return;
		
		}
		
		//	If code passed in indicates failure,
		//	complete with that
		if (code!=CURLE_OK) {
		
			promise->Fail(
				std::make_exception_ptr(
					get_exception(code)
				)
			);
			promise.Destroy();
			
			return;
		
		}
	
		try {
			
			//	Get the status code
			long status;
			auto result=curl_easy_getinfo(
				handle,
				CURLINFO_RESPONSE_CODE,
				&status
			);
			if (result!=CURLE_OK) raise(result);
			response.Status=safe_cast<Word>(status);
		
		} catch (...) {
		
			//	Complete the promise
			promise->Fail(std::current_exception());
			promise.Destroy();
			
			return;
		
		}
		
		//	Complete the promise
		promise->Complete(std::move(response));
		promise.Destroy();
	
	}
	
	
	static const char header_delimiter=':';
	static const Regex banner("^HTTP\\/(\\d+)\\.(\\d+)\\s+(?:\\d+)\\s+(.+)$");
	
	
	static const char * too_many_header_bytes="Headers were longer than specified maximum";
	static const char * malformed_version="Server sent malformed HTTP version";
	
	
	Word HTTPHandler::Request::Header (const void * ptr, Word len) noexcept {
	
		//	Ignore zero length headers
		if (len==0) return 0;
	
		try {
		
			auto begin=reinterpret_cast<const Byte *>(ptr);
			auto end=reinterpret_cast<const Byte *>(ptr)+len;
			//	Find the first header delimiter
			auto end_key=std::find(begin,end,header_delimiter);
			auto begin_value=(end_key==end) ? end : (end_key+1);
			
			//	UTF8 decoder
			UTF8 decoder;
			
			//	Decode
			auto key=decoder.Decode(begin,end_key).Trim();
			auto value=decoder.Decode(begin_value,end).Trim();
			
			//	If this is the beginning of a new
			//	request, clear all other headers
			//	as they're irrelevant
			bool ignore=false;
			if (value.Size()==0) {
			
				auto match=banner.Match(key);
				
				if (match.Success()) {
				
					ignore=true;
					
					headers_curr=0;
					response.Headers.Clear();
					
					if (!(
						match[1].Value().ToInteger(&response.MajorVersion) &&
						match[2].Value().ToInteger(&response.MinorVersion)
					)) throw std::runtime_error(malformed_version);
					
					response.Response=match[3].Value();
				
				}
			
			}
			
			//	If this header is ignored, or
			//	empty, ignore it
			if (!(
				ignore ||
				(
					(value.Size()==0) &&
					(key.Size()==0)
				)
			)) {
			
				//	Check length if applicable
				if (!request.MaxHeaderBytes.IsNull()) {
				
					headers_curr=Word(SafeWord(headers_curr)+SafeWord(len));
					//	If too long, throw
					if (headers_curr>*request.MaxHeaderBytes) throw std::runtime_error(
						too_many_header_bytes
					);
				
				}
			
				response.Headers.Add(
					HTTPHeader{
						std::move(key),
						std::move(value)
					}
				);
				
			}
		
		} catch (...) {
		
			//	Store exception for pass through
			ex=std::current_exception();
			
			//	Return something different than
			//	what we were given to signal an
			//	error to cURL
			//
			//	We can safely return zero -- certain
			//	that it will be different from everything
			//	because we just ignore zero lengths
			return 0;
		
		}
		
		//	Return exactly what we were given to
		//	signal success to cURL
		return len;
	
	}
	
	
	static const char * too_many_body_bytes="Body was longer than specified maximum";
	
	
	Word HTTPHandler::Request::ResponseBody (const void * ptr, Word len) noexcept {
	
		//	If there's nothing, ignore
		if (len==0) return 0;
	
		try {
		
			//	Determine how much capacity we're going to need
			Word after=static_cast<Word>(
				MakeSafe(len)+MakeSafe(
					response.Body.Count()
				)
			);
			
			//	Check to make sure this isn't more than
			//	the maximum
			if (
				!request.MaxBodyBytes.IsNull() &&
				(after>*request.MaxBodyBytes)
			) throw std::runtime_error(too_many_body_bytes);
			
			//	Resize if necessary
			if (response.Body.Capacity()<after) response.Body.SetCapacity(after);
			
			//	Copy all the data
			std::memcpy(
				response.Body.end(),
				ptr,
				len
			);
			
			//	Set count
			response.Body.SetCount(after);
		
		} catch (...) {
		
			//	Store exception for pass through
			ex=std::current_exception();
			
			//	Return something different than
			//	what we were given to signal an error
			//	to cURL
			//
			//	We can safely return zero because
			//	zero lengths are simply ignored
			return 0;
		
		}
		
		//	Return exactly what we were given to
		//	signal success to cURL
		return len;
	
	}
	
	
	Tuple<CURL *,Promise<HTTPResponse>> HTTPHandler::Request::Get () const noexcept {
	
		return Tuple<CURL *,Promise<HTTPResponse>>(
			handle,
			*promise
		);
	
	}
	
	
	void HTTPHandler::do_panic () const noexcept {
	
		if (panic) try {
		
			panic(std::current_exception());
		
		} catch (...) {	}
		
		//	If the panic callback returned, or
		//	there wasn't one, there's not a lot
		//	that we can do -- we're only calling
		//	this because the state is corrupted,
		//	traumatically unexpected, et cetera
		std::abort();
	
	}
	
	
	bool HTTPHandler::should_stop () {
	
		for (;;) {
		
			auto msg=control.Receive<bool>();
			
			if (msg.IsNull()) return false;
			
			if (*msg) return true;
		
		}
	
	}
	
	
	void HTTPHandler::worker_func () noexcept {
	
		try {
		
			worker();
			
		} catch (...) {
		
			do_panic();
		
		}
	
	}
	
	
	//	Default timeout when libcurl doesn't
	//	supply one -- 5 seconds
	//
	//	libcurl documentation says "a few
	//	seconds"
	static const long default_timeout=5000;
	
	
	void HTTPHandler::worker () {
	
		//	FD sets, nfds, and timeout for select
		fd_set readable;
		fd_set writeable;
		fd_set exceptional;
		int nfds;
		long timeout;
	
		//	Loop forever, or until told
		//	to stop
		for (;;) {
		
			//	Zero FD sets
			FD_ZERO(&readable);
			FD_ZERO(&writeable);
			FD_ZERO(&exceptional);
			
			//	Lock and check
			if (lock.Execute([&] () mutable {
			
				//	Wait for there to be something to do
				while (!stop && (requests.size()==0)) wait.Sleep(lock);
				
				//	Were we commanded to stop?
				if (stop) return true;
				
				//	Get FD sets and timeout from libcurl
				auto result=curl_multi_fdset(
					handle,
					&readable,
					&writeable,
					&exceptional,
					&nfds
				);
				if (result!=CURLM_OK) raise_multi(result);
				result=curl_multi_timeout(
					handle,
					&timeout
				);
				if (result!=CURLM_OK) raise_multi(result);
				
				return false;
			
			})) break;
			
			//	How we proceed depends on the timeout
			//	value.  If it's 0 we proceed at once
			if (timeout!=0) {
			
				//	If the timeout is -1, we choose
				//	some other reasonable timeout
				if (timeout==-1) timeout=default_timeout;
				
				//	Get a struct timeval that
				//	corresponds to the number of
				//	milliseconds returned by libcurl
				struct timeval tv;
				tv.tv_sec=timeout/1000;
				tv.tv_usec=(timeout%1000)*1000;
				
				//	Wait also on the control socket
				nfds=control.Add(readable,nfds);
				
				//	Select
				if (select(
					nfds,
					&readable,
					&writeable,
					&exceptional,
					&tv
				)==
				#ifdef ENVIRONMENT_WINDOWS
				SOCKET_ERROR
				#else
				-1
				#endif
				) raise_os();
			
			}
			
			//	If the control socket became readable,
			//	and a shutdown command was given, shutdown
			if (control.Is(readable) && should_stop()) break;
			
			//	Invoke libcurl
			lock.Execute([&] () mutable {
			
				//	Loop until cURL no longer wants to be
				//	called
				for (;;) {
				
					int running;	//	Ignored
					auto result=curl_multi_perform(
						handle,
						&running
					);
					//	If cURL wants to be called again, LOOP
					if (result==CURLM_CALL_MULTI_PERFORM) continue;
					//	If all's good, break out of the loop
					if (result==CURLM_OK) break;
					//	Otherwise an error occurred, throw
					raise_multi(result);
				
				}
				
				//	Pop all messages off cURL's stack
				for (;;) {
				
					int msgs;	//	Ignored
					auto msg=curl_multi_info_read(
						handle,
						&msgs
					);
					//	End when no more messages to read
					if (msg==nullptr) break;
					//	CURLMSG_DONE is the only message defined
					//	as of this writing, but more may be added
					//	in the future...
					if (msg->msg!=CURLMSG_DONE) continue;
					
					//	Lookup this particular easy handle and
					//	extract the request
					auto iter=requests.find(msg->easy_handle);
					auto request=std::move(iter->second);
					requests.erase(iter);
					//	Save the result of the completion
					auto code=msg->data.result;
					//	Remove from multi handle
					auto result=curl_multi_remove_handle(
						handle,
						msg->easy_handle
					);
					if (result!=CURLM_OK) raise_multi(result);
					
					//	Complete request
					request->Complete(code);
				
				}
			
			});
		
		}
	
	}
	
	
	static const char * curl_multi_init_error="Could not create a cURL multi handle";
	
	
	HTTPHandler::HTTPHandler (PanicType panic) : stop(false), panic(std::move(panic)) {
	
		//	Make sure libcurl was initialized
		//	properly
		libcurl.Okay();
	
		//	Attempt to create a multi handle
		if ((handle=curl_multi_init())==nullptr) throw std::runtime_error(
			curl_multi_init_error
		);
		
		//	We're managing a multi handle now
		try {
		
			//	Start our worker
			thread=Thread([this] () mutable {	worker_func();	});
		
		} catch (...) {
		
			auto result=curl_multi_cleanup(handle);
			//	Not really much sane we can do about
			//	this
			if (result!=CURLM_OK) std::abort();
			
			throw;
		
		}
	
	}
	
	
	HTTPHandler::~HTTPHandler () noexcept {
	
		try {
		
			//	Kill the worker
			
			control.Send(true);
			lock.Execute([&] () mutable {
			
				stop=true;
				
				wait.WakeAll();
			
			});
			thread.Join();
			
			//	Remove all easy handles
			for (auto & pair : requests) {
			
				auto result=curl_multi_remove_handle(
					handle,
					pair.first
				);
				if (result!=CURLM_OK) raise_multi(result);
			
			}
			
			//	Kill the multi handle
			auto result=curl_multi_cleanup(handle);
			if (result!=CURLM_OK) raise_multi(result);
		
		} catch (...) {
		
			//	Nothing we can do to save
			//	ourselves
			std::abort();
		
		}
	
	}
	
	
	Promise<HTTPResponse> HTTPHandler::Execute (HTTPRequest request) {
	
		//	Create the request
		auto ptr=std::unique_ptr<Request>(new Request(std::move(request)));
		
		//	Get the data
		auto t=ptr->Get();
		
		//	Add to map and multi handle
		lock.Execute([&] () mutable {
		
			//	Add to map
			auto pair=requests.emplace(
				t.Item<0>(),
				std::move(ptr)
			);
			
			//	If anything goes awry, we need
			//	to remove the request
			try {
			
				//	Attempt to command worker to
				//	wake up
				control.Send(false);
			
				//	Attempt to add to multi handle
				auto result=curl_multi_add_handle(
					handle,
					t.Item<0>()
				);
				if (result!=CURLM_OK) raise_multi(result);
				
				//	Done adding, wake up worker
				wait.WakeAll();
			
			} catch (...) {
			
				requests.erase(pair.first);
				
				throw;
			
			}
		
		});
		
		//	Return the promise
		return std::move(t.Item<1>());
	
	}


}

#include <http_handler.hpp>
#include <new>
#include <stdexcept>
#include <system_error>
#include <utility>


namespace MCPP {


	//	Error messages
	static const char * curl_error="cURL error";
	
	
	//	Settings
	
	//	Maximum number of 3xx headers that a cURL
	//	request may follow before failing
	static const long max_redirs=5;
	
	
	static size_t write_callback (char * ptr, size_t size, size_t nmemb, void * userdata) noexcept {

		try {
		
			Word safe_size=Word(SafeWord(size)*SafeWord(nmemb));
			
			return reinterpret_cast<HTTPRequest *>(userdata)->Write(
				reinterpret_cast<Byte *>(ptr),
				safe_size
			);
		
		} catch (...) {	}

		return 0;
		
	}


	static size_t header_callback (char * ptr, size_t size, size_t nmemb, void * userdata) noexcept {

		try {
		
			Word safe_size=Word(SafeWord(size)*SafeWord(nmemb));
			
			//	Decode header
			String header(UTF8().Decode(
				reinterpret_cast<Byte *>(ptr),
				reinterpret_cast<Byte *>(ptr)+safe_size
			));
			
			//	Parse it
			String key;
			String value;
			bool found=false;
			for (const auto & gc : header) {
			
				if (found) {
				
					value << gc;
				
				} else if (gc==':') {
				
					found=true;
				
				} else {
				
					key << gc;
				
				}
			
			}
			
			//	Trim
			key.Trim();
			value.Trim();
			
			//	Only proceed if this is a "real"
			//	header
			if (found) {
			
				//	Dispatch callback
				
				if (reinterpret_cast<HTTPRequest *>(userdata)->Header(key,value)) return safe_size;
				
				return 0;
			
			}
			
			//	Succeed
			return safe_size;
		
		} catch (...) {	}
		
		//	Fail
		return 0;

	}


	static size_t read_callback (void * ptr, size_t size, size_t nmemb, void * userdata) noexcept {

		try {
		
			Word safe_size=Word(SafeWord(size)*SafeWord(nmemb));
			
			return reinterpret_cast<HTTPRequest *>(userdata)->Read(
				reinterpret_cast<Byte *>(ptr),
				safe_size
			);
		
		} catch (...) {	}
		
		//	Fail
		return 0;

	}
	
	
	void HTTPHandler::worker_thread (void * ptr) {
	
		try {
	
			reinterpret_cast<HTTPHandler *>(ptr)->worker_thread_impl();
			
		} catch (...) {
		
			//	TODO: Panic code?
		
			throw;
		
		}
	
	}
	
	
	//	Time worker will sleep if there's
	//	nothing to do or it's waiting for
	//	something
	static const Word worker_timeout=50;
	
	
	void HTTPHandler::worker_thread_impl () {
	
		fd_set read_fds;
		fd_set write_fds;
		fd_set exc_fds;
		int max_fd;
		CURLMcode result;
		struct timeval select_timeout;
		select_timeout.tv_sec=worker_timeout/1000;
		select_timeout.tv_usec=(worker_timeout%1000)*1000;
		bool wait=false;
	
		for (;;) {
		
			//	Zero out socket sets
			FD_ZERO(&read_fds);
			FD_ZERO(&write_fds);
			FD_ZERO(&exc_fds);
		
			mutex.Acquire();
			
			try {
			
				//	Don't sleep on the condvar
				//	unless there's nothing to do
				while (
					!stop &&
					(
						wait ||
						(requests.size()==0)
					)
				) {
				
					condvar.Sleep(mutex,worker_timeout);
				
					//	Reset wait flag
					wait=false;
				
				}
				
				//	Stop if necessary
				if (stop) {
				
					mutex.Release();
					
					return;
				
				}
				
				//	Populate FD sets
				if ((result=curl_multi_fdset(
					multi,
					&read_fds,
					&write_fds,
					&exc_fds,
					&max_fd
				))!=CURLM_OK) throw std::runtime_error(
					curl_multi_strerror(
						result
					)
				);
				
			} catch (...) {
			
				mutex.Release();
				
				throw;
			
			}
			
			mutex.Release();
			
			int num_socks;
			
			//	Bypass wait if cURL returned
			//	-1 for max_fd
			if (max_fd==-1) {
			
				//	Wait since we can't poll
				//	whatever libcurl wants
				//	to wait on
				Thread::Sleep(worker_timeout);
			
				goto call_curl;
				
			}
			
			//	Wait on FD sets
			num_socks=select(
				max_fd,
				&read_fds,
				&write_fds,
				&exc_fds,
				&select_timeout
			);
			
			//	ERROR
			if (num_socks<0) throw std::system_error(
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
			
			call_curl:
		
			mutex.Acquire();
			
			try {
		
				//	Call cURL until it has nothing
				//	more to do
				int running_handles;
				do {

					result=curl_multi_perform(
						multi,
						&running_handles
					);
					
				} while (result==CURLM_CALL_MULTI_PERFORM);
				
				//	Error?
				if (result!=CURLM_OK) throw std::runtime_error(
					curl_multi_strerror(
						result
					)
				);
				
				//	Convert the number of running handles
				//	to the same type as the count of
				//	open requests (the size of the map)
				decltype(requests.size()) running_handles_safe=static_cast<decltype(requests.size())>(
					SafeInt<int>(running_handles)
				);
				
				//	See if any requests finished
				if (running_handles_safe!=requests.size()) {
				
					//	At least one request finished,
					//	handle it
					CURLMsg * msg;
					int msgs;	//	Ignored
					while ((msg=curl_multi_info_read(multi,&msgs))!=nullptr) {
					
						//	Finished!
						if (msg->msg==CURLMSG_DONE) {
						
							//	Lookup this handle
							HTTPRequest * request=requests[msg->easy_handle].get();
							
							//	Dispatch callback as appropriate
							if (msg->data.result!=CURLE_OK) {
							
								//	FAIL
								
								request->Done(
									0,
									String(
										curl_easy_strerror(
											msg->data.result
										)
									)
								);
								
							} else {
							
								//	Lookup HTTP status code
								long status_code;
								CURLcode result=curl_easy_getinfo(
									msg->easy_handle,
									CURLINFO_RESPONSE_CODE,
									&status_code
								);
								
								if (result!=CURLE_OK) throw std::runtime_error(
									curl_easy_strerror(
										result
									)
								);
								
								Word safe_status_code=Word(SafeInt<long>(status_code));
								
								//	Dispatch
								request->Done(safe_status_code);
							
							}
							
							//	Save handle so it isn't lost when
							//	cURL cleans up the request
							auto easy_handle=msg->easy_handle;
							
							//	Remove handle
							if ((result=curl_multi_remove_handle(
								multi,
								msg->easy_handle
							))!=CURLM_OK) throw std::runtime_error(
								curl_multi_strerror(
									result
								)
							);
							
							//	Kill the handle
							requests.erase(easy_handle);
						
						}
					
					}
				
				}
				
			} catch (...) {
			
				mutex.Release();
				
				throw;
			
			}
			
			mutex.Release();
		
		}
	
	}


	HTTPHandler::HTTPHandler (Word max_bytes) : stop(false), max_bytes(max_bytes) {
	
		//	Initialize handle and set settings
		if (
			(curl_global_init(CURL_GLOBAL_ALL)!=0) ||
			((multi=curl_multi_init())==nullptr)
		) throw std::runtime_error(curl_error);
		
		#ifdef CURLMOPT_PIPELINING
		CURLMcode result;
		if ((result=curl_multi_setopt(
			multi,
			CURLMOPT_PIPELINING,
			static_cast<long>(0)
		))!=CURLM_OK) throw std::runtime_error(
			curl_multi_strerror(
				result
			)
		);
		#endif
		
		try {
		
			//	Launch worker
			thread=Thread(worker_thread,this);
		
		} catch (...) {
		
			curl_multi_cleanup(multi);
			
			throw;
		
		}
	
	}
	
	
	HTTPHandler::~HTTPHandler () noexcept {
	
		//	Stop
		
		Stop();
		
		//	Cleanup
		
		//	Remove all easy handles
		//	from the multi handle
		for (const auto & pair : requests) {
		
			curl_multi_remove_handle(
				multi,
				const_cast<CURL *>(
					pair.first
				)
			);
		
		}
		
		//	Clear the list of
		//	requests so that all
		//	easy handles are cleaned
		//	up
		requests.clear();
		
		//	Clean up the multi handle
		curl_multi_cleanup(multi);
	
	}
	
	
	void HTTPHandler::Stop () noexcept {
	
		//	Order worker to stop
		mutex.Acquire();
		stop=true;
		condvar.WakeAll();
		mutex.Release();
		
		//	Wait for working to stop
		thread.Join();
	
	}
	
	
	static inline void common_opts (CURL * curl, const std::unique_ptr<HTTPRequest> & request) {
		
		//	Apply settings
		CURLcode result;
		if (
			//	Set URL
			((result=curl_easy_setopt(
				curl,
				CURLOPT_URL,
				reinterpret_cast<const char *>(
					static_cast<const Byte *>(
						request->URL()
					)
				)
			))!=CURLE_OK) ||
			//	Only use HTTP and HTTPS
			((result=curl_easy_setopt(
				curl,
				CURLOPT_PROTOCOLS,
				static_cast<long>(
					CURLPROTO_HTTP|CURLPROTO_HTTPS
				)
			))!=CURLE_OK) ||
			//	Call the write callback
			//	to handle incoming data
			((result=curl_easy_setopt(
				curl,
				CURLOPT_WRITEFUNCTION,
				&write_callback
			))!=CURLE_OK) ||
			//	Pass the HTTPRequest object
			//	through to the callback
			((result=curl_easy_setopt(
				curl,
				CURLOPT_WRITEDATA,
				request.get()
			))!=CURLE_OK) ||
			//	Call the header callback
			//	to handle headers
			((result=curl_easy_setopt(
				curl,
				CURLOPT_HEADERFUNCTION,
				&header_callback
			))!=CURLE_OK) ||
			//	Pass the HTTPRequest object
			//	through to the callback
			((result=curl_easy_setopt(
				curl,
				CURLOPT_WRITEHEADER,
				request.get()
			))!=CURLE_OK) ||
			//	Call the read callback to
			//	handle POSTs and such
			((result=curl_easy_setopt(
				curl,
				CURLOPT_READFUNCTION,
				&read_callback
			))!=CURLE_OK) ||
			//	Pass the HTTPRequest object
			//	through to the callback
			((result=curl_easy_setopt(
				curl,
				CURLOPT_READDATA,
				request.get()
			))!=CURLE_OK) ||
			//	Set maximum number of redirects
			((result=curl_easy_setopt(
				curl,
				CURLOPT_MAXREDIRS,
				max_redirs
			))!=CURLE_OK)
			//	If DEBUG is defined, verbose
			//#ifdef DEBUG
			/*|| ((result=curl_easy_setopt(
				curl,
				CURLOPT_VERBOSE,
				static_cast<long>(1)
			))!=CURLE_OK)*/
			//#endif
			//	Figure out a way to solve the
			//	problem this "addresses"
			|| ((result=curl_easy_setopt(
				curl,
				CURLOPT_SSL_VERIFYPEER,
				static_cast<long>(0)
			))!=CURLE_OK)
		) throw std::runtime_error(curl_easy_strerror(result));
	
	}
	
	
	void HTTPHandler::Get (const String & url, HTTPStatusStringDone callback) {
	
		//	Setup request
		CURL * curl=curl_easy_init();
		
		if (curl==nullptr) throw std::runtime_error(curl_error);
		
		//	Create HTTPRequest object
		std::unique_ptr<HTTPRequest> request;
		try {
		
			request=std::unique_ptr<HTTPRequest>(new HTTPRequest(
				curl,
				max_bytes,
				url,
				std::move(callback)
			));
			
		} catch (...) {
		
			curl_easy_cleanup(curl);
			
			throw;
		
		}
		
		//	Set options
		common_opts(curl,request);
		
		//	Add to the list of pending/active
		//	requests
		mutex.Execute([&] () {
		
			CURLMcode result;
		
			//	Add to handle
			if ((result=curl_multi_add_handle(multi,curl))!=CURLM_OK) throw std::runtime_error(
				curl_multi_strerror(
					result
				)
			);
			
			//	Add to list
			requests.emplace(
				curl,
				std::move(request)
			);
			
			//	Wake up worker to handle
			//	request
			condvar.WakeAll();
		
		});
	
	}
	
	
	static const String content_type_template("Content-Type: {0}");
	static const char * content_type_error="Could not set Content-Type header";
	
	
	void HTTPHandler::Post (const String & url, const String & content_type, const String & body, HTTPStatusStringDone callback) {
	
		//	Encode the content-type
		auto ct=String::Format(
			content_type_template,
			content_type
		).ToCString();
		struct curl_slist * headers=curl_slist_append(
			nullptr,
			static_cast<char *>(ct)
		);
		if (headers==nullptr) throw std::runtime_error(content_type_error);
		
		CURL * curl;
		std::unique_ptr<HTTPRequest> request;
		try {
		
			curl=curl_easy_init();
			
			if (curl==nullptr) throw std::runtime_error(curl_error);
			
			//	Create HTTPRequest object
			try {
			
				request=std::unique_ptr<HTTPRequest>(new HTTPRequest(
					curl,
					max_bytes,
					url,
					body,
					headers,
					std::move(callback)
				));
			
			} catch (...) {
			
				curl_easy_cleanup(curl);
				
				throw;
			
			}
			
		} catch (...) {
		
			curl_slist_free_all(headers);
			
			throw;
		
		}
		
		//	Set options
		common_opts(curl,request);
		CURLcode result;
		if (!(
			((result=curl_easy_setopt(
				curl,
				CURLOPT_HTTPHEADER,
				headers
			))==CURLE_OK) &&
			((result=curl_easy_setopt(
				curl,
				CURLOPT_POST,
				static_cast<long>(1)
			))==CURLE_OK) &&
			((result=curl_easy_setopt(
				curl,
				CURLOPT_POSTFIELDSIZE_LARGE,
				static_cast<curl_off_t>(SafeWord(request->Body().Count()))
			))==CURLE_OK)
		)) throw std::runtime_error(
			curl_easy_strerror(result)
		);
		
		//	Add to list of pending/active
		//	requests
		mutex.Execute([&] () {
		
			//	Add to multi handle
			CURLMcode result=curl_multi_add_handle(multi,curl);
			if (result!=CURLM_OK) throw std::runtime_error(
				curl_multi_strerror(result)
			);
			
			//	Add to list
			requests.emplace(
				curl,
				std::move(request)
			);
			
			//	Wake up worker to handle
			//	request
			condvar.WakeAll();
		
		});
	
	}


}

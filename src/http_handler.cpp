#include <http_handler.hpp>
#include <stdexcept>
#include <new>
#include <utility>
#include <system_error>


namespace MCPP {


	//	Error messages
	static const char * curl_error="cURL error";
	
	
	#include "http_handler_callbacks.cpp"
	
	
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
			
				//	Set wait flag since we
				//	can't wait for whatever
				//	it is that cURL is doing
				wait=true;
			
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
			
			//	We can call cURL
			if (num_socks!=0) {
			
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
								SmartPointer<HTTPRequest> request(requests[msg->easy_handle]);
								
								//	Dispatch callback as appropriate
								if (msg->data.result!=CURLE_OK) {
								
									//	FAIL
									request->Done(0);
									
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
								requests.erase(msg->easy_handle);
							
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
	
	}


	HTTPHandler::HTTPHandler (Word max_bytes) : stop(false), max_bytes(max_bytes) {
	
		//	Initialize handle and set settings
		if (
			(curl_global_init(CURL_GLOBAL_ALL)!=0) ||
			((multi=curl_multi_init())==nullptr)
		) throw std::runtime_error(curl_error);
		
		CURLMcode result;
		if (
			((result=curl_multi_setopt(
				multi,
				CURLMOPT_PIPELINING,
				static_cast<long>(0)
			))!=CURLM_OK) /*||
			((result=curl_multi_setopt(
				multi,
				CURLMOPT_MAX_TOTAL_CONNECTIONS,
				static_cast<long>(0)
			))!=CURLM_OK)*/
		) throw std::runtime_error(
			curl_multi_strerror(
				result
			)
		);
		
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
	
	
	static inline void common_opts (CURL * curl, const String & url, SmartPointer<HTTPRequest> request) {
	
		//	We'll need the UTF-8,
		//	null-terminated representation
		//	of the URL to pass through to
		//	cURL
		Vector<Byte> url_encoded(UTF8().Encode(url));
		url_encoded.Add(0);
		
		//	Apply settings
		CURLcode result;
		if (
			//	Set URL
			((result=curl_easy_setopt(
				curl,
				CURLOPT_URL,
				reinterpret_cast<char *>(
					static_cast<Byte *>(
						url_encoded
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
				reinterpret_cast<void *>(
					static_cast<HTTPRequest *>(
						request
					)
				)
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
				reinterpret_cast<void *>(
					static_cast<HTTPRequest *>(
						request
					)
				)
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
				reinterpret_cast<void *>(
					static_cast<HTTPRequest *>(
						request
					)
				)
			))!=CURLE_OK)
			//	If DEBUG is defined, verbose
			#ifdef DEBUG
			/*|| ((result=curl_easy_setopt(
				curl,
				CURLOPT_VERBOSE,
				static_cast<long>(1)
			))!=CURLE_OK)*/
			#endif
		) throw std::runtime_error(curl_easy_strerror(result));
	
	}
	
	
	void HTTPHandler::Get (const String & url, const HTTPStatusStringDone & callback) {
	
		//	Setup request
		CURL * curl=curl_easy_init();
		
		if (curl==nullptr) throw std::runtime_error(curl_error);
		
		//	Create HTTPRequest object
		SmartPointer<HTTPRequest> request;
		try {
		
			request=SmartPointer<HTTPRequest>::Make(
				curl,
				max_bytes,
				callback
			);
			
		} catch (...) {
		
			curl_easy_cleanup(curl);
			
			throw;
		
		}
		
		//	Set options
		common_opts(curl,url,request);
		
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
			requests.insert(
				decltype(requests)::value_type(
					curl,
					std::move(request)
				)
			);
			
			//	Wake up worker to handle
			//	request
			condvar.WakeAll();
		
		});
	
	}


}

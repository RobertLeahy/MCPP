#include <dns_handler.hpp>
#include <safeint.hpp>
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>
#ifdef ENVIRONMENT_WINDOWS
#include <nameser.h>
#include <windows.h>
#else
#include <arpa/nameser.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#endif


namespace MCPP {


	//	Constants for worker/master end
	//	of socket pair
	static const Word worker_end=1;
	static const Word master_end=0;
	
	
	static std::runtime_error get_exception (int code) noexcept {
	
		return std::runtime_error(
			ares_strerror(code)
		);
	
	}


	[[noreturn]]
	static void raise (int code) {
	
		throw get_exception(code);
	
	}
	
	
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
	
	
	void DNSHandler::worker_func () noexcept {
	
		try {
		
			worker();
		
		} catch (...) {
		
			try {
		
				panic(std::current_exception());
				
			} catch (...) {	}
		
		}
	
	}
	
	
	bool DNSHandler::should_stop () {
	
		for (;;) {
		
			auto command=control.Receive<bool>();
			
			if (command.IsNull()) return false;
			
			if (*command) return true;
			
		}
	
	}
	
	
	void DNSHandler::worker () {
	
		//	FD sets used for selecting
		fd_set readable;
		fd_set writeable;
		//	Maximum file descriptor to pass
		//	to select
		int nfds;
		//	The timeout for select
		struct timeval tv;
	
		//	Loop until shutdown
		for (;;) {
		
			//	Zero FDs
			FD_ZERO(&readable);
			FD_ZERO(&writeable);			
			
			if (lock.Execute([&] () mutable {
			
				//	Wait for there to be something actionable,
				//	either pending queries, or a shutdown command
				while (!stop && (queries.size()==0)) wait.Sleep(lock);
				
				//	If the command to shutdown has been given,
				//	do that at once
				if (stop) return true;
				
				//	Get the file descriptors from libcares
				nfds=ares_fds(
					channel,
					&readable,
					&writeable
				);
				//	Get the timeout from libcares
				ares_timeout(
					channel,
					nullptr,
					&tv
				);
				
				return false;
			
			})) break;
			
			//	If there's a message pending on
			//	the worker end of the socket pair,
			//	we want to be woken up, so 
			
			//	If there's a message pending
			//	on the worker end of the socket
			//	pair, we want to be woken up,
			//	so we're checking for readability
			nfds=control.Add(readable,nfds);
			
			//	Wait for something to happen
			if (select(
				nfds,
				&readable,
				&writeable,
				nullptr,
				&tv
			)==
			#ifdef ENVIRONMENT_WINDOWS
			SOCKET_ERROR
			#else
			-1
			#endif
			) raise_os();
			
			//	Did something happen with the
			//	control socket?
			if (control.Is(readable)) {
			
				//	Check to see if a shutdown command
				//	is coming through the control socket,
				//	if so end at once
				if (should_stop()) break;
			
				//	Remove it in case it matters to
				//	libcares what's in the fd_set
				control.Clear(readable);
			
			}
			
			//	Call libcares
			lock.Execute([&] () mutable {
			
				ares_process(
					channel,
					&readable,
					&writeable
				);
			
			});
		
		}
	
	}
	
	
	void DNSHandler::issue_command (bool is_shutdown) {
	
		control.Send(is_shutdown);
	
	}
	
	
	template <typename T, typename... Args>
	decltype(std::declval<T>().Get()) DNSHandler::begin (const String & name, Args &&... args) {
	
		auto ptr=std::unique_ptr<Query>(new T(std::forward<Args>(args)...));
		
		return lock.Execute([&] () mutable {
		
			auto p=ptr.get();
			auto pair=queries.emplace(
				p,
				std::move(ptr)
			);
			
			try {
			
				issue_command();
				
				p->Begin(
					channel,
					name,
					complete
				);
				
				wait.WakeAll();
				
			} catch (...) {
			
				queries.erase(pair.first);
				
				throw;
			
			}
			
			return reinterpret_cast<T *>(p)->Get();
		
		});
	
	}


	DNSHandler::DNSHandler (PanicType panic) : stop(false), panic(std::move(panic)) {
	
		//	Setup the completion callback
		complete=[this] (Query * q, bool) mutable noexcept {
		
			lock.Execute([&] () mutable {	queries.erase(q);	});
		
		};
	
		//	Check panic callback, if it's empty,
		//	default it to std::abort
		if (!panic) panic=[] (std::exception_ptr) {	std::abort();	};
	
		//	Initialize libcares
		auto result=ares_library_init(ARES_LIB_INIT_ALL);
		if (result!=0) raise(result);
		
		try {
		
			//	Prepare an asynchronous resolver
			//	channel
			if ((result=ares_init(&channel))!=0) raise(result);
			
			try {
			
				//	Create worker thread
				thread=Thread([this] () mutable {	worker_func();	});
			
			} catch (...) {
			
				ares_destroy(channel);
				
				throw;
			
			}
		
		} catch (...) {
		
			ares_library_cleanup();
			
			throw;
		
		}
	
	}
	
	
	DNSHandler::~DNSHandler () noexcept {
	
		//	Shutdown the worker
		
		try {
		
			issue_command(true);
		
		} catch (...) {
		
			try {
			
				panic(std::current_exception());
			
			} catch (...) {	}
			
			//	If the panic function returns,
			//	we can't do anything constructive
			//	at this point, so kill the whole
			//	process.
			std::abort();
		
		}
		
		lock.Execute([&] () mutable {
		
			stop=true;
			
			wait.WakeAll();
		
		});
		
		thread.Join();
	
		//	Cleanup the asynchronous resolver channel
		ares_destroy(channel);
		
		//	Cleanup libcares
		ares_library_cleanup();
	
	}
	
	
	DNSHandler::ResultType DNSHandler::ResolveA (const String & name) {
	
		return begin<AQuery>(name);
	
	}
	
	
	DNSHandler::ResultType DNSHandler::ResolveAAAA (const String & name) {
	
		return begin<AAAAQuery>(name);
	
	}
	
	
	template <Word num>
	class DNSRecordAggregator {
	
	
		private:
		
		
			mutable Mutex lock;
			Vector<DNSRecord> results;
			DNSHandler::ResultType promise;
			Word completed;
			bool failed;
			
			
		public:
		
		
			DNSRecordAggregator () noexcept : completed(0), failed(false) {	}
			
			
			void Complete (Vector<DNSRecord> results) {
			
				lock.Execute([&] () mutable {
				
					//	Fast fail
					if (failed) return;
				
					//	Try and add each result from this set
					//	to the aggregate set
					try {
					
						for (auto & result : results) this->results.Add(std::move(result));
					
					//	On exception, dutifully fail
					} catch (...) {
					
						promise.Fail(std::current_exception());
						
						failed=true;
					
					}
					
					//	If this is the last one in, complete
					//	the promise
					if ((++completed)==num) promise.Complete(std::move(this->results));
				
				});
			
			}
			
			
			void Fail (std::exception_ptr ex) noexcept {
			
				lock.Execute([&] () mutable {
				
					promise.Fail(std::move(ex));
					
					failed=true;
				
				});
			
			}
			
			
			DNSHandler::ResultType Get () const noexcept {
			
				return promise;
			
			}
	
	
	};
	
	
	DNSHandler::ResultType DNSHandler::Resolve (const String & name) {
	
		auto ptr=SmartPointer<DNSRecordAggregator<2>>::Make();
		
		std::function<void (ResultType)> callback=[ptr] (ResultType result) mutable noexcept {
		
			try {
			
				ptr->Complete(result.Get());
			
			} catch (...) {
			
				ptr->Fail(std::current_exception());
			
			}
		
		};
		
		ResolveA(name).Then(callback);
		
		try {
		
			ResolveAAAA(name).Then(std::move(callback));
		
		} catch (...) {
		
			ptr->Fail(std::current_exception());
			
			throw;
		
		}
		
		return ptr->Get();
	
	}
	
	
	DNSHandler::MXType DNSHandler::ResolveMX (const String & name) {
	
		return begin<MXQuery>(name);
	
	}
	
	
	DNSHandler::HostType DNSHandler::ResolveNS (const String & name) {
	
		return begin<NSQuery>(name);
	
	}
	
	
	DNSHandler::TXTType DNSHandler::ResolveTXT (const String & name) {
	
		return begin<TXTQuery>(name);
	
	}
	
	
	static const String ipv4_ptr("in-addr.arpa");
	static const String ipv6_ptr("ip6.arpa");
	static const String domain_separator(".");
	
	
	DNSHandler::HostType DNSHandler::ResolvePTR (IPAddress ip) {
	
		//	Hostname to query
		String name;
		
		if (ip.IsV6()) {
		
			//	Convert the IPv6 address to an
			//	integer
			auto num=static_cast<UInt128>(ip);
			
			//	Extract each nibble
			for (Word i=0;i<(sizeof(num)*2);++i,num>>=4) name << String(
				num&static_cast<UInt128>(
					15	//	Low 4 bits set
				),
				16	//	Hexadecimal
			) << domain_separator;
			
			name << ipv6_ptr;
		
		} else {
		
			//	Convert the IPv4 address to an
			//	integer
			auto num=static_cast<UInt32>(ip);
			
			//	Extract the octets
			for (Word i=0;i<sizeof(num);++i,num>>=8) name << String(
				num&static_cast<UInt32>(255)
			) << domain_separator;
			
			name << ipv4_ptr;
		
		}
		
		return begin<PTRQuery>(name,ip);
	
	}
	
	
	DNSHandler::SRVType DNSHandler::ResolveSRV (const String & name) {
	
		return begin<SRVQuery>(name);
	
	}
	
	
	DNSHandler::SOAType DNSHandler::ResolveSOA (const String & name) {
	
		return begin<SOAQuery>(name);
	
	}
	
	
	DNSHandler::Query::Query (int type) noexcept : type(type) {	}
	
	
	DNSHandler::Query::~Query () noexcept {	}
	
	
	//	Backslashes in arguments to libcares must
	//	be backslash-escaped
	static const Regex escape("\\\\");
	static const RegexReplacement escape_replacement("\\\\");
	
	
	void DNSHandler::Query::Begin (ares_channel channel, const String & name, DoneType callback) {
	
		this->callback=std::move(callback);
		
		//	Escape the string and convert it
		//	into a C string to be passed to
		//	libcares
		auto c_string=escape.Replace(name,escape_replacement).ToCString();
		
		ares_query(
			channel,
			c_string.begin(),
			ns_c_in,	//	Internet
			type,
			[] (void * arg, int status, int timeouts, unsigned char * abuf, int alen) noexcept {
			
				auto ptr=reinterpret_cast<Query *>(arg);
				
				ptr->Complete(
					status,
					static_cast<Word>(timeouts),
					abuf,
					alen
				);
				
				ptr->Done(status==ARES_SUCCESS);
			
			},
			this
		);
	
	}
	
	
	void DNSHandler::Query::Done (bool success) noexcept {
	
		try {
		
			callback(this,success);
		
		} catch (...) {	}
	
	}
	
	
	template <typename T>
	bool is_empty (int status, Promise<T> & promise) noexcept {
	
		return false;
	
	}
	
	
	template <typename T>
	bool is_empty (int status, Promise<Vector<T>> & promise) noexcept {
	
		if (status!=ARES_ENODATA) return false;
		
		promise.Complete(Vector<T>());
		
		return true;
	
	}
	
	
	template <typename T>
	bool is_failure (int status, Promise<T> & promise) noexcept {
	
		if (status==ARES_SUCCESS) return false;
		
		if (!is_empty(status,promise)) promise.Fail(
			std::make_exception_ptr(
				get_exception(status)
			)
		);
		
		return true;
	
	}
	
	
	static Vector<DNSHostRecord> hostent_to_result (struct hostent * ptr) {
	
		auto aliases=ptr->h_aliases;
	
		//	Count number of aliases so we can
		//	return proper number
		auto p=aliases;
		for (;*p!=nullptr;++p);
		
		//	Allocate enough space to hold all such
		//	results
		Vector<DNSHostRecord> retr(static_cast<Word>(p-aliases));
		
		//	Loop again and store each result
		for (p=aliases;*p!=nullptr;++p) retr.Add(
			DNSHostRecord{
				*p,
				0	//	libcares does not supply the TTL
			}
		);
		
		return retr;
	
	}
	
	
	template <typename T>
	void fix_endianness (Byte (& arr) [sizeof(T)]) noexcept {
	
		if (!Endianness::IsBigEndian<T>()) std::reverse(
			std::begin(arr),
			std::end(arr)
		);
		
	}
	
	
	DNSHandler::AQuery::AQuery () noexcept : Query(ns_t_a) {	}
	
	
	void DNSHandler::AQuery::Complete (int status, Word, unsigned char * buffer, int len) noexcept {
	
		//	Do not proceed if this query failed
		if (is_failure(status,promise)) return;
		
		promise.Execute([&] () mutable {
		
			Vector<struct ares_addrttl> vec;
			for (;;) {
			
				//	Resize vector
				vec.SetCapacity();
				
				//	Attempt to parse
				auto num=safe_cast<int>(vec.Capacity());
				auto result=ares_parse_a_reply(
					buffer,
					len,
					nullptr,
					vec.begin(),
					&num
				);
				//	If there's just nothing in this
				//	response, return nothing
				if (result==ARES_ENODATA) return Vector<DNSRecord>();
				if (result!=ARES_SUCCESS) raise(result);
				
				//	If there's extra room left over, this
				//	means that we parsed every single record
				//	and are done
				if (static_cast<Word>(num)!=vec.Capacity()) {
				
					vec.SetCount(static_cast<Word>(num));
					
					break;
				
				}
			
			}
			
			//	Convert each ares_addrttl structure
			//	to a DNSRecord
			Vector<DNSRecord> results(vec.Count());
			for (auto & addrttl : vec) {
			
				union {
					decltype(addrttl.ipaddr) in;
					UInt32 out;
					Byte buffer [sizeof(UInt32)];
				};
				in=addrttl.ipaddr;
				fix_endianness<UInt32>(buffer);
				
				results.Add(
					DNSRecord{
						out,
						static_cast<Word>(addrttl.ttl)
					}
				);
				
			};
			
			return results;
		
		});
	
	}
	
	
	DNSHandler::AAAAQuery::AAAAQuery () noexcept : Query(ns_t_aaaa) {	}
	
	
	void DNSHandler::AAAAQuery::Complete (int status, Word, unsigned char * buffer, int len) noexcept {
	
		//	Do not proceed if this query failed
		if (is_failure(status,promise)) return;
		
		promise.Execute([&] () mutable {
		
			Vector<struct ares_addr6ttl> vec;
			for (;;) {
			
				//	Resize vector
				vec.SetCapacity();
				
				//	Attempt to parse
				auto num=safe_cast<int>(vec.Capacity());
				auto result=ares_parse_aaaa_reply(
					buffer,
					len,
					nullptr,
					vec.begin(),
					&num
				);
				//	If there's just nothing in this
				//	response, return nothing
				if (result==ARES_ENODATA) return Vector<DNSRecord>();
				if (result!=ARES_SUCCESS) raise(result);
				
				//	If there's extra room left over, this means
				//	that we parsed every single record and are
				//	thus finished
				if (static_cast<Word>(num)!=vec.Capacity()) {
				
					vec.SetCount(static_cast<Word>(num));
					
					break;
				
				}
				
			}
				
			//	Convert each ares_addr6ttl structure
			//	to a DNSRecord
			Vector<DNSRecord> results(vec.Count());
			for (auto & addrttl : vec) {
				
				union {
					decltype(addrttl.ip6addr) in;
					UInt128 out;
					Byte buffer [sizeof(UInt128)];
				};
				in=addrttl.ip6addr;
				fix_endianness<UInt128>(buffer);
				
				results.Add(
					DNSRecord{
						out,
						static_cast<Word>(addrttl.ttl)
					}
				);
			
			}
			
			return results;
		
		});
	
	}
	
	
	DNSHandler::MXQuery::MXQuery () noexcept : Query(ns_t_mx) {	}
	
	
	void DNSHandler::MXQuery::Complete (int status, Word, unsigned char * buffer, int len) noexcept {
	
		//	Do not proceed if this query failed
		if (is_failure(status,promise)) return;
		
		promise.Execute([&] () mutable {
		
			Vector<DNSMXRecord> results;
		
			//	Attempt to parse the MX reply
			struct ares_mx_reply * ptr;
			auto result=ares_parse_mx_reply(
				buffer,
				len,
				&ptr
			);
			//	If there's just nothing in this
			//	response, return nothing
			if (result==ARES_ENODATA) return results;
			if (result!=ARES_SUCCESS) raise(result);
			
			try {
			
				//	Count the number of MX entries
				//	so we can allocate an appropriately-sized
				//	vector
				Word count=0;
				for (auto p=ptr;p!=nullptr;p=p->next) ++count;
				
				results=Vector<DNSMXRecord>(count);
				
				//	Place MX entries into the vector
				for (auto p=ptr;p!=nullptr;p=p->next) results.Add(
					DNSMXRecord{
						p->host,
						p->priority,
						0	//	Dummy value, TTL isn't actually provided by libcares
					}
				);
			
			} catch (...) {
			
				ares_free_data(ptr);
				
				throw;
			
			}
			
			ares_free_data(ptr);
			
			return results;
		
		});
	
	}
	
	
	DNSHandler::NSQuery::NSQuery () noexcept : Query(ns_t_ns) {	}
	
	
	void DNSHandler::NSQuery::Complete (int status, Word, unsigned char * buffer, int len) noexcept {
	
		//	Do not proceed if this query failed
		if (is_failure(status,promise)) return;
		
		promise.Execute([&] () mutable {
		
			Vector<DNSHostRecord> results;
			
			//	Attempt to parse the NS reply
			struct hostent * ptr;
			auto result=ares_parse_ns_reply(
				buffer,
				len,
				&ptr
			);
			//	If there's just nothing in this
			//	response, return nothing
			if (result==ARES_ENODATA) return results;
			if (result!=ARES_SUCCESS) raise(result);
			
			//	Responsible for freeing memory
			//	from here on out
			try {
			
				results=hostent_to_result(ptr);
			
			} catch (...) {
			
				ares_free_hostent(ptr);
				
				throw;
			
			}
			
			ares_free_hostent(ptr);
			
			return results;
		
		});
	
	}
	
	
	DNSHandler::TXTQuery::TXTQuery () noexcept : Query(ns_t_txt) {	}
	
	
	void DNSHandler::TXTQuery::Complete (int status, Word, unsigned char * buffer, int len) noexcept {
	
		//	Do not proceed if this query failed
		if (is_failure(status,promise)) return;
		
		promise.Execute([&] () mutable {
		
			Vector<DNSTXTRecord> results;
			
			//	Attempt to parse the TXT reply
			struct ares_txt_reply * ptr;
			auto result=ares_parse_txt_reply(
				buffer,
				len,
				&ptr
			);
			//	If there's no response, return
			//	nothing
			if (result==ARES_ENODATA) return results;
			if (result!=ARES_SUCCESS) raise(result);
			
			//	Responsible for freeing ares_txt_reply
			//	linked list
			try {
			
				//	Count the number of TXT records
				Word count=0;
				for (auto p=ptr;p!=nullptr;p=p->next) ++count;
				
				results=Vector<DNSTXTRecord>(count);
				
				//	Place TXT entries into the vector
				for (auto p=ptr;p!=nullptr;p=p->next) results.Add(
					DNSTXTRecord{
						reinterpret_cast<char *>(p->txt),
						0	//	Dummy value, TTL isn't actually provided by libcares
					}
				);
			
			} catch (...) {
			
				ares_free_data(ptr);
				
				throw;
			
			}
			
			ares_free_data(ptr);
			
			return results;
		
		});
	
	}
	
	
	DNSHandler::PTRQuery::PTRQuery (IPAddress ip) noexcept : Query(ns_t_ptr), ip(ip) {	}
	
	
	void DNSHandler::PTRQuery::Complete (int status, Word, unsigned char * buffer, int len) noexcept {
	
		//	Do not proceed if this query failed
		if (is_failure(status,promise)) return;
		
		promise.Execute([&] () mutable {
		
			Vector<DNSHostRecord> results;
			
			//	ares_parse_ptr_reply requires the
			//	original IP address that was
			//	queried for
			
			union {
				struct in6_addr addr_6;
				struct in_addr addr;
				UInt128 addr_6_int;
				UInt32 addr_int;
				Byte addr_6_buffer [sizeof(UInt128)];
				Byte addr_buffer [sizeof(UInt32)];
			};
			int family;
			int addrlen;
			
			if (ip.IsV6()) {
			
				family=AF_INET;
				addrlen=sizeof(addr_6);
				
				addr_6_int=static_cast<UInt128>(ip);
				fix_endianness<UInt128>(addr_6_buffer);
			
			} else {
			
				family=AF_INET6;
				addrlen=sizeof(addr);
				
				addr_int=static_cast<UInt32>(ip);
				fix_endianness<UInt32>(addr_buffer);
			
			}
			
			//	Attempt to parse the PTR reply
			struct hostent * ptr;
			auto result=ares_parse_ptr_reply(
				buffer,
				len,
				&addr,
				addrlen,
				family,
				&ptr
			);
			//	Short-circuit out if there's no
			//	data
			if (result==ARES_ENODATA) return results;
			if (result!=ARES_SUCCESS) raise(result);
			
			//	Responsible for freeing hostent
			try {
			
				results=hostent_to_result(ptr);
			
			} catch (...) {
			
				ares_free_hostent(ptr);
				
				throw;
			
			}
			
			ares_free_hostent(ptr);
			
			return results;
		
		});
	
	}
	
	
	DNSHandler::SRVQuery::SRVQuery () noexcept : Query(ns_t_srv) {	}
	
	
	void DNSHandler::SRVQuery::Complete (int status, Word, unsigned char * buffer, int len) noexcept {
	
		//	Do not proceed if this query failed
		if (is_failure(status,promise)) return;
		
		promise.Execute([&] () mutable {
		
			Vector<DNSSRVRecord> results;
			
			//	Attempt to parse SRV records
			struct ares_srv_reply * ptr;
			auto result=ares_parse_srv_reply(
				buffer,
				len,
				&ptr
			);
			//	If there's no data, return at once
			if (result==ARES_ENODATA) return results;
			if (result!=ARES_SUCCESS) raise(result);
			
			//	We're responsible for freeing the pointer,
			//	make sure that gets done
			try {
			
				//	Count number of results to allocate the
				//	proper amount of space and minimize
				//	reallocations
				Word count=0;
				for (auto p=ptr;p!=nullptr;p=p->next) ++count;
				
				results=Vector<DNSSRVRecord>(count);
				
				//	Loop and extract each SRV record
				for (auto p=ptr;p!=nullptr;p=p->next) results.Add(
					DNSSRVRecord{
						p->host,
						static_cast<Word>(p->weight),
						static_cast<Word>(p->priority),
						static_cast<UInt16>(p->port)
					}
				);
			
			} catch (...) {
			
				ares_free_data(ptr);
				
				throw;
			
			}
			
			ares_free_data(ptr);
			
			return results;
		
		});
	
	}
	
	
	DNSHandler::SOAQuery::SOAQuery () noexcept : Query(ns_t_soa) {	}
	
	
	void DNSHandler::SOAQuery::Complete (int status, Word, unsigned char * buffer, int len) noexcept {
	
		promise.Execute([&] () mutable {
		
			//	Throw on failure
			if (status!=ARES_SUCCESS) raise(status);
			
			//	Attempt to parse
			struct ares_soa_reply * ptr;
			auto result=ares_parse_soa_reply(
				buffer,
				len,
				&ptr
			);
			//	Throw on failure
			if (result!=ARES_SUCCESS) raise(result);
			
			//	We're responsible for freeing the
			//	pointer
			DNSSOARecord retr;
			try {
			
				retr.PrimaryNameServer=ptr->nsname;
				retr.HostMaster=ptr->hostmaster;
				retr.Serial=static_cast<Word>(ptr->serial);
				retr.Refresh=static_cast<Word>(ptr->refresh);
				retr.Retry=static_cast<Word>(ptr->retry);
				retr.Expire=static_cast<Word>(ptr->expire);
				retr.NXTTL=static_cast<Word>(ptr->minttl);
				retr.TTL=0;	//	libcares does not provide this
			
			} catch (...) {
			
				ares_free_data(ptr);
				
				throw;
			
			}
			
			ares_free_data(ptr);
			
			return retr;
		
		});
	
	}


}

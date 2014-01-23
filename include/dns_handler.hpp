/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <socketpair.hpp>
#include <promise.hpp>
#include <ares.h>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>


namespace MCPP {
	
	
	/**
	 *	A single result of a DNS query for
	 *	A or AAAA records.
	 */
	class DNSRecord {
	
	
		public:
		
		
			/**
			 *	The IPv4 or IPv6 address associated with
			 *	this A or AAAA record.
			 */
			IPAddress IP;
			/**
			 *	The time to live associated with this record.
			 */
			Word TTL;
	
	
	};
	
	
	/**
	 *	A single result of a DNS query returning
	 *	host names.
	 */
	class DNSHostRecord {
	
	
		public:
		
		
			/**
			 *	The host name.
			 */
			String Host;
			/**
			 *	The time to live of this record.
			 *
			 *	Unused.
			 */
			Word TTL;
	
	
	};
	
	
	/**
	 *	A single result of a DNS query for
	 *	MX records.
	 */
	class DNSMXRecord {
	
	
		public:
		
		
			/**
			 *	The host name of the mail exchanger.
			 */
			String Host;
			/**
			 *	The priority associated with the mail
			 *	exchanger.
			 */
			Word Priority;
			/**
			 *	The time to live of this record.
			 *
			 *	Unused.
			 */
			Word TTL;
	
	
	};
	
	
	/**
	 *	A single result of a DNS query for
	 *	TXT records.
	 */
	class DNSTXTRecord {
	
	
		public:
		
		
			/**
			 *	The text associated with this TXT
			 *	record.
			 */
			String Text;
			/**
			 *	The time to live of this record.
			 *
			 *	Unused.
			 */
			Word TTL;
	
	
	};
	
	
	/**
	 *	A single result of a DNS query for SRV
	 *	records.
	 */
	class DNSSRVRecord {
	
	
		public:
		
		
			/**
			 *	The host associated with this
			 *	SRV record.
			 */
			String Host;
			/**
			 *	The weight associated with this host.
			 */
			Word Weight;
			/**
			 *	The priority associated with this host.
			 */
			Word Priority;
			/**
			 *	The port on which the service-in-question
			 *	is available.
			 */
			UInt16 Port;
	
	
	};
	
	
	/**
	 *	The result of a SOA query.
	 */
	class DNSSOARecord {
	
	
		public:
		
		
			/**
			 *	The primary name server of the
			 *	zone-in-question.
			 */
			String PrimaryNameServer;
			/**
			 *	The name of the host master.
			 */
			String HostMaster;
			/**
			 *	The zone's serial.
			 */
			Word Serial;
			/**
			 *	Indicates the time when the slave will
			 *	try to refresh the zone from the master.
			 */
			Word Refresh;
			/**
			 *	Defines the time between retries if the
			 *	slave fails to contact the master when
			 *	Refresh has expired.
			 */
			Word Retry;
			/**
			 *	Used by slave servers, indicates when zone
			 *	data is no longer authoritative.
			 */
			Word Expire;
			/**
			 *	The time to live for NXDOMAIN responses
			 *	to queries into this zore.
			 */
			Word NXTTL;
			/**
			 *	The time to live of this record.
			 *
			 *	Unused.
			 */
			Word TTL;
	
	
	};


	/**
	 *	Allows asynchronous DNS queries to be made.
	 */
	class DNSHandler {
	
	
		public:
		
		
			typedef std::function<void (std::exception_ptr)> PanicType;
			typedef Promise<Vector<DNSRecord>> ResultType;
			typedef Promise<Vector<DNSMXRecord>> MXType;
			typedef Promise<Vector<DNSHostRecord>> HostType;
			typedef Promise<Vector<DNSTXTRecord>> TXTType;
			typedef Promise<Vector<DNSSRVRecord>> SRVType;
			typedef Promise<DNSSOARecord> SOAType;
	
	
		private:
		
		
			class Query;
			
			
			typedef std::function<void (Query *, bool)> DoneType;
		
		
			class Query {
			
			
				protected:
				
				
					int type;
					DoneType callback;
					
					
					Query (int) noexcept;
					
					
				public:
				
				
					Query () = delete;
					Query (const Query &) = delete;
					Query (Query &&) = delete;
					Query & operator = (const Query &) = delete;
					Query & operator = (Query &&) = delete;
				
				
					virtual ~Query () noexcept;
					virtual void Complete (int status, Word timeouts, unsigned char * abuf, int alen) noexcept = 0;
					void Begin (ares_channel, const String &, DoneType);
					void Done (bool) noexcept;
			
			
			};
			
			
			template <typename T>
			class ASyncQuery {
			
			
				protected:
				
				
					Promise<T> promise;
					
					
				public:
				
				
					Promise<T> Get () const noexcept {
					
						return promise;
					
					}
			
			
			};
			
			
			class AQuery : public Query, public ASyncQuery<Vector<DNSRecord>> {
			
			
				public:
				
				
					AQuery () noexcept;
					
					
					virtual void Complete (int, Word, unsigned char *, int) noexcept override;
			
			
			};
			
			
			class AAAAQuery : public Query, public ASyncQuery<Vector<DNSRecord>> {
			
			
				public:
				
				
					AAAAQuery () noexcept;
					
					
					virtual void Complete (int, Word, unsigned char *, int) noexcept override;
					
			
			
			};
			
			
			class MXQuery : public Query, public ASyncQuery<Vector<DNSMXRecord>> {
					
					
				public:
				
				
					MXQuery () noexcept;
					
					
					virtual void Complete (int, Word, unsigned char *, int) noexcept override;
			
			
			};
			
			
			class NSQuery : public Query, public ASyncQuery<Vector<DNSHostRecord>> {
			
			
				public:
				
				
					NSQuery () noexcept;
					
					
					virtual void Complete (int, Word, unsigned char *, int) noexcept override;
			
			
			};
			
			
			class TXTQuery : public Query, public ASyncQuery<Vector<DNSTXTRecord>> {
			
			
				public:
				
				
					TXTQuery () noexcept;
					
					
					virtual void Complete (int, Word, unsigned char *, int) noexcept override;
			
			
			};
			
			
			class PTRQuery : public Query, public ASyncQuery<Vector<DNSHostRecord>> {
			
			
				private:
				
				
					IPAddress ip;
			
				
				public:
				
				
					PTRQuery (IPAddress) noexcept;
					
					
					virtual void Complete (int, Word, unsigned char *, int) noexcept override;
				
			
			};
			
			
			class SRVQuery : public Query, public ASyncQuery<Vector<DNSSRVRecord>> {
			
			
				public:
				
				
					SRVQuery () noexcept;
					
					
					virtual void Complete (int, Word, unsigned char *, int) noexcept override;
			
			
			};
			
			
			class SOAQuery : public Query, public ASyncQuery<DNSSOARecord> {
			
			
				public:
				
				
					SOAQuery () noexcept;
					
					
					virtual void Complete (int, Word, unsigned char *, int) noexcept override;
			
			
			};
			
			
			//	Handle to libcares
			ares_channel channel;
			
			
			//	Worker
			Thread thread;
			//	Control socket allows worker to be
			//	notified and shutdown
			ControlSocket control;
			//	Flag is set when the worker should
			//	stop
			bool stop;
			
			
			//	List of ongoing queries
			mutable Mutex lock;
			mutable CondVar wait;
			std::unordered_map<
				Query *,
				std::unique_ptr<Query>
			> queries;
			
			
			//	Panic callback
			PanicType panic;
			
			
			//	Completion callback
			DoneType complete;
			
			
			void worker_func () noexcept;
			bool should_stop ();
			void worker ();
			void issue_command (bool is_shutdown=false);
			template <typename T, typename... Args>
			decltype(std::declval<T>().Get()) begin (const String &, Args &&...);
			
			
		public:
		
		
			/**
			 *	Creates and starts a new DNSHandler.
			 *
			 *	\param [in] panic
			 *		Optional.  If not provided std::abort
			 *		will be used.  The callback that will
			 *		be invoked when and if something goes
			 *		wrong in the DNS handler's worker
			 *		thread.
			 */
			DNSHandler (PanicType panic=PanicType());
			/**
			 *	Stops and cleans up a DNSHandler.
			 *
			 *	All ongoing asynchronous requests will
			 *	stop and fail.
			 */
			~DNSHandler () noexcept;
			
			
			/**
			 *	Resolves a host name to A records.
			 *
			 *	\param [in] name
			 *		The host name.
			 *
			 *	\return
			 *		A promise of future results.
			 */
			ResultType ResolveA (const String & name);
			/**
			 *	Resolves a host name to AAAA records.
			 *
			 *	\param [in] name
			 *		The host name.
			 *
			 *	\return
			 *		A promise of future results.
			 */
			ResultType ResolveAAAA (const String & name);
			/**
			 *	Resolves a host name to both A and AAAA
			 *	records simultaneously.
			 *
			 *	Note that calling this function will be
			 *	significantly faster than calling ResolveA
			 *	and then ResolveAAAA (or vice versa) as
			 *	this functional internally queries for
			 *	both A and AAAA records simultaneously.
			 *
			 *	\param [in] name
			 *		The host name.
			 *
			 *	\return
			 *		A promise of future results.
			 */
			ResultType Resolve (const String & name);
			/**
			 *	Resolves a host name to MX records.
			 *
			 *	\param [in] name
			 *		The host name.
			 *
			 *	\return
			 *		A promise of future results.
			 */
			MXType ResolveMX (const String & name);
			/**
			 *	Resolves the name servers for a particular
			 *	zone.
			 *
			 *	\param [in] name
			 *		The zone name.
			 *
			 *	\return
			 *		A promise of future results.
			 */
			HostType ResolveNS (const String & name);
			/**
			 *	Resolves a host name to TXT records.
			 *
			 *	\param [in] name
			 *		The host name.
			 *
			 *	\return
			 *		A promise of future results.
			 */
			TXTType ResolveTXT (const String & name);
			/**
			 *	Resolves an IP address to a host name
			 *	through reverse DNS.  I.e. retrieves
			 *	the PTR records associated with the
			 *	hostname associated with the provided
			 *	IP address.
			 *
			 *	\param [in] ip
			 *		The IP address-in-question.
			 *
			 *	\return
			 *		A promise of future results.
			 */
			HostType ResolvePTR (IPAddress ip);
			/**
			 *	Resolves a host name to SRV records.
			 *
			 *	\param [in] name
			 *		The host name.
			 *
			 *	\return
			 *		A promise of future results.
			 */
			SRVType ResolveSRV (const String & name);
			/**
			 *	Resolves the SOA record for a zone.
			 *
			 *	\param [in] name
			 *		The zone name.
			 *
			 *	\return
			 *		A promise of a future result.
			 */
			SOAType ResolveSOA (const String & name);
	
	
	};


}

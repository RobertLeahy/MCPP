/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <ip_address_range.hpp>
#include <mod.hpp>
#include <unordered_set>


namespace MCPP {


	/**
	 *	Contains information about a Blacklist instance
	 *	at a particular point in time.
	 */
	class BlacklistInfo {
	
	
		public:
		
		
			/**
			 *	IP ranges that are blacklisted.
			 */
			Vector<IPAddressRange> Ranges;
	
	
	};


	/**
	 *	Allows IPs and IP ranges to be blocked from connecting
	 *	to the server.
	 */
	class Blacklist : public Module {
	
	
		private:
		
		
			std::unordered_set<IPAddressRange> ranges;
			mutable RWLock lock;
			
			
			static bool is_verbose ();
			void save () const;
			void load ();
		
		
		public:
		
		
			static Blacklist & Get () noexcept;
			
			
			/**
			 *	\cond
			 */
			
			
			virtual Word Priority () const noexcept override;
			virtual const String & Name () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
			
			
			/**
			 *	Checks to see if the given IP is blacklisted.
			 *
			 *	\param [in] ip
			 *		The IP to check.
			 *
			 *	\return
			 *		\em true if \em ip is blacklisted, \em false
			 *		otherwise.
			 */
			bool Check (IPAddress ip) const noexcept;
			
			
			/**
			 *	Blacklists the given range of IPs.  If any clients
			 *	in the given range are connected, they will be
			 *	disconnected.
			 */
			void Add (IPAddressRange range);
			
			
			/**
			 *	Removes the given IP range from the blacklist if
			 *	it is blacklisted.
			 *
			 *	\param [in] range
			 *		The IP range to remove from the blacklist.
			 */
			void Remove (IPAddressRange range);
			
			
			/**
			 *	Retrieves a structure containing information about
			 *	this blacklist instance.
			 *
			 *	\return
			 *		A BlacklistInfo structure.
			 */
			BlacklistInfo GetInfo () const;
	
	
	};
	
	
}

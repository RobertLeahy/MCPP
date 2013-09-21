/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <unordered_set>


namespace MCPP {


	/**
	 *	Stores the mask associated with a given
	 *	IP address range.
	 */
	union IPAddressMask {
	
	
		/**
		 *	The 32-bit mask if the associated IP
		 *	address is a version 4 IP address.
		 */
		UInt32 IPv4;
		/**
		 *	The 128-bit mask if the associated IP
		 *	address is a version 6 IP address.
		 */
		UInt128 IPv6;
	
	
	};
	
	
	/**
	 *	Encapsulates the banlist.
	 */
	class BanList {
	
	
		public:
		
		
			/**
			 *	A list of the usernames of all
			 *	banned users, normalized to all
			 *	lower case.
			 *
			 *	This list is generated in no set
			 *	order.
			 */
			Vector<String> Users;
			/**
			 *	A list of the IP addresses that
			 *	are banned.
			 *
			 *	This list is generated in no set
			 *	order.
			 */
			Vector<IPAddress> IPs;
			/**
			 *	A list of the IP address ranges
			 *	that are banned.
			 *
			 *	This list is generated in no set
			 *	order.
			 */
			Vector<Tuple<IPAddress,IPAddressMask>> Ranges;
	
	
	};

	
	/**
	 *	Allows users to be banned from the server
	 *	based on username, IP address, or IP address
	 *	range.
	 */
	class Bans : public Module {
	
	
		private:
		
		
			std::unordered_set<String> banned_users;
			mutable RWLock users_lock;
			std::unordered_set<IPAddress> banned_ips;
			Vector<Tuple<IPAddress,IPAddressMask>> banned_ip_ranges;
			mutable RWLock ips_lock;
			
			
		public:
		
		
			/**
			 *	Retrieves a reference to a valid
			 *	instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance
			 *		of this class.
			 */
			static Bans & Get () noexcept;
		
		
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
			 *	Determines whether the user identified
			 *	by \em username is banned.
			 *
			 *	\param [in] username
			 *		The username-in-question.
			 *
			 *	\return
			 *		\em true if the user identified by
			 *		\em username is banned, \em false
			 *		otherwise.
			 */
			bool IsBanned (String username) const;
			/**
			 *	Determines whether a certain IP address
			 *	is banned.
			 *
			 *	\param [in] ip
			 *		The IP-in-question.
			 *
			 *	\return
			 *		\em true if \em ip is banned, \em false
			 *		otherwise.
			 */
			bool IsBanned (IPAddress ip) const noexcept;
			/**
			 *	Bans the user identified by \em username.
			 *
			 *	\param [in] username
			 *		The username-in-question.
			 */
			void Ban (String username);
			/**
			 *	Removes the ban on \em username, if any.
			 *
			 *	\param [in] username
			 *		The username-in-question.
			 */
			void Unban (String username);
			/**
			 *	Bans a certain IP.
			 *
			 *	\param [in] ip
			 *		The IP to ban.
			 */
			void Ban (IPAddress ip);
			/**
			 *	Unbans a certain IP.
			 *
			 *	\param [in] ip
			 *		The IP to unban.
			 */
			void Unban (IPAddress ip);
			/**
			 *	Bans a range of IPs.
			 *
			 *	\except std::invalid_argument
			 *		Thrown if \em bits is greater
			 *		than the number of bits in
			 *		\em ip.
			 *
			 *	\param [in] ip
			 *		The IP address-in-question.
			 *	\param [in] bits
			 *		The number of bits of \em ip
			 *		that shall be ignored when
			 *		matching it with other IPs.
			 */
			void Ban (IPAddress ip, Word bits);
			/**
			 *	Unbans a range of IPs.
			 *
			 *	If \em bits is greater than the
			 *	number of bits in \em ip the
			 *	call will be silently ignored.
			 *
			 *	\param [in] ip
			 *		The IP address-in-question.
			 *	\param [in] bits
			 *		The number of bits of \em ip
			 *		that shall be ignored when
			 *		matching it with other IPs.
			 */
			void Unban (IPAddress ip, Word bits);
			
			
			/**
			 *	Retrieves a list of all bans.
			 *
			 *	\return
			 *		An object containing the complete
			 *		contents of the banlist as of
			 *		the time the function was called.
			 */
			BanList List () const;
	
	
	};


}

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <atomic>
#include <unordered_set>


namespace MCPP {


	/**
	 *	Manages a whitelist, which controls access
	 *	to the server by only permitting those
	 *	explicitly authorized.
	 */
	class Whitelist : public Module {
	
	
		private:
		
		
			mutable Mutex lock;
			std::unordered_set<String> whitelist;
			
			std::atomic<bool> enabled;
	
	
		public:
		
		
			/**
			 *	Retrieves a valid instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance of this
			 *		class.
			 */
			static Whitelist & Get () noexcept;
			
			
			/**
			 *	\cond
			 */
			 
			 
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
			 
			 
			/**
			 *	Enables the whitelist.
			 *
			 *	\return
			 *		\em true if the whitelist was
			 *		enabled, \em false if the whitelist
			 *		was already enabled.
			 */
			bool Enable () noexcept;
			/**
			 *	Disables the whitelist.
			 *
			 *	\return
			 *		\em true if the whitelist was
			 *		disabled, \em false if the whitelist
			 *		was already disabled.
			 */
			bool Disable () noexcept;
			
			
			/**
			 *	Determines if the whitelist is enabled.
			 *
			 *	\return
			 *		\em true if the whitelist is enabled,
			 *		\em false otherwise.
			 */
			bool Enabled () const noexcept;
			
			
			/**
			 *	Adds a user to the whitelist.
			 *
			 *	\param [in] username
			 *		The username of the user to add
			 *		to the whitelist.
			 *
			 *	\return
			 *		\em true if \em username was added
			 *		to the whitelist, \em false if
			 *		\em username was already
			 *		whitelisted.
			 */
			bool Add (String username);
			/**
			 *	Removes a user from the whitelist.
			 *
			 *	\param [in] username
			 *		The username of the user to remove
			 *		from the whitelist.
			 *
			 *	\return
			 *		\em true if \em username was removed
			 *		from the whitelist, \em false if
			 *		\em username wasn't on the whitelist.
			 */
			bool Remove (String username);
			
			
			/**
			 *	Checks to see if a user is whitelisted.
			 *
			 *	\param [in] username
			 *		The username to check.
			 *
			 *	\return
			 *		\em true if \em username is whitelisted,
			 *		\em false otherwise.
			 */
			bool Check (String username) const;
			
			
			/**
			 *	Retrieves the whitelist.
			 *
			 *	\return
			 *		A list of strings containing
			 *		the usernames on the whitelist.
			 */
			Vector<String> GetInfo () const;
			
	
	
	};


}

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <unordered_map>


namespace MCPP {


	/**
	 *	Represents a single ban.
	 */
	class BanInfo {
	
	
		public:
		
		
			/**
			 *	The username of the banned user.
			 */
			String Username;
			/**
			 *	The username of the user who banned
			 *	the user-in-question, if specified.
			 */
			Nullable<String> By;
			/**
			 *	The reason for the ban, if any.
			 */
			Nullable<String> Reason;
	
	
	};


	/**
	 *	Allows usernames to be banned from logging
	 *	into the server.
	 */
	class Bans : public Module {
	
	
		private:
		
		
			std::unordered_map<String,BanInfo> bans;
			mutable RWLock lock;
			
			
		public:
		
		
			/**
			 *	Retrieves a valid instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance of
			 *		this class.
			 */
			static Bans & Get () noexcept;
			
			
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
			 *	Bans a particular user.
			 *
			 *	\param [in] ban
			 *		The ban to enact.
			 *
			 *	\return
			 *		\em true if the ban was
			 *		successfully applied, \em false
			 *		if the given username was already
			 *		banned.
			 */
			bool Ban (BanInfo ban);
			/**
			 *	Unbans a particular username.
			 *
			 *	\param [in] username
			 *		The username to unban.
			 *	\param [in] by
			 *		The username of the person unbanned
			 *		\em username, if applicable.  Defaults
			 *		to null.
			 *
			 *	\return
			 *		\em true if \em username was
			 *		unbanned.  \em false if \em username
			 *		was not banned to begin with.
			 */
			bool Unban (String username, Nullable<String> by=Nullable<String>());
			
			
			/**
			 *	Checks to see if a particular username is
			 *	banned.
			 *
			 *	\param [in] username
			 *		The username to check.
			 *
			 *	\return
			 *		\em true if \em username is banned,
			 *		\em false otherwise.
			 */
			bool Check (String username) const;
			/**
			 *	Attempts to retrieve the information about
			 *	the ban applied to a certain username.
			 *
			 *	\param [in] username
			 *		The username to retrieve.
			 *
			 *	\return
			 *		A structure containing the information
			 *		about the ban of \em username if
			 *		\em username is banned, null otherwise.
			 */
			Nullable<BanInfo> Retrieve (String username) const;
			
			
			/**
			 *	Retrieves the banlist.
			 *
			 *	\return
			 *		A vector of bans containing every
			 *		ban.
			 */
			Vector<BanInfo> GetInfo () const;
	
	
	};


}

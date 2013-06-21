/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <unordered_set>


namespace MCPP {


	class OpModule : public Module {
	
	
		private:
		
		
			std::unordered_set<String> ops;
			mutable RWLock lock;
			
			
		public:
		
		
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
			 *	Ops the user identified by \em username.
			 *
			 *	\param [in] username
			 *		The username of the user to op.
			 */
			void Op (String username);
			/**
			 *	Determines whether the user identified
			 *	by \em username is an op.
			 *
			 *	\param [in] username
			 *		The username of the user to op.
			 *
			 *	\return
			 *		\em true if the user identified by
			 *		\em username is an op, \em false
			 *		otherwise.
			 */
			bool IsOp (String username);
			/**
			 *	Removes op status from the user identified by
			 *	\em username.
			 *
			 *	\param [in] username
			 *		The username of the user to remove op
			 *		status from.
			 */
			void DeOp (String username);
			/**
			 *	Returns a list of all ops as of
			 *	the time this function is called.
			 *
			 *	\return
			 *		A list of the usernames of the
			 *		ops.  The order of this list is
			 *		unspecified.
			 */
			Vector<String> List () const;
	
	
	};
	
	
	/**
	 *	The currently active OpModule.
	 */
	extern Nullable<OpModule> Ops;


}

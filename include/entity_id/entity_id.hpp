/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <atomic>
 
 
namespace MCPP {


	/**
	 *	Generates unique entity IDs.
	 */
	class EntityIDGenerator : public Module {
	
	
		private:
		
		
			//	Minecraft only supports 32-bit IDs
			std::atomic<UInt32> id;
			
			
		public:
		
		
			/**
			 *	Retrieves a signed 32-bit identifier
			 *	which is almost guaranteed to be
			 *	unique across this instance of the
			 *	server.
			 *
			 *	Thread safe.
			 *
			 *	\return
			 *		A signed 32-bit identifier.
			 */
			static Int32 Get () noexcept;
			
			
			/**
			 *	\cond
			 */
			 
			 
			EntityIDGenerator () noexcept;
			
			
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
	
	
	};


}

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <client.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <unordered_map>


namespace MCPP {


	class Brands : public Module {
	
	
		private:
		
		
			std::unordered_map<
				SmartPointer<Client>,
				Nullable<String>
			> brands;
			RWLock lock;
		
		
		public:
		
		
			/**
			 *	Retrieves a reference to a valid
			 *	instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance
			 *		of this class.
			 */
			static Brands & Get () noexcept;
			
			
			/**
			 *	\cond
			 */
			 
			 
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
			 
			 
			Nullable<String> Get (const SmartPointer<Client> & client);
	
	
	};


}
 
/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <command/command.hpp>
#include <chat/chat.hpp>


namespace MCPP {


	/**
	 *	Interface which all information
	 *	providers must implement.
	 */
	class InformationProvider {
	
	
		public:
		
		
			virtual const String & Identifier () const noexcept = 0;
			virtual const String & Help () const noexcept = 0;
			virtual void Execute (ChatMessage & message) const = 0;
	
	
	};


	/**
	 *	Allows information about various
	 *	server components to be obtained
	 *	by server operators through chat.
	 */
	class Information : public Module, public Command {
	
	
		private:
		
		
			Vector<InformationProvider *> providers;
			
			
			Vector<InformationProvider *> retrieve (const Regex &);
			InformationProvider * retrieve (const String &);
			
			
		public:
		
		
			/**
			 *	Retrieves a reference to a valid
			 *	instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance
			 *		of this class.
			 */
			static Information & Get () noexcept;
		
		
			/**
			 *	\cond
			 */
			 
			 
			virtual Word Priority () const noexcept override;
			virtual const String & Name () const noexcept override;
			virtual void Install () override;
			 
			 
			virtual const String & Identifier () const noexcept override;
			virtual bool Check (SmartPointer<Client>) const override;
			virtual const String & Summary () const noexcept override;
			virtual const String & Help () const noexcept override;
			virtual Vector<String> AutoComplete (const String &) const override;
			virtual bool Execute (SmartPointer<Client>, const String &, ChatMessage &) override;
			 
			 
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	Adds a new information provider to be
			 *	managed by the module.
			 *
			 *	\param [in] provider
			 *		The information provider to add.
			 */
			void Add (InformationProvider * provider);
	
	
	};


}

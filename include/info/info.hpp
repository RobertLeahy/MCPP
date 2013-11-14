/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <command/command.hpp>
#include <unordered_map>


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
		
		
			std::unordered_map<String,InformationProvider *> map;
			Vector<InformationProvider *> list;
			
			
			InformationProvider * get (const String &);
			
			
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
			
			
			virtual void Summary (const String &, ChatMessage &) override;
			virtual void Help (const String &, ChatMessage &) override;
			virtual Vector<String> AutoComplete (const CommandEvent &) override;
			virtual bool Check (const CommandEvent &) override;
			virtual CommandResult Execute (CommandEvent) override;
			 
			 
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

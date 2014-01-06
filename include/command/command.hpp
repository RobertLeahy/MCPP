/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <command_interpreter.hpp>
#include <mod.hpp>
#include <unordered_map>


namespace MCPP {


	/**
	 *	Represents a command which is either
	 *	being executed, or which may be
	 *	executed
	 */
	class CommandEvent {
	
	
		public:
		
		
			/**
			 *	The client object representing the
			 *	client who issued the command, or
			 *	a null pointer if no client issued
			 *	the command.
			 */
			SmartPointer<Client> Issuer;
			/**
			 *	The identifier that caused this
			 *	event.  In the case of a command
			 *	event initiated through chat or
			 *	other text interface, this is all
			 *	characters after the leading
			 *	solidus up until the first whitespace
			 *	character.
			 */
			String Identifier;
			/**
			 *	The arguments passed to the command.
			 */
			Vector<String> Arguments;
			/**
			 *	The raw string that was parsed to
			 *	initiate this event.
			 */
			String Raw;
			/**
			 *	The raw arguments parsed into the
			 *	\em Arguments vector.
			 */
			String RawArguments;
	
	
	};
	
	
	/**
	 *	Represents the result of attempting
	 *	to execute a command.
	 */
	enum class CommandStatus {
	
		Success,		/**<	The command was successful.	*/
		SyntaxError,	/**<	There was a syntax error.	*/
		DoesNotExist,	/**<	The requested command does not exist.	*/
		Forbidden		/**<	User is not permitted to do that.	*/
	
	};
	
	
	/**
	 *	Represents the result of executing a command.
	 */
	class CommandResult {
	
	
		public:
		
		
			/**
			 *	Represents the status the execution completed
			 *	with.
			 */
			CommandStatus Status;
			/**
			 *	A ChatMessage which represents the response
			 *	that should be sent to the user (if any).
			 */
			ChatMessage Message;
	
	
	};
	
	
	/**
	 *	Defines the interface to which
	 *	commands must adhere.
	 */
	class Command {
	
	
		public:
		
		
			/**
			 *	Gets a summary entry for this command.
			 *
			 *	Summary entries should provide a brief overview
			 *	of a command's purpose.
			 *
			 *	\param [in] identifier
			 *		The identifier to get the summary for.  Can
			 *		be ignored if the implementing command object
			 *		only implements a single identifier.
			 *	\param [in] message
			 *		A ChatMessage object to which the summary
			 *		for this command shall be appended.
			 */
			virtual void Summary (const String & identifier, ChatMessage & message) = 0;
		
		
			/**
			 *	Gets a help entry for this command.
			 *
			 *	Help entries should document the syntax
			 *	and purpose of a command fully.
			 *
			 *	\param [in] identifier
			 *		The identifier to get help for.  Can
			 *		be ignored if the implementing command
			 *		object only implements a single identifier.
			 *	\param [in] message
			 *		A ChatMessage object to which the help
			 *		for this command shall be appended.
			 */
			virtual void Help (const String & identifier, ChatMessage & message) = 0;
			
			
			/**
			 *	Attempts to get autocompletions.
			 *
			 *	Default implementation returns no auto
			 *	completions.
			 *
			 *	\param [in] event
			 *		A CommandEvent representing what the user
			 *		has already typed.
			 *
			 *	\return
			 *		A vector of string representing possible
			 *		auto completions for the last work the user
			 *		typed.
			 */
			virtual Vector<String> AutoComplete (const CommandEvent & event);
		
		
			/**
			 *	Checks to see if a given command event should
			 *	be permitted to proceed.
			 *
			 *	Default implementation allows all commands to
			 *	proceed.
			 *
			 *	\param [in] event
			 *		The CommandEvent encapsulating the command
			 *		event to check.
			 *
			 *	\return
			 *		\em true if \em event should be allowed to
			 *		proceed, \em false otherwise.
			 */
			virtual bool Check (const CommandEvent & event);
			
			
			/**
			 *	Executes the command.
			 *
			 *	\param [in] event
			 *		A CommandEvent encapsulating the command event
			 *		to execute.
			 *
			 *	\return
			 *		A CommandResult object encapsulating the
			 *		result of executing the command.
			 */
			virtual CommandResult Execute (CommandEvent event) = 0;
	
	
	};


	/**
	 *	Handles functionality related to
	 *	commands, including displaying help
	 *	for them, autocompletion, and invoking
	 *	them as necessary.
	 */
	class Commands : public CommandInterpreter, public Module {
	
	
		private:
		
		
			std::unordered_map<
				String,
				Command *
			> map;
			Vector<
				Tuple<
					String,
					Command *
				>
			> list;
			
			
			Command * get (const String &);
			
			
			static ChatMessage incorrect_syntax ();
			static ChatMessage incorrect_syntax (const String &);
			static ChatMessage command_dne ();
			static ChatMessage insufficient_privileges ();
			
			
			ChatMessage help (CommandEvent);
			Nullable<CommandEvent> parse (SmartPointer<Client>, const String &, bool keep_trailing=false);
			Nullable<ChatMessage> execute (SmartPointer<Client>, const String &);
			Vector<String> auto_complete (SmartPointer<Client>, const String &);
		
		
		public:
		
		
			/**
			 *	Retrieves a reference to a valid
			 *	instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance
			 *		of this class.
			 */
			static Commands & Get () noexcept;
		
		
			/**
			 *	\cond
			 */
		
		
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			virtual Nullable<String> operator () (const String &) override;
			
			
			/**
			 *	\endcond
			 */
			/**
			 *	Adds a new command which the module
			 *	will manage.
			 *
			 *	Not thread safe, do not invoke after
			 *	start up.
			 *
			 *	\param [in] identifier
			 *		The identifier by which this Command
			 *		object shall be identified.
			 *	\param [in] command
			 *		A pointer to the Command object
			 *		to be managed.
			 */
			void Add (String identifier, Command * command);
	
	
	};


}

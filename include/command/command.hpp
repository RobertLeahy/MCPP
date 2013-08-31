/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>


namespace MCPP {
	
	
	/**
	 *	Defines the interface to which
	 *	commands must adhere.
	 */
	class Command {
	
	
		public:
		
		
			/**
			 *	Retrieves this command's identifier.
			 *	The identifier is the string after
			 *	the solidus at the beginning of a chat
			 *	message which shall identify this command.
			 *
			 *	\return
			 *		A reference to an immutable string
			 *		containing this command's identifier.
			 */
			virtual const String & Identifier () const noexcept = 0;
			/**
			 *	Checks to see if a given user is
			 *	permitted to execute a command.
			 *
			 *	\param [in] client
			 *		The client to check.
			 *
			 *	\return
			 *		\em true if \em client is permitted
			 *		to execute this command, \em false
			 *		otherwise.
			 */
			virtual bool Check (SmartPointer<Client> client) const = 0;
			/**
			 *	Retrieves a summary of this command's
			 *	functionality.
			 *
			 *	\return
			 *		A reference to an immutable string
			 *		containing a summary of this command's
			 *		functionality.
			 */
			virtual const String & Summary () const noexcept = 0;
			/**
			 *	Retrieves help for this command.
			 *
			 *	\return
			 *		A reference to an immutable string
			 *		containing help regarding this
			 *		command's functionality and 
			 *		accepted arguments.
			 */
			virtual const String & Help () const noexcept = 0;
			/**
			 *	Retrieves possible autocompletions
			 *	for this command.
			 *
			 *	\param [in] args
			 *		A string containing the arguments
			 *		to autocomplete.
			 *
			 *	\return
			 *		A vector of strings containing all
			 *		possible autocompletions for the
			 *		last word in \em args.
			 */
			virtual Vector<String> AutoComplete (const String & args) const = 0;
			/**
			 *	Executes this command.
			 *
			 *	\param [in] client
			 *		The client executing this command,
			 *		or \em nullptr if no client is
			 *		executing this command.
			 *	\param [in] args
			 *		A string containing the arguments
			 *		to this command.
			 *
			 *	\return
			 *		\em true if the command completed
			 *		successfully, \em false if the syntax
			 *		of \em args was invalid or incorrect.
			 */
			virtual bool Execute (SmartPointer<Client> client, const String & args) = 0;
	
	
	};
	
	
	/**
	 *	Represents the result of attempting
	 *	to execute a command.
	 */
	enum class CommandResult {
	
		Success,		/**<	The command was successful.	*/
		SyntaxError,	/**<	There was a syntax error.	*/
		DoesNotExist	/**<	The requested command does not exist.	*/
	
	};


	/**
	 *	Handles functionality related to
	 *	commands, including displaying help
	 *	for them, autocompletion, and invoking
	 *	them as necessary.
	 */
	class CommandModule : public Module {
	
	
		private:
		
		
			Vector<Command *> commands;
			
			
			inline Command * retrieve (const String &);
			inline Vector<Command *> retrieve (const Regex &);
			void incorrect_syntax (SmartPointer<Client>, Command *);
			void command_dne (SmartPointer<Client>);
			void insufficient_privileges (SmartPointer<Client>);
			void help (SmartPointer<Client>, const String &);
		
		
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			CommandModule ();
			~CommandModule () noexcept;
		
		
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	Executes a command.
			 *
			 *	\param [in] command
			 *		The command and all of its
			 *		arguments.
			 *
			 *	\return
			 *		The result of the command.
			 */
			CommandResult Execute (const String & command);
			/**
			 *	Executes a command.
			 *
			 *	\param [in] id
			 *		The identifier of the command
			 *		to execute.
			 *	\param [in] args
			 *		The arguments to pass to the
			 *		specified command.
			 *
			 *	\return
			 *		The result of the command.
			 */
			CommandResult Execute (const String & id, const String & args);
			/**
			 *	Adds a new command which the module
			 *	will manage.
			 *
			 *	Not thread safe, do not invoke after
			 *	start up.
			 *
			 *	\param [in] command
			 *		A pointer to the Command object
			 *		to be managed.
			 */
			void Add (Command * command);
	
	
	};
	
	
	/**
	 *	The single valid instance of CommandModule.
	 */
	extern Nullable<CommandModule> Commands;


}
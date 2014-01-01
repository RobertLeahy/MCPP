/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <windows.h>
#include <hash.hpp>
#include <functional>
#include <exception>
#include <unordered_map>


namespace MCPP {


	namespace CLIImpl {
	
	
		[[noreturn]]
		void Raise ();
	
	
		String GetDateTime ();


		class ConsoleScreenBuffer {


			private:
			
			
				typedef CONSOLE_SCREEN_BUFFER_INFO info_t;
			
			
				//	The output strings in the buffer
				Vector<String> output;
				//	The input currently being built
				Vector<CodePoint> input;
				//	A handle to the console
				HANDLE handle;
				//	The element in the input that the
				//	cursor is currently pointing at,
				//	may be equal to the size to point
				//	past the last element
				Word ptr;
				//	The first character of input which
				//	is displayed
				Word first;
				//	The last character of input which is
				//	displayed (exclusive)
				Word last;
				//	The number of the page we're on
				Word page;
				//	Maximum number of lines to keep in
				//	the output buffer
				Word output_max;
				//	Records the width the last time it
				//	was checked
				Word width;
				//	Records the height the last time
				//	it was checked
				Word height;
				
				
				//	STRING INFORMATION
				
				//	Determine the number of rows in a
				//	string
				inline Word num_rows (const String &);
				//	Determine the number of pages in
				//	all the strings in the output buffer
				inline Word num_pages ();
				
				
				//	DRAW
				
				//	Draws the divider
				inline void draw_divider ();
				//	Draws the input buffer
				inline void draw_input ();
				//	Draws the output buffer
				inline void draw_output ();
				//	Redraws everything
				inline void draw_impl ();
				
				
				//	Adds a string to the output
				//	buffer, pruning the buffer
				//	if necessary
				inline void add_output (String);
				
				
				//	Gets information about the
				//	screen buffer being managed
				inline void get_info (info_t &);
				//	Clears the screen
				inline void clear_impl ();
				//	Gets the dimensions of the screen
				inline void get_dimensions (Word &, Word &);
				//	Sets the cursor's position
				inline void set_cursor (Word pos);
				//	Sets up the window based on the
				//	current console viewport size
				inline void window_setup ();
				
				
			public:
			
			
				ConsoleScreenBuffer () = delete;
				ConsoleScreenBuffer (HANDLE handle);
				ConsoleScreenBuffer (HANDLE handle, Word output_max);
				
				
				void WriteLine (String str);
				void WriteLines (Vector<String> strs);
				
				
				void Clear ();
				
				
				void Home ();
				void End ();
				void Left ();
				void Right ();
				void Delete ();
				void Backspace ();
				void PageUp ();
				void PageDown ();
				void CtrlHome ();
				void CtrlEnd ();
				String Return ();
				void Add (CodePoint cp);
				void Resize ();


		};


		class Console {


			private:
			
			
				//	Set when there are lines to be
				//	written
				HANDLE queued;
				//	Queue of lines to write, if null
				//	it means that the buffer should be
				//	cleared
				Vector<Nullable<String>> queue;
				mutable Mutex lock;
				mutable CondVar wait;
				//	Callback invoked whenever a line
				//	is read from STDIN
				std::function<void (String)> callback;
				//	Maximum number of lines the output
				//	buffer will maintain
				Nullable<Word> output_max;
				//	Worker thread
				Thread t;
				//	Set when worker should shutdown
				HANDLE stop;
				//	Synchronizes startup
				Barrier barrier;
				//	Stores an exception thrown during
				//	startup (if any)
				std::exception_ptr except;
				//	Panic callback
				std::function<void (std::exception_ptr)> panic;
				
				
				void worker_func () noexcept;
				inline void handle_input (HANDLE, ConsoleScreenBuffer &);
				inline void do_output (ConsoleScreenBuffer &);
				inline void enqueue_impl (Nullable<String>);
				
				
			public:
			
			
				enum class InterruptType {
				
					Interrupt,
					Break,
					Stop
				
				};
			
			
				static void SetHandler (std::function<bool (InterruptType)> callback);
				static void UnsetHandler ();
			
			
				Console () = delete;
				Console (
					std::function<void (String)> callback,
					Nullable<Word> output_max=Nullable<Word>(),
					std::function<void (std::exception_ptr)> panic=std::function<void (std::exception_ptr)>()
				);
				~Console () noexcept;
				
				
				Console & WriteLine (String str);
				Console & WriteLines (Vector<String> strs);
				Console & Clear ();
				
				
				void Wait () const noexcept;


		};
		
		
	}
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	The interface which the CLI class uses to
	 *	interface with the server.
	 */
	class CLIProvider {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			typedef std::function<void (const String &, const Vector<String> &, const String &, const Nullable<String> &)> ChatLogType;
			typedef std::function<void (const String &, Service::LogType)> LogType;
			
			
			/**
			 *	\endcond
			 */
			
			
		private:
		
		
			LogType log;
			ChatLogType chat_log;
			mutable Mutex lock;
			
			
		protected:
		
		
			void WriteLog (const String & str, Service::LogType type) const;
			void WriteChatLog (const String & from, const Vector<String> & to, const String & msg, const Nullable<String> & notes) const;
		
		
		public:
		
		
			/**
			 *	\cond
			 */
			 
			 
			void Set (LogType, ChatLogType) noexcept;
			void Clear () noexcept;
			
			
			/**
			 *	\endcond
			 */
			
			
			/**
			 *	Invoked whenever input is read from
			 *	the command line.
			 *
			 *	Default implementation does nothing.
			 *
			 *	\param [in] in
			 *		The string that was read.
			 */
			virtual void operator () (String in);
	
	
	};


	/**
	 *	Provides a command line interface for the
	 *	server.
	 */
	class CLI {
	
	
		public:
		
		
			/**
			 *	The possible reasons the CLI stopped.
			 */
			enum class ShutdownReason {
			
				None,		/**<	Still running.	*/
				Stop,		/**<	A stop was requested.	*/
				Restart,	/**<	A restart was requested.	*/
				Panic,		/**<	Something went wrong.	*/
			
			};
	
	
		private:
		
		
			//	What kind of output is being written?
			enum class OutputType {
			
				Chat,
				Log,
				Both
			
			};
		
		
			//	Provider
			CLIProvider * provider;
			
			
			//	Shutdown/restart/wait management
			mutable Mutex lock;
			mutable CondVar wait;
			ShutdownReason reason;
			
			
			//	Console output/input manager
			Nullable<CLIImpl::Console> console;
			
			
			//	Log history
			Vector<String> log_history;
			//	Chat history
			Vector<String> chat_history;
			Mutex history_lock;
			
			
			//	Which is currently selected,
			//	chat or log?
			bool chat_selected;
			
			
			//	Panicks
			void panic (std::exception_ptr) noexcept;
			
			
			//	Updates output history
			void update_history (String, Vector<String> &);
			//	Writes output
			void output (String, OutputType);
			
			
			//	Helper for switching displays
			void switch_to (const Vector<String> &);
			//	Switches the output display
			void switch_display (bool);
			
			
			//	Handles commands
			bool command (String str);
			
			
			//	Handles user input
			void input (String);
			//	Handles chat logging
			void chat_log (const String &, const Vector<String> &, const String &, const Nullable<String> &);
			//	Handles logging
			void log (const String &, Service::LogType);
			
			
			//	Stops by transitioning into a
			//	certain state
			void stop (ShutdownReason) noexcept;
			
			
			//	Clears the provider (requires lock)
			void clear_provider () noexcept;
		
		
		public:
		
		
			/**
			 *	Creates a new command line interface.
			 */
			CLI ();
			
			
			/**
			 *	Shuts this command line interface down.
			 */
			~CLI () noexcept;
		
		
			/**
			 *	Writes a string to the command line
			 *	interface.
			 */
			void WriteLine (String str);
			/**
			 *	Changes providers.
			 *
			 *	Thread safe.
			 *
			 *	\param [in] provider
			 *		The provider to use.
			 */
			void SetProvider (CLIProvider * provider);
			/**
			 *	Clears the provider.
			 *
			 *	Thread safe.
			 */
			void ClearProvider () noexcept;
			/**
			 *	Waits for the CLI to stop.
			 *
			 *	\return
			 *		The reason the CLI stopped.
			 */
			ShutdownReason Wait () const noexcept;
			/**
			 *	Gets the CLI's current state.
			 *
			 *	\return
			 *		A ShutdownReason which describes
			 *		the CLI's current state.
			 */
			ShutdownReason GetState () const noexcept;
			/**
			 *	Resets the CLI's current state.
			 */
			void ResetState () noexcept;
			/**
			 *	Requests that the CLI shut down.
			 */
			void Shutdown () noexcept;
	
	
	};
	
	
	/**
	 *	A parsed command line argument.
	 */
	class CommandLineArgument {
	
	
		public:
		
		
			/**
			 *	The raw flag as it was entered on
			 *	the command line.
			 */
			String RawFlag;
			/**
			 *	The flag as parsed.
			 */
			Nullable<String> Flag;
			/**
			 *	The arguments attached to the flag.
			 */
			Vector<String> Arguments;
	
	
	};
	
	
	/**
	 *	Parses command line arguments.
	 *
	 *	\param [in] args
	 *		The command line arguments.
	 *	\param [in] callback
	 *		A callback which shall be invoked
	 *		each time command line arguments
	 *		are parsed.
	 */
	void ParseCommandLineArguments (const Vector<const String> & args, std::function<void (CommandLineArgument)> callback);


}

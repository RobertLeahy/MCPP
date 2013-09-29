#pragma once


#include <rleahylib/rleahylib.hpp>
#include <exception>
#include <functional>
#include <system_error>


#include <windows.h>


__attribute__((noreturn))
inline void raise () {

	throw std::system_error(
		std::error_code(
			GetLastError(),
			std::system_category()
		)
	);

}


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

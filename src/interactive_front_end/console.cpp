#include "console.hpp"
#include <stdexcept>
#include <type_traits>
#include <utility>


static std::function<bool (Console::InterruptType)> handler;


static BOOL WINAPI handler_impl (DWORD type) noexcept {

	try {
	
		if (handler) {
		
			Console::InterruptType param;
			switch (type) {
			
				case CTRL_C_EVENT:
					param=Console::InterruptType::Interrupt;
					break;
				case CTRL_BREAK_EVENT:
					param=Console::InterruptType::Break;
					break;
				default:
					param=Console::InterruptType::Stop;
					break;
			
			}
			
			return handler(param);
		
		}
	
	} catch (...) {	}
	
	return false;

}


void Console::SetHandler (std::function<bool (InterruptType)> callback) {

	handler=std::move(callback);
	
	if (!SetConsoleCtrlHandler(
		handler_impl,
		true
	)) raise();

}


void Console::UnsetHandler () {

	if (!SetConsoleCtrlHandler(
		handler_impl,
		false
	)) raise();

}


inline HANDLE create_event () {

	HANDLE retr=CreateEvent(
		nullptr,
		true,
		false,
		nullptr
	);
	
	if (retr==nullptr) raise();
	
	return retr;

}


Console::Console (
	std::function<void (String)> callback,
	Nullable<Word> output_max,
	std::function<void (std::exception_ptr)> panic
) : callback(std::move(callback)), output_max(std::move(output_max)), barrier(2), panic(std::move(panic)) {

	//	Prepare events
	queued=create_event();
	
	try {
	
		stop=create_event();
		
		try {
		
			//	Start worker
			t=Thread([this] () mutable {	worker_func();	});
			
			//	Wait for worker thread
			//	to start
			barrier.Enter();
			
			//	Did startup fail?
			if (except) {
			
				//	YES, join worker and
				//	throw
				
				t.Join();
				
				std::rethrow_exception(except);
			
			}
		
		} catch (...) {
		
			CloseHandle(stop);
			
			throw;
		
		}
	
	} catch (...) {
	
		CloseHandle(queued);
		
		throw;
	
	}

}


Console::~Console () noexcept {

	//	Wait for worker to finish
	//	writes
	lock.Acquire();
	while (queue.Count()!=0) wait.Sleep(lock);
	lock.Release();

	//	Stop worker
	SetEvent(stop);
	t.Join();
	
	//	Cleanup events
	CloseHandle(stop);
	CloseHandle(queued);

}


static const char * std_unavail="Standard handle could not be acquired";


static inline HANDLE get_std_handle (DWORD handle) {

	HANDLE retr=GetStdHandle(handle);
	
	//	Failure
	if (retr==INVALID_HANDLE_VALUE) raise();
	//	Handle does not exist/is unavailable
	if (retr==nullptr) throw std::runtime_error(std_unavail);
	
	return retr;

}


void Console::worker_func () noexcept {

	//	STDIN
	HANDLE in;
	//	Screen buffer
	Nullable<ConsoleScreenBuffer> buffer;
	//	Console mode when this thread
	//	started
	DWORD mode;

	try {
	
		//	Get handles
		in=get_std_handle(STD_INPUT_HANDLE);
		
		if (!(
			//	Attempt to cache old console
			//	mode so that it can be restored
			//	when this thread exits
			GetConsoleMode(
				in,
				&mode
			) &&
			//	Attempt to set new console mode,
			//	enabling the receipt of resize
			//	messages
			SetConsoleMode(
				in,
				mode|ENABLE_WINDOW_INPUT
			)
		)) raise();
		
		//	Create buffer
		try {
		
			HANDLE out=get_std_handle(STD_OUTPUT_HANDLE);
		
			if (output_max.IsNull()) buffer.Construct(out);
			else buffer.Construct(out,*output_max);
			
		} catch (...) {
		
			//	Restore original console
			//	mode on failure
			
			SetConsoleMode(
				in,
				mode
			);
			
			throw;
		
		}
	
	} catch (...) {
	
		//	Startup failed, notify
		//	constructor
		
		except=std::current_exception();
		
		barrier.Enter();
		
		//	ABORT
		return;
	
	}
	
	//	Startup success, notify
	//	constructor
	barrier.Enter();
	
	//	Prepare handles to wait
	//	on
	HANDLE handles []={stop,in,queued};
	
	try {
	
		//	Worker loop
		for (;;) {
		
			//	Wait
			auto result=WaitForMultipleObjects(
				static_cast<DWORD>(std::extent<decltype(handles)>::value),
				handles,
				false,
				INFINITE
			);
			
			//	Did the wait fail?
			if (result==WAIT_FAILED) raise();
			
			//	Was the wait awoken by a
			//	command to stop?
			if (result==WAIT_OBJECT_0) break;
			
			//	Was the wait awoken to notify
			//	us that there's console events
			//	to process?
			if (result==(WAIT_OBJECT_0+1)) handle_input(in,*buffer);
			//	Otherwise it must be because
			//	there are lines to write
			else do_output(*buffer);
		
		}
	
	} catch (...) {
	
		//	PANIC
		
		if (panic) try {
		
			panic(std::current_exception());
		
		//	Can't handle this
		} catch (...) {	}
	
	}
	
	//	Restore original console
	//	mode before we terminate
	SetConsoleMode(
		in,
		mode
	);

}


inline void Console::do_output (ConsoleScreenBuffer & buffer) {

	//	The strings to write
	Vector<String> write;
	//	Whether or not we must
	//	lead with a clear
	bool clear=false;
	
	lock.Execute([&] () mutable {
	
		for (auto & str : queue) {
		
			if (str.IsNull()) {
			
				//	Clear
				
				clear=true;
				
				//	Don't even bother
				//	writing out these
				//	lines, they'll just
				//	get immediately cleared
				//	anyway
				write.Clear();
			
			} else {
			
				//	A string to write
				
				write.EmplaceBack(
					std::move(*str)
				);
			
			}
		
		}
		
		//	We've dequeued everything,
		//	clear the queue
		queue.Clear();
		
		//	Wake waiting threads
		wait.WakeAll();
		
		//	Reset event so we don't
		//	loop infinitely
		if (!ResetEvent(queued)) raise();
	
	});
	
	//	Clear console if necessary
	if (clear) buffer.Clear();
	
	//	Write strings
	buffer.WriteLines(std::move(write));

}


static inline bool ctrl_pressed (const INPUT_RECORD & ir) {

	return !(
		((ir.Event.KeyEvent.dwControlKeyState&LEFT_CTRL_PRESSED)==0) &&
		((ir.Event.KeyEvent.dwControlKeyState&RIGHT_CTRL_PRESSED)==0)
	);

}


inline void Console::handle_input (HANDLE in, ConsoleScreenBuffer & buffer) {
	
	DWORD num;
	INPUT_RECORD ir;
	if (!ReadConsoleInputW(
		in,
		&ir,
		1,
		&num
	)) raise();
	
	//	Nothing was read
	if (num==0) return;
	
	//	If a resize event was read,
	//	handle and return
	if (ir.EventType==WINDOW_BUFFER_SIZE_EVENT) {
	
		buffer.Resize();
		
		return;
	
	}
	
	//	We're only concerned with key
	//	press events besides resize
	//	events
	if (!(
		(ir.EventType==KEY_EVENT) &&
		(ir.Event.KeyEvent.bKeyDown)
	)) return;
	
	//	Loop for the number of times
	//	this key press was repeated
	for (WORD i=0;i<ir.Event.KeyEvent.wRepeatCount;++i) {
	
		//	Switch depending on the key
		//	pressed
		switch (ir.Event.KeyEvent.wVirtualKeyCode) {
		
			case VK_RETURN:{
			
				//	Enter was pressed
				
				//	Get line from the screen
				String line(buffer.Return());
				//	Dispatch callback (if applicable)
				if (callback) callback(std::move(line));
				
			}break;
				
			case VK_HOME:
			
				//	Home was pressed
				
				//	Fire appropriate event
				//	depending on whether CTRL
				//	is pressed or not
				if (ctrl_pressed(ir)) buffer.CtrlHome();
				else buffer.Home();
				
				break;
				
			case VK_END:
			
				//	End was pressed
				
				//	Fire appropriate event
				//	depending on whether
				//	CTRL is pressed or not
				if (ctrl_pressed(ir)) buffer.CtrlEnd();
				else buffer.End();
				
				break;
				
			case VK_RIGHT:
			
				//	Right was pressed
				
				buffer.Right();
				
				break;
				
			case VK_LEFT:
			
				//	Left was pressed
				
				buffer.Left();
				
				break;
				
			case VK_DELETE:
			
				//	Delete was pressed
				
				buffer.Delete();
				
				break;
				
			case VK_BACK:
			
				//	Backspace was pressed
				
				buffer.Backspace();
				
				break;
				
			case VK_PRIOR:
			
				//	Page Up was pressed
				
				buffer.PageUp();
				
				break;
				
			case VK_NEXT:
			
				//	Page Down was pressed
				
				buffer.PageDown();
				
				break;
				
			default:
			
				if (ir.Event.KeyEvent.uChar.UnicodeChar!=0) buffer.Add(
					static_cast<CodePoint>(
						ir.Event.KeyEvent.uChar.UnicodeChar
					)
				);
				
				break;
		
		}
		
	}
	
}


void Console::enqueue_impl (Nullable<String> str) {

	lock.Execute([&] () mutable {
	
		//	Enqueue
		queue.EmplaceBack(std::move(str));
		
		//	Awaken worker
		if (!SetEvent(queued)) raise();
	
	});

}


Console & Console::WriteLine (String str) {

	enqueue_impl(std::move(str));
	
	return *this;

}


Console & Console::WriteLines (Vector<String> strs) {

	lock.Execute([&] () mutable {
	
		//	Enqueue all strings in
		//	the vector
		for (auto & str : strs) queue.EmplaceBack(
			std::move(str)
		);
		
		//	Awaken worker
		if (!SetEvent(queued)) raise();
	
	});
	
	return *this;

}


Console & Console::Clear () {

	enqueue_impl(Nullable<String>());
	
	return *this;

}


void Console::Wait () const noexcept {

	lock.Acquire();
	while (queue.Count()!=0) wait.Sleep(lock);
	lock.Release();

}

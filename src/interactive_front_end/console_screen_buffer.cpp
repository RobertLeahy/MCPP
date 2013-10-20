#include "console.hpp"
#include <cstring>
#include <limits>
#include <utility>


//	The number of console rows which are
//	used for the input field
static const Word input_rows=2;
//	The character that will be used to divide
//	the input area from the output area
static const CodePoint divider=0x2014;	//	EM DASH U+2014
//	Default number of lines to maintain in the
//	output buffer
static const Word default_output_max=1024;


inline void ConsoleScreenBuffer::get_info (info_t & info) {

	if (!GetConsoleScreenBufferInfo(
		handle,
		&info
	)) raise();

}


inline void ConsoleScreenBuffer::get_dimensions (Word & width, Word & height) {

	info_t info;
	get_info(info);
	
	width=static_cast<Word>(
		info.srWindow.Right-info.srWindow.Left
	)+1;
	height=static_cast<Word>(
		info.srWindow.Bottom-info.srWindow.Top
	)+1;

}


inline void ConsoleScreenBuffer::clear_impl () {

	info_t info;
	get_info(info);
	
	//	Ignored
	DWORD written;
	//	Set all character attributes to default
	if (!FillConsoleOutputAttribute(
		handle,
		info.wAttributes,
		info.dwSize.X*info.dwSize.Y,
		{0,0},
		&written
	)) raise();
	
	//	Clear output
	output.Clear();
	//	Clear input
	input=Vector<CodePoint>();
	
	draw_impl();

}


inline Word ConsoleScreenBuffer::num_rows (const String & str) {

	//	If there are no code points in
	//	the string, it still takes up
	//	one row
	if (str.Size()==0) return 1;

	Word width;
	Word height;
	get_dimensions(width,height);
	
	//	Degenerate case: No width
	if (width==0) return 0;
	
	Word retr=str.Size()/width;
	//	If there's a remainder we
	//	take up an extra row
	if ((str.Size()%width)!=0) ++retr;
	
	return retr;

}


inline Word ConsoleScreenBuffer::num_pages () {

	Word rows=0;
	for (const auto & s : output) rows+=num_rows(s);
	
	//	There's always at least one
	//	page
	if (rows==0) return 1;
	
	Word width;
	Word height;
	get_dimensions(width,height);
	
	//	Get height excluding input
	//	area
	height-=input_rows;
	
	//	Avoid arithmetic error due to
	//	divide by zero
	if (height==0) return rows;
	
	Word retr=rows/height;
	//	If there's a remainder we
	//	take up an extra page
	if ((rows%height)!=0) ++retr;
	
	return retr;

}


inline void ConsoleScreenBuffer::draw_output () {

	Word width;
	Word height;
	get_dimensions(width,height);
	
	//	Handle degenerate cases: No columns
	//	and/or insufficient rows
	if (
		(width==0) ||
		(height<(input_rows+1))
	) return;
	
	//	Height of the output portion of
	//	the screen
	Word h=height-input_rows;
	
	//	Create a sufficiently-sized buffer
	Vector<WCHAR> buffer(
		Word(SafeWord(width)*SafeWord(h))
	);
	buffer.SetCount(buffer.Capacity());
	
	//	Initialize entire array
	//	to spaces
	for (auto & cp : buffer) cp=static_cast<WCHAR>(' ');
	
	//	Only proceed if there are output
	//	strings
	if (output.Count()!=0) {
	
		//	Work our way backwards through the
		//	list of strings to find the correct
		//	start point for this page
		Word ptr=output.Count()-1;
		//	This represents the offset from
		//	the beginning of the pointed to string
		//	where we should start copying to the
		//	output buffer
		Word str_offset=0;
		//	The number of rows we're counted off
		Word curr_rows=0;
		//	Number of rows we're aiming for
		Word target_rows=Word(SafeWord(page)+SafeWord(1))*h;
		for (;;--ptr) {
		
			//	Investigate this string
			
			//	Number of rows this string occupies
			Word curr=num_rows(output[ptr]);
			
			//	Will this string cause us to reach
			//	our row target?
			if ((curr_rows+curr)>=target_rows) {
			
				//	YES
				
				//	By how much?
				str_offset=Word(SafeWord(curr-(target_rows-curr_rows))*SafeWord(width));
				
				//	Set the number of rows counted
				//	of to target
				curr_rows=target_rows;
				
				//	We're done
				break;
			
			}
			
			//	Accumulate
			curr_rows+=curr;
			
			//	If we're pointing at the
			//	first string, we're done
			if (ptr==0) break;
		
		}
		
		//	Copy strings to the output buffer
		
		//	Offset into the output buffer
		//	that we're starting at (if, for
		//	example, we didn't use all the
		//	rows)
		Word i=Word(SafeWord(target_rows-curr_rows)*SafeWord(width));
		//	Loop until we've filled the
		//	whole buffer
		while (i<buffer.Count()) {
		
			//	Get a pointer to the code points
			//	in the string ptr is pointing
			//	to
			const auto * cp_ptr=output[ptr].CodePoints().begin();
			
			//	Loop through this string,
			//	adding code points to the
			//	output buffer
			for (
				Word n=str_offset;
				(n<output[ptr].Size()) &&
				(i<buffer.Count());
				++n
			) {
			
				CodePoint cp=cp_ptr[n];
				
				//	Choose a substitution code point
				//	if this code point is out of
				//	range
				buffer[i++]=(cp>std::numeric_limits<WCHAR>::max()) ? static_cast<WCHAR>(' ') : static_cast<WCHAR>(cp);
			
			}
			
			//	Reset offset -- we've handled
			//	the first string, subsequent
			//	strings are just printed starting
			//	from the beginning
			str_offset=0;
			
			//	If the string has zero length,
			//	move to the next row
			if (output[ptr].Size()==0) {
			
				i+=width;
				
			//	Otherwise complete this row unless
			//	we somehow ended at the end of
			//	a row
			} else if ((i%width)!=0) {
			
				i/=width;
				++i;
				i*=width;
			
			}
			
			//	Move to the next string
			++ptr;
		
		}
		
	}
	
	//	Write the output buffer to
	//	the console
	DWORD written;	//	Ignored
	if (!WriteConsoleOutputCharacterW(
		handle,
		reinterpret_cast<LPCWSTR>(
			static_cast<WCHAR *>(
				buffer
			)
		),
		DWORD(SafeWord(buffer.Count())),
		{0,0},
		&written
	)) raise();

}


inline void ConsoleScreenBuffer::draw_divider () {

	Word width;
	Word height;
	get_dimensions(width,height);
	
	//	Handle degenerate cases: No columns
	//	and/or insufficient rows
	if (
		(width==0) ||
		(height<input_rows)
	) return;
	
	//	Create a buffer to write to the
	//	screen and populate it with the
	//	divider character
	Vector<WCHAR> buffer(width);
	buffer.SetCount(buffer.Capacity());
	for (auto & cp : buffer) cp=static_cast<WCHAR>(divider);
	
	//	Write
	DWORD written;	//	Ignored
	if (!WriteConsoleOutputCharacterW(
		handle,
		reinterpret_cast<LPCWSTR>(
			static_cast<WCHAR *>(
				buffer
			)
		),
		DWORD(SafeWord(buffer.Count())),
		{
			0,
			static_cast<SHORT>(height-2)
		},
		&written
	)) raise();

}


inline void ConsoleScreenBuffer::draw_input () {

	Word width;
	Word height;
	get_dimensions(width,height);
	
	//	Handle degenerate cases: No columns
	//	and/or no rows
	if (
		(width==0) ||
		(height==0)
	) return;
	
	//	Create a buffer
	Vector<WCHAR> buffer(width);
	buffer.SetCount(buffer.Capacity());
	
	//	Copy the input buffer into
	//	that buffer
	Word offset=0;
	for (Word i=first;i<last;++i) {
	
		CodePoint cp=input[i];
		
		//	Replace out of range code points
		//	with a space
		buffer[offset++]=(cp>std::numeric_limits<WCHAR>::max()) ? static_cast<WCHAR>(' ') : static_cast<WCHAR>(cp);
	
	}
	
	//	Fill the remaining space with
	//	spaces
	while (offset<buffer.Count()) buffer[offset++]=static_cast<WCHAR>(' ');
	
	//	Write
	DWORD written;	//	Ignored
	if (!WriteConsoleOutputCharacterW(
		handle,
		reinterpret_cast<LPCWSTR>(
			static_cast<WCHAR *>(
				buffer
			)
		),
		DWORD(SafeWord(buffer.Count())),
		{
			0,
			static_cast<SHORT>(height-1)
		},
		&written
	)) raise();

}


inline void ConsoleScreenBuffer::draw_impl () {

	draw_output();
	draw_divider();
	draw_input();

}


inline void ConsoleScreenBuffer::set_cursor (Word pos) {

	Word width;
	Word height;
	get_dimensions(width,height);

	if (!SetConsoleCursorPosition(
		handle,
		{
			static_cast<SHORT>(pos),
			static_cast<SHORT>(height-1)
		}
	)) raise();

}


inline void ConsoleScreenBuffer::window_setup () {

	info_t info;
	get_info(info);
	
	//	Contains the coordinates of the
	//	window when it's scrolled all the
	//	way to the top
	SMALL_RECT loc{
		0,
		0,
		static_cast<SHORT>(
			info.srWindow.Right-info.srWindow.Left
		),
		static_cast<SHORT>(
			info.srWindow.Bottom-info.srWindow.Top
		)
	};
	
	if (loc.Right>=info.dwMaximumWindowSize.X) loc.Right=info.dwMaximumWindowSize.X-1;
	if (loc.Bottom>=info.dwMaximumWindowSize.Y) loc.Bottom=info.dwMaximumWindowSize.Y-1;
	
	//	Bring the window "home"
	if (!SetConsoleWindowInfo(
		handle,
		true,
		&loc
	)) raise();

}


ConsoleScreenBuffer::ConsoleScreenBuffer (HANDLE handle) : handle(handle), ptr(0), first(0), last(0), page(0), output_max(default_output_max), width(0), height(0) {

	//	Setup the viewport
	window_setup();
	
	//	Bring the cursor "home"
	set_cursor(0);
	
	//	Clear the screen
	clear_impl();

}


ConsoleScreenBuffer::ConsoleScreenBuffer (HANDLE handle, Word output_max) : ConsoleScreenBuffer(handle) {

	this->output_max=output_max;

}


inline void ConsoleScreenBuffer::add_output (String str) {

	//	Check to make sure output
	//	buffer isn't full
	if (
		//	Zero means unlimited
		(output_max!=0) &&
		(output.Count()==output_max)
	) {
	
		//	Delete the oldest
		//	string
		output.Delete(0);
	
	}
	
	//	Process the string so it's suitable for
	//	display
	Vector<CodePoint> substring;
	bool added=false;
	for (auto cp : str.CodePoints()) {
	
		switch (cp) {
		
			//	Skip vertical tabs and
			//	carriage returns
			case '\v':
			case '\r':
				break;
			
			//	End lines at newline
			case '\n':
				output.EmplaceBack(std::move(substring));
				added=true;
				continue;
				
			//	Replace tabs with four
			//	spaces
			case '\t':
				for (Word i=0;i<4;++i) substring.Add(
					static_cast<CodePoint>(' ')
				);
				break;
				
			//	Copy everything else verbatim
			default:
				substring.Add(cp);
				break;
		
		}
		
		added=false;
	
	}
	
	//	If there's something left to be
	//	added, add it
	if (!added) output.EmplaceBack(std::move(substring));

}


void ConsoleScreenBuffer::WriteLine (String str) {

	add_output(std::move(str));
	
	draw_output();

}


void ConsoleScreenBuffer::WriteLines (Vector<String> strs) {

	for (auto & str : strs) add_output(std::move(str));
	
	draw_output();

}


void ConsoleScreenBuffer::Clear () {

	//	Clear buffers
	output.Clear();
	
	draw_output();

}


void ConsoleScreenBuffer::Home () {

	Word width;
	Word height;
	get_dimensions(width,height);

	//	Pointing at the first character
	ptr=0;
	//	First character displayed is the
	//	first character
	first=0;
	//	Last character displayed is either
	//	the last character in the string, or
	//	the last character which will fit
	//	onto the width of the console
	last=(width>input.Count()) ? input.Count() : width;
	
	draw_input();
	
	//	Bring the cursor home
	set_cursor(0);

}


void ConsoleScreenBuffer::End () {

	Word width;
	Word height;
	get_dimensions(width,height);
	
	//	One past the end
	ptr=input.Count();
	
	//	If the whole string fits on
	//	the screen, display all characters,
	//	otherwise display as many as
	//	will fit, minus 1 (a blank space
	//	for text to be typed into)
	first=(input.Count()<width) ? 0 : (input.Count()-(width-1));
	//	Always display the last character
	//	since we're at the end of the
	//	input string
	last=input.Count();
	
	draw_input();
	
	//	Set the cursor to the end
	//	of the string
	set_cursor((input.Count()<width) ? input.Count() : (width-1));

}


void ConsoleScreenBuffer::Left () {

	//	If the cursor is already at
	//	its leftmost, nothing happens
	if (ptr==0) return;

	Word width;
	Word height;
	get_dimensions(width,height);
	
	--ptr;
	
	//	We only have to adjust the visible
	//	text if we moved past the first
	//	visible code point
	if (ptr<first) {
	
		//	Adjust visible characters within
		//	the string
		if ((last-first)==width) --last;
		--first;
		
		draw_input();
	
	}
	
	//	Set the cursor's position
	set_cursor(ptr-first);

}


void ConsoleScreenBuffer::Right () {

	//	If the cursor is already at its
	//	rightmost, nothing happens
	if (ptr==input.Count()) return;
	
	Word width;
	Word height;
	get_dimensions(width,height);
	
	++ptr;
	
	//	We only have to adjust the visible
	//	text if we moved past the last
	//	visible code point
	if (ptr==input.Count()) {
	
		++first;
		
		draw_input();
	
	} else if (ptr==last) {
	
		++last;
		++first;
		
		draw_input();
	
	}
	
	//	Set the cursor's position
	set_cursor(ptr-first);

}


String ConsoleScreenBuffer::Return () {

	Word width;
	Word height;
	get_dimensions(width,height);
	
	ptr=0;
	first=0;
	last=0;
	draw_input();
	
	set_cursor(0);
	
	return String(std::move(input));

}


void ConsoleScreenBuffer::Add (CodePoint cp) {

	Word width;
	Word height;
	get_dimensions(width,height);
	
	if (ptr==input.Count()) {
	
		//	Inserting at end of input
		//	string
	
		input.Add(cp);
		
		if ((last-first)==(width-1)) ++first;
		++last;
	
	} else {
	
		//	Inserting in the middle
		
		input.Insert(cp,ptr);
		
		if ((last-first)<width) {

			++last;
			
		} else if (ptr==(last-1)) {
		
			++last;
			++first;
		
		}
	
	}
	
	draw_input();
	
	++ptr;
	
	Word ptr_loc=ptr-first;
	
	if (ptr_loc>=width) {
	
		++last;
		++first;
		--ptr_loc;
	
	}
	
	set_cursor(ptr_loc);

}


void ConsoleScreenBuffer::Delete () {

	//	If we're at the end of the
	//	string, there's nothing to
	//	delete
	if (ptr==input.Count()) return;
	
	//	Delete the character we're
	//	pointing at
	input.Delete(ptr);
	if (last>input.Count()) --last;
	
	draw_input();

}


void ConsoleScreenBuffer::PageUp () {

	//	If we're on the topmost page,
	//	abort
	if (page==(num_pages()-1)) return;
	
	//	Move to a higher page
	++page;
	
	draw_output();

}


void ConsoleScreenBuffer::PageDown () {

	//	If we're on the bottommest page,
	//	abort
	if (page==0) return;
	
	//	Move to a lower page
	--page;
	
	draw_output();

}


void ConsoleScreenBuffer::CtrlHome () {

	//	Determine the number of the
	//	last page
	Word last=num_pages()-1;
	
	//	If we're already on the last page,
	//	abort
	if (page==last) return;
	
	page=last;
	
	draw_output();

}


void ConsoleScreenBuffer::CtrlEnd () {

	//	If we're already on the first page,
	//	abort
	if (page==0) return;
	
	page=0;
	
	draw_output();

}


void ConsoleScreenBuffer::Backspace () {

	//	If we're at the beginning of
	//	the string, there's nothing to
	//	remove
	if (ptr==0) return;
	
	--ptr;
	input.Delete(ptr);
	if (last>input.Count()) --last;
	if (ptr<first) {
	
		--first;
		
		Word width;
		Word height;
		get_dimensions(width,height);
		
		if ((last-first)>width) --last;
	
	}
	
	draw_input();
	
	set_cursor(ptr-first);

}


void ConsoleScreenBuffer::Resize () {
	
	//	Get the dimensions of this new
	//	size
	Word width;
	Word height;
	get_dimensions(width,height);
	
	if (
		(width==this->width) &&
		(height==this->height)
	) return;
	
	this->width=width;
	this->height=height;
	
	//	Setup the viewport for this new
	//	size
	window_setup();
	
	//	Adjust the number of pages if
	//	necessary
	Word pages=num_pages();
	if (page>=pages) page=pages-1;
	
	//	Don't bother with pointer
	//	manipulations in the degenerate
	//	case
	if (width!=0) {
	
		//	Is the input area too wide for
		//	the new screen size?
		if ((last-first)>width) {
		
			//	Set it to be exactly the right
			//	width
			last=first+width;
			
			//	Did we push the pointer off the
			//	screen?
			if (ptr>=last) {
			
				if (ptr==input.Count()) {
				
					last=input.Count();
					first=last-(width-1);
	
				
				} else {
				
					last=ptr+1;
					first=last-width;
				
				}
			
			}
		
		}
		
		set_cursor(ptr-first);
		
	}
	
	draw_output();
	draw_divider();
	draw_input();

}

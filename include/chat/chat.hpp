/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <functional>
#include <utility>


namespace MCPP {
	
	
	/**
	 *	The different styles that a
	 *	sequence in a chat message may
	 *	have.
	 */
	enum class ChatStyle : ASCIIChar {
	
		Random='k',
		Bold='l',
		Strikethrough='m',
		Underline='n',
		Italic='o',
		Black='0',
		DarkBlue='1',
		DarkGreen='2',
		DarkCyan='3',
		DarkRed='4',
		Purple='5',
		Gold='6',
		Grey='7',
		DarkGrey='8',
		Blue='9',
		BrightGreen='a',
		Cyan='b',
		Red='c',
		Pink='d',
		Yellow='e',
		White='f'
	
	};
	
	
	/**
	 *	Different commands that can be given
	 *	to the chat formatter.
	 */
	enum class ChatFormat {
	
		Push,			/**<	Pushes a style onto the stack.	*/
		Pop,			/**<	Pops a style off the stack.	*/
		Label,			/**<	Inserts an automatically generated label for this message.	*/
		Sender,			/**<	Inserts the sender's username.	*/
		Recipients,		/**<	Inserts a comma-separated list of recipients.	*/
		Segment,		/**<	Inserts a message segment.	*/
		LabelSeparator,	/**<	Inserts the label separator.	*/
		LabelStyle,		/**<	Pushes the default label style onto the stack.	*/
	
	};
	
	
	/**
	 *	A token that gives the chat formatter
	 *	a certain command.
	 */
	class ChatToken {
	
	
		private:
		
		
			inline void copy (const ChatToken &);
			inline void move (ChatToken &&) noexcept;
			inline void destroy () noexcept;
	
		
		public:
		
		
			ChatToken () = delete;
			ChatToken (const ChatToken & other);
			ChatToken (ChatToken && other) noexcept;
			ChatToken & operator = (const ChatToken & other);
			ChatToken & operator = (ChatToken && other) noexcept;
		
		
			/**
			 *	Creates a new ChatToken which pushes
			 *	a style onto the stack.
			 *
			 *	\param [in] style
			 *		The style to be pushed onto the
			 *		stack.
			 */
			ChatToken (ChatStyle style) noexcept;
			/**
			 *	Creates a new ChatToken which encapsulates
			 *	a segment of text.
			 *
			 *	\param [in] segment
			 *		The segment of text.
			 */
			ChatToken (String segment) noexcept;
			/**
			 *	Creates a new ChatToken which executes a
			 *	command.
			 *
			 *	\param [in] type
			 *		One of the ChatFormat commands.
			 */
			ChatToken (ChatFormat type) noexcept;
		
		
			/**
			 *	Cleans up this ChatToken.
			 */
			~ChatToken () noexcept;
		
		
			/**
			 *	The type of command this is.
			 */
			ChatFormat Type;
			
			
			union {
			
				/**
				 *	The segment associated with this
				 *	command, if it has one.
				 */
				String Segment;
				/**
				 *	The style associated with this
				 *	command, if it has one.
				 */
				ChatStyle Style;
			
			};
		
	
	};
	
	
	/**
	 *	Encapsulates a chat message.
	 */
	class ChatMessage {
	
	
		private:
		
		
			inline void add_recipients () const noexcept {	}
		
		
			template <typename... Args>
			void add_recipients (SmartPointer<Client> client, Args &&... args) {
			
				Recipients.Add(std::move(client));
				
				add_recipients(std::forward<Args>(args)...);
			
			}
		
		
			template <typename... Args>
			void add_recipients (String str, Args &&... args) {
			
				To.Add(std::move(str));
				
				add_recipients(std::forward<Args>(args)...);
			
			}
	
	
		public:
		
		
			/**
			 *	The sender of the message.
			 *
			 *	If null, it will be assumed
			 *	that the message is from
			 *	\"SERVER\".
			 */
			SmartPointer<Client> From;
			/**
			 *	A list of recipients of the message.
			 *
			 *	If empty, along with Recipients,
			 *	it will be assumed that the message
			 *	is a broadcast.
			 */
			Vector<String> To;
			/**
			 *	A list of recipients of the message.
			 *
			 *	If empty, along with To, it will be
			 *	assumed that the message is a
			 *	broadcast.
			 */
			Vector<SmartPointer<Client>> Recipients;
			/**
			 *	A list of ChatToken objects which
			 *	describe the message.
			 */
			Vector<ChatToken> Message;
			/**
			 *	\em true if the message is being sent
			 *	back to the original sender (as in the
			 *	case of whispers, which have to be sent
			 *	back to the sender to confirm they were
			 *	delivered), \em false otherwise.
			 */
			bool Echo;
		
		
			/**
			 *	Creates a new, empty chat message with
			 *	no recipients or sender.
			 */
			ChatMessage () noexcept = default;
			/**
			 *	Creates a default message which encapsulates
			 *	a given textual message.
			 *
			 *	\param [in] message
			 *		The text to wrap.
			 */
			ChatMessage (String message);
			/**
			 *	Creates a default message which encapsulates
			 *	a given textual message and is from a given
			 *	sender.
			 *
			 *	\param [in] from
			 *		The sender.
			 *	\param [in] message
			 *		The text to wrap.
			 */
			ChatMessage (SmartPointer<Client> from, String message);
			/**
			 *	Creates a default message which encapsulates
			 *	a given textual message, is from a given sender,
			 *	and sent to a specific recipient.
			 *
			 *	\param [in] from
			 *		The sender.
			 *	\param [in] to
			 *		The recipient.
			 *	\param [in] message
			 *		The text to wrap.
			 */
			ChatMessage (SmartPointer<Client> from, String to, String message);
			
			
			/**
			 *	Adds an arbitrary number of recipients
			 *	to the message.
			 *
			 *	\tparam Args
			 *		The types of the objects to add
			 *		as recipients.  SmartPointers to
			 *		Clients and Strings are acceptable.
			 *
			 *	\param [in] args
			 *		Arguments of type \em Args which shall
			 *		be added as recipients.
			 */
			template <typename... Args>
			void AddRecipients (Args &&... args) {
			
				add_recipients(std::forward<Args>(args)...);
			
			}
			/**
			 *	Adds a new token to the message.
			 *
			 *	\tparam T
			 *		The type of the object that
			 *		shall be passed to the ChatToken
			 *		constructor to create a token
			 *		in place at the back of the
			 *		message.
			 *
			 *	\param [in] obj
			 *		The object that shall be passed
			 *		to the constructor to create a
			 *		token in place at the back of
			 *		the message.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			template <typename T>
			ChatMessage & operator << (T && obj) {
			
				Message.EmplaceBack(std::forward<T>(obj));
				
				return *this;
			
			}
	
	
	};


	/**
	 *	The type of callback which shall be
	 *	invoked whenever a new chat message
	 *	arrives.
	 */
	typedef std::function<void (SmartPointer<Client>, const String &)> ChatHandler;


	/**
	 *	A modular framework for conveniently
	 *	implementing chat-related functionalities.
	 */
	class Chat : public Module {
	
	
		private:
		
		
			typedef PacketTypeMap<0x03> pt;
	
	
		public:
		
		
			/**
			 *	Retrieves a reference to a valid
			 *	instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance
			 *		of this class.
			 */
			static Chat & Get () noexcept;
		
		
			/**
			 *	Writes a message to the chat log.
			 *
			 *	\param [in] message
			 *		The message to log.
			 */
			static void Log (const ChatMessage & message);
			/**
			 *	Writes a message to the chat log.
			 *
			 *	\param [in] message
			 *		The message to log.
			 *	\param [in] notes
			 *		Any notes about the chat message
			 *		that should be logged alongside
			 *		the message itself.
			 */
			static void Log (const ChatMessage & message, String notes);
			/**
			 *	Writes a message to the chat log.
			 *
			 *	\param [in] message
			 *		The message to log.
			 *	\param [in] dne
			 *		A list of recipients to whom
			 *		delivery failed.
			 */
			static void Log (const ChatMessage & message, const Vector<String> & dne);
			/**
			 *	Creates a string representation of
			 *	\em message which may be sent to a
			 *	vanilla Minecraft client via a
			 *	0x03 packet.
			 *
			 *	\param [in] message
			 *		A chat message to format.
			 *	\param [in] json
			 *		If \em true JSON output shall
			 *		be generated, otherwise plain
			 *		text output shall be generated.
			 *
			 *	\return
			 *		\em message formatted as a
			 *		string.
			 */
			static String Format (const ChatMessage & message, bool json=true);
		
		
			/**
			 *	The callback that shall be invoked
			 *	when a new chat message arrives.
			 */
			ChatHandler Chat;
			
			
			/**
			 *	\cond
			 */
		
		
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
			
			
			/**
			 *	Formats and sends a chat message.
			 *
			 *	\param [in] message
			 *		The message to format and send.
			 *
			 *	\return
			 *		A list of recipients to whom the
			 *		message could not be delivered.
			 */
			Vector<String> Send (const ChatMessage & message);
	
	
	};


}

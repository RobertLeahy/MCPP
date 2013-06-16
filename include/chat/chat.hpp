/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <functional>


namespace MCPP {


	/**
	 *	A moduler framework for conveniently
	 *	implementing chat-related functionalities.
	 */
	class ChatModule : public Module {
	
	
		private:
		
		
			ModuleLoader mods;
	
	
		public:
		
		
			/**
			 *	Sanitizes \em subject, ensuring that
			 *	it is safe for transmission.
			 *
			 *	As a result of Notch's enlightened
			 *	protocol design, the section sign
			 *	("§" or U+00A7) is impossible to
			 *	represent as a literal in the protocol
			 *	and will therefore simply be removed
			 *	if \em escape is set to \em true.
			 *
			 *	Due to the fact that the Minecraft
			 *	protocol prepends strings with a
			 *	16-bit signed integer representing
			 *	their length (in Unicode code points)
			 *	strings longer than 32267 characters
			 *	will be sliced to 32267 characters
			 *	in length.  Note that this slicing
			 *	process is based solely on code
			 *	paints, and may very well divide
			 *	a grapheme.
			 *
			 *	Due to the fact that the Minecraft
			 *	protocol supports only UCS-2, all
			 *	code points outside the BMP will
			 *	be removed.
			 *
			 *	The string will additionally be
			 *	placed in Normal Form Canonical
			 *	Composition (i.e.\ NFC) which will
			 *	decrease the number of bytes that
			 *	will be required to transmit it.
			 *
			 *	\param [in] subject
			 *		The string to sanitize.
			 *	\param [in] escape
			 *		If \em true the function will
			 *		remove the section sign from
			 *		the string.  Defaults to
			 *		\em true.
			 *
			 *	\return
			 *		A string fit for transmission
			 *		over the Minecraft protocol.
			 */
			static String Sanitize (String subject, bool escape=true);
		
		
			/**
			 *	The callback that shall be invoked
			 *	when a new chat message arrives.
			 */
			std::function<void (SmartPointer<Client>, const String &)> Chat;
			
			
			/**
			 *	\cond
			 */
			 
			 
			ChatModule ();
			
			
			/**
			 *	\endcond
			 */
		
		
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	Sends a message to all connected and
			 *	authenticated clients.
			 *
			 *	\param [in] message
			 *		The message to send.  This message
			 *		is sent verbatim, no further
			 *		processing is applied.
			 */
			void Broadcast (const String & message) const;
			/**
			 *	Sends a message to connected and
			 *	authenticated users with a certain username.
			 *
			 *	\param [in] usernames
			 *		A vector of usernames to send
			 *		the message to.
			 *	\param [in] message
			 *		The message to send to those
			 *		specified by \em usernames.  This
			 *		message is sent verbatim, no
			 *		further processing is
			 *		applied.
			 *
			 *	\return
			 *		A vector of strings representing the
			 *		usernames in \em usernames which
			 *		were not found.
			 */
			Vector<String> Send (const Vector<String> & usernames, const String & message) const;
	
	
	};
	
	
	/**
	 *	The currently-loaded chat module.
	 */
	extern Nullable<ChatModule> Chat;


}
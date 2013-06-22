#include <chat/chat.hpp>
#include <utility>
#include <new>


namespace MCPP {


	//
	//	CHAT TOKEN
	//


	ChatToken::ChatToken (String segment) noexcept : Type(ChatFormat::Segment), Segment(std::move(segment)) {	}
	
	
	ChatToken::ChatToken (ChatColour colour) noexcept : Type(ChatFormat::PushColour), Colour(colour) {	}
	
	
	ChatToken::ChatToken (ChatStyle style) noexcept : Type(ChatFormat::PushStyle), Style(style) {	}
	
	
	ChatToken::ChatToken (ChatFormat type) noexcept : Type(type) {
	
		if (type==ChatFormat::Segment) new (&Segment) String ();
	
	}
	
	
	inline void ChatToken::destroy () noexcept {
	
		if (Type==ChatFormat::Segment) Segment.~String();
	
	}
	
	
	inline void ChatToken::copy (const ChatToken & other) {
	
		if (Type==ChatFormat::Segment) {
		
			new (&Segment) String (other.Segment);
		
		} else if (Type==ChatFormat::PushStyle) {
		
			Style=other.Style;
		
		} else if (Type==ChatFormat::PushColour) {
		
			Colour=other.Colour;
		
		}
	
	}
	
	
	inline void ChatToken::move (ChatToken && other) noexcept {
	
		if (Type==ChatFormat::Segment) {
		
			new (&Segment) String (std::move(other.Segment));
		
		} else {
		
			copy(other);
		
		}
	
	}
	
	
	ChatToken::~ChatToken () noexcept {
	
		destroy();
	
	}
	
	
	ChatToken::ChatToken (const ChatToken & other) : Type(other.Type) {
	
		copy(other);
	
	}
	
	
	ChatToken::ChatToken (ChatToken && other) noexcept : Type(other.Type) {
	
		move(std::move(other));
	
	}
	
	
	ChatToken & ChatToken::operator = (const ChatToken & other) {
	
		if (&other!=this) {
	
			Type=other.Type;
			
			destroy();
		
			copy(other);
			
		}
		
		return *this;
	
	}
	
	
	ChatToken & ChatToken::operator = (ChatToken && other) noexcept {
	
		if (&other!=this) {
		
			Type=other.Type;
			
			destroy();
			
			move(std::move(other));
		
		}
		
		return *this;
	
	}
	
	
	//
	//	CHAT MESSAGE
	//
	
	
	ChatMessage::ChatMessage (String message) : Echo(false) {
	
		Message.EmplaceBack(ChatFormat::DefaultStyle);
		Message.EmplaceBack(ChatFormat::DefaultColour);
		Message.EmplaceBack(ChatFormat::Label);
		Message.EmplaceBack(ChatFormat::LabelSeparator);
		Message.EmplaceBack(String(" "));
		Message.EmplaceBack(std::move(message));
	
	}
	
	
	ChatMessage::ChatMessage (SmartPointer<Client> from, String message) : From(std::move(from)), Echo(false) {
	
		Message.EmplaceBack(ChatFormat::DefaultStyle);
		Message.EmplaceBack(ChatFormat::DefaultColour);
		Message.EmplaceBack(ChatFormat::Label);
		Message.EmplaceBack(ChatFormat::LabelSeparator);
		Message.EmplaceBack(String(" "));
		Message.EmplaceBack(std::move(message));
	
	}
	
	
	ChatMessage::ChatMessage (SmartPointer<Client> from, String to, String message) : From(std::move(from)), Echo(false) {
	
		To.EmplaceBack(std::move(to));
	
		Message.EmplaceBack(ChatFormat::DefaultStyle);
		Message.EmplaceBack(ChatFormat::DefaultColour);
		Message.EmplaceBack(ChatFormat::Label);
		Message.EmplaceBack(ChatFormat::LabelSeparator);
		Message.EmplaceBack(String(" "));
		Message.EmplaceBack(std::move(message));
	
	}


}

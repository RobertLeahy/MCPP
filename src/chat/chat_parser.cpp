#include <chat/chat.hpp>
#include <limits>
#include <utility>


namespace MCPP {


	static const String from_server("SERVER");
	static const GraphemeCluster escape(static_cast<CodePoint>(0x00A7));
	static const String whispers("whispers");
	static const String whisper("whisper");
	static const String label_separator(":");
	static const String you("You");
	static const String recipient_separator(", ");
	static const String could_not_be_delivered("Delivery failed to the following: ");
	
	
	static inline void insert_sorted (Vector<String> & list, String insert) {
	
		String insert_to_lower(insert.ToLower());
	
		for (Word i=0;i<list.Count();++i) {
		
			String curr_to_lower(list[i].ToLower());
			
			if (insert_to_lower==curr_to_lower) return;
			
			if (curr_to_lower>insert_to_lower) {
			
				list.Insert(
					std::move(insert),
					i
				);
				
				return;
			
			}
		
		}
		
		list.Add(std::move(insert));
	
	}
	
	
	static inline Vector<String> sorted_list (const Vector<String> & to, const Vector<SmartPointer<Client>> & recipients) {
	
		Vector<String> sorted;
	
		for (auto & s : to) insert_sorted(
			sorted,
			s
		);
		
		for (auto & c : recipients) insert_sorted(
			sorted,
			c->GetUsername()
		);
		
		return sorted;
	
	}
	
	
	static inline Tuple<String,Vector<String>,String> log_prep (const ChatMessage & message) {
	
		String from=message.From.IsNull() ? from_server : message.From->GetUsername().ToLower();
		
		Vector<String> to(
			sorted_list(
				message.To,
				message.Recipients
			)
		);
		
		String output;
		for (const auto & token : message.Message) {
		
			if (token.Type==ChatFormat::Segment) output << token.Segment;
		
		}
		
		return Tuple<String,Vector<String>,String>(
			std::move(from),
			std::move(to),
			std::move(output)
		);
	
	}
	
	
	void ChatModule::Log (const ChatMessage & message) {
	
		auto t=log_prep(message);
		
		Nullable<String> notes;	//	This stays null
		
		RunningServer->WriteChatLog(
			t.Item<0>(),
			t.Item<1>(),
			t.Item<2>(),
			notes
		);
	
	}
	
	
	void ChatModule::Log (const ChatMessage & message, String notes) {
	
		auto t=log_prep(message);
		
		Nullable<String> notes_copy(std::move(notes));
		
		RunningServer->WriteChatLog(
			t.Item<0>(),
			t.Item<1>(),
			t.Item<2>(),
			notes_copy
		);
	
	}
	
	
	void ChatModule::Log (const ChatMessage & message, const Vector<String> & dne) {
	
		auto t=log_prep(message);
		
		Nullable<String> dne_str;
		if (dne.Count()!=0) {
		
			dne_str.Construct();
			
			*dne_str << could_not_be_delivered;
			
			for (Word i=0;i<dne.Count();++i) {
			
				if (i!=0) *dne_str << recipient_separator;
				
				*dne_str << dne[i];
			
			}
		
		}
		
		RunningServer->WriteChatLog(
			t.Item<0>(),
			t.Item<1>(),
			t.Item<2>(),
			dne_str
		);
	
	}
	
	
	static inline String recipient_list (const Vector<String> & to, const Vector<SmartPointer<Client>> & recipients) {
	
		Vector<String> sorted=sorted_list(
			to,
			recipients
		);
		
		String output;
		
		for (Word i=0;i<sorted.Count();++i) {
		
			if (i!=0) output << recipient_separator;
			
			output << sorted[i];
		
		}
		
		return output;
	
	}
	
	
	String ChatModule::Format (const ChatMessage & message) {
	
		//	Whether we're in the text
		//	property of a JSON object
		bool in_text=false;
		//	Specifies whether any actual
		//	text was generated
		bool did_output=false;
		//	Specifies how deep in the JSON
		//	object we are in terms of nested
		//	JSON objects.
		Word stack_depth=0;
		
		//	String into which output
		//	will be generated
		String output("{");
		
		auto generate_style=[&] (ChatStyle style) {
		
			//	We always generate a nested
			//	object for styles, which allows
			//	us to always return to the default
			//	style
			
			//	If we're in the text property, we
			//	need to output a separator
			if (in_text) {
			
				output << ",";
			
			//	Otherwise we need to introduce the
			//	text property
			} else {
			
				//	WORKAROUND
				//
				//	Stack-based formatting does
				//	not work unless the base level
				//	has a "color" property (Mojang...)
				if (stack_depth==0) output << "\"color\":\"white\"";
				
				output << ",";
			
				/*
				 *	This code will be needed if the
				 *	stack-based formatting bug is
				 *	fixed
				 *
				//	If we're in a nested object,
				//	we need to output a separator
				if (stack_depth!=0) output << ",";
				*/
				
				//	Output the text property
				output << "\"text\":[";
			
			}
			
			//	We're now in a new JSON object,
			//	therefore the next text element will
			//	be the first, and we're not in
			//	thet text property
			in_text=false;
			
			//	We're deeper in the tree now
			++stack_depth;
			
			//	Begin the new object
			output << "{\"";
			
			//	What we do here depends on whether
			//	the style being applied is a colour
			//	or a style
			switch (style) {
			
				//	Style
				case ChatStyle::Random:
				case ChatStyle::Bold:
				case ChatStyle::Strikethrough:
				case ChatStyle::Underline:
				case ChatStyle::Italic:{
				
					//	We have to choose the correct
					//	property
					switch (style) {
					
						case ChatStyle::Random:
							output << "random";
							break;
						case ChatStyle::Bold:
							output << "bold";
							break;
						case ChatStyle::Strikethrough:
							output << "strikethrough";
							break;
						case ChatStyle::Underline:
							output << "underline";
							break;
						case ChatStyle::Italic:
						default:
							output << "italic";
							break;
					
					}
					
					//	Set it to true
					output << "\":true";
				
				}break;
				
				//	Colour
				default:{
				
					//	We just use the "color" property
					//	for colours
					output << "color\":\"";
					
					//	Choose the correct value
					switch (style) {
					
						case ChatStyle::Black:
							output << "black";
							break;
						case ChatStyle::DarkBlue:
							output << "dark blue";
							break;
						case ChatStyle::DarkGreen:
							output << "dark green";
							break;
						case ChatStyle::DarkCyan:
							output << "dark cyan";
							break;
						case ChatStyle::DarkRed:
							output << "dark red";
							break;
						case ChatStyle::Purple:
							output << "purple";
							break;
						case ChatStyle::Gold:
							output << "gold";
							break;
						case ChatStyle::Grey:
							output << "gray";
							break;
						case ChatStyle::DarkGrey:
							output << "dark grey";
							break;
						case ChatStyle::Blue:
							output << "blue";
							break;
						case ChatStyle::BrightGreen:
							output << "green";
							break;
						case ChatStyle::Cyan:
							output << "cyan";
							break;
						case ChatStyle::Red:
							output << "red";
							break;
						case ChatStyle::Pink:
							output << "pink";
							break;
						case ChatStyle::Yellow:
							output << "yellow";
							break;
						case ChatStyle::White:
						default:
							output << "white";
							break;
					
					}
					
					output << "\"";
				
				}break;
			
			}
		
		};
		
		auto generate_string=[&] (const String & str) {
		
			//	If the string is the empty string, do
			//	nothing
			if (str.Size()==0) return;
			
			//	We output at least some text
			did_output=true;
		
			//	If we're already within a text
			//	property, we have to output
			//	a separator
			if (in_text) {
			
				output << ",";
			
			//	If we're not in a text property,
			//	we have to output a separator to
			//	separate ourselves from the property
			//	before us, UNLESS this is the bottom
			//	of the stack, in which case there
			//	isn't such a property
			//
			//	We also have to output the text property
			//	itself
			} else {
			
				//	WORKAROUND
				//
				//	Stack-based formatting does
				//	not work unless the base level
				//	has a "color" property (Mojang...)
				if (stack_depth==0) output << "\"color\":\"white\"";
				
				output << ",";
			
				/*
				 *	This code will be needed if the
				 *	stack-based formatting bug is
				 *	fixed
				 *
				//	If we're in a nested object,
				//	we need to output a separator
				if (stack_depth!=0) output << ",";
				*/
				
				//	Output the text property
				output << "\"text\":[";
				
				//	We're now in a text property
				in_text=true;
			
			}
			
			//	Begin string
			output << "\"";
			
			//	JSON has a few rules about
			//	what can appear in a string:
			//
			//	1.	Literal control characters
			//		cannot.
			//	2.	Quotes cannot unless escaped.
			//	3.	The reverse solidus cannot
			//		unless escaped.
			//
			//	An escape for the forward solidus
			//	is provided, so we'll also use that.
			for (auto cp : str.CodePoints()) {
			
				switch (cp) {
				
					case '"':
						output << "\\\"";
						break;
					case '\\':
						output << "\\\\";
						break;
					case '/':
						output << "\\/";
						break;
					case '\b':
						output << "\\b";
						break;
					case '\f':
						output << "\\f";
						break;
					case '\n':
						output << "\\n";
						break;
					case '\r':
						output << "\\r";
						break;
					case '\t':
						output << "\\t";
						break;
					default:
					
						//	Get information about the code point
						//	to check whether it's a control
						//	character or not
						const CodePointInfo * properties=CodePointInfo::GetInfo(cp);
						
						//	If we know nothing about the code point,
						//	assume it's not a control character and
						//	output it as a literal.
						//
						//	Otherwise if it's not a control character,
						//	output it as a literal.
						if (
							(properties==nullptr) ||
							(properties->Category!=GeneralCategory::Cc)
						) {
						
							output << cp;
						
						//	If it's a control character, use a \uXXXX
						//	escape to represent it
						} else {
						
							output << "\\u";
							
							String num(cp,16);
							
							for (Word i=num.Count();i<4;++i) output << "0";
							
							output << num;
						
						}
						
						break;
				
				}
			
			}
			
			//	End string
			output << "\"";
		
		};
		
		auto pop=[&] () {
		
			//	Popping while there's nothing
			//	on the stack doesn't make sense,
			//	so ignore it.
			if (stack_depth!=0) {
			
				//	If we're in the text property,
				//	close it.
				if (in_text) output << "]";
				
				//	Close the object
				output << "}";
				
				//	The only place nested objects
				//	can appear is in the text
				//	property, therefore we're guaranteed
				//	to be in the text property, and
				//	there's guaranteed to be an object
				//	ahead of us (the one we just escaped
				//	from).
				in_text=true;
				
				--stack_depth;
			
			}
		
		};
		
		//	Loop for each token
		for (const auto & token : message.Message) {
		
			switch (token.Type) {
			
				case ChatFormat::Push:
					generate_style(token.Style);
					break;
				case ChatFormat::Pop:
					pop();
					break;
				case ChatFormat::Label:{
				
					//	Labels are bold, push that
					//	style onto the stack
					generate_style(ChatStyle::Bold);
					
					String label;
					
					//	Is message from server?
					if (message.From.IsNull()) {
					
						label << from_server;
					
					} else {
					
						//	Is this a whisper?
						if (message.To.Count()==0) {
						
							//	NO
							
							label << message.From->GetUsername();
						
						} else {
						
							//	YES
							
							//	Is it a whisper back to the original
							//	sender?
							if (message.Echo) {
							
								label << you << " " << whisper << " " << recipient_list(
									message.To,
									message.Recipients
								);
							
							} else {
							
								label << message.From->GetUsername() << " " << whispers;
							
							}
						
						}
					
					}
					
					//	Generate
					generate_string(label);
					
					//	Remove bold from the stack
					pop();
				
				}break;
				case ChatFormat::Sender:
					generate_string(message.From.IsNull() ? from_server : message.From->GetUsername());
					break;
				case ChatFormat::Recipients:
					generate_string(
						recipient_list(
							message.To,
							message.Recipients
						)
					);
					break;
				case ChatFormat::Segment:
					generate_string(token.Segment);
					break;
				case ChatFormat::LabelSeparator:
					generate_style(ChatStyle::Bold);
					generate_string(label_separator);
					pop();
					break;
				case ChatFormat::LabelStyle:
					generate_style(ChatStyle::Bold);
					break;
				default:
					break;
			
			}
		
		}
		
		//	Did we output any text?
		//
		//	If not just return the empty
		//	string
		if (!did_output) return String();
		
		//	Navigate back down the stack
		for (Word i=stack_depth;;--i) {
		
			if (in_text) output << "]";
			
			output << "}";
			
			in_text=true;
		
			if (i==0) break;
		
		}
		
		//	Is the output too long?
		//
		//	If so just return the empty
		//	string
		if (output.Size()>static_cast<Word>(std::numeric_limits<Int16>::max())) return String();
		
		//	Done
		return output;
	
	}
	

}

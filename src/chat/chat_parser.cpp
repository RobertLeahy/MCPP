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
	
		//	Prepare format stacks
		Vector<ChatColour> colour_stack;
		Vector<ChatStyle> style_stack;
		
		//	Set default style and colour
		ChatColour colour=ChatColour::None;
		ChatStyle style=ChatStyle::Default;
		
		//	String into which output
		//	will be generated
		String output;
		
		//	Generator function
		auto generate=[&] (const String & segment) {
		
			ChatColour target_colour=(colour_stack.Count()==0) ? ChatColour::None : colour_stack[colour_stack.Count()-1];
			ChatStyle target_style=(style_stack.Count()==0) ? ChatStyle::Default : style_stack[style_stack.Count()-1];
			
			//	Are we changing colours?
			bool change_colour=target_colour!=colour;
			//	Are we changing styles?
			bool change_style=target_style!=style;
			
			colour=target_colour;
			style=target_style;
			
			//	The only way to get back to the default
			//	colour OR style is to output the default
			//	style.
			//
			//	Therefore we need to output the leading
			//	default style IF:
			//
			//	1.	We're changing to ChatColour::None
			//	2.	We're changing to ChatStyle::Default
			//
			//	However, setting the colour sets the
			//	default style automatically, so if we're
			//	changing colours, and NOT changing to
			//	ChatColour::None, we don't have to output
			//	this.
			bool set_default_style=(
				change_colour
					?	(colour==ChatColour::None)
					:	(
							change_style &&
							(style==ChatStyle::Default)
						)
			);
			
			if (set_default_style) output << escape << static_cast<CodePoint>(ChatStyle::Default);
			
			//	Did we set the default style?
			//
			//	If so we must set the colour
			//	UNLESS the target colour is None
			if (set_default_style) {
			
				if (colour!=ChatColour::None) output << escape << static_cast<CodePoint>(colour);
			
			//	Otherwise output the colour
			//	only if it's changed
			} else if (change_colour) {
			
				output << escape << static_cast<CodePoint>(colour);
			
			}
			
			//	If we set the default
			//	style OR we set the colour
			//	we have to output the style
			//	UNLESS it's the default
			//	style
			if (set_default_style || change_colour) {
			
				if (style!=ChatStyle::Default) output << escape << static_cast<CodePoint>(style);
			
			//	Otherwise set the style only if
			//	it's changed
			} else if (change_style) output << escape << static_cast<CodePoint>(style);
			
			//	Sanitize section signs
			//	from the output
			Vector<CodePoint> cps(segment.Size());
			for (CodePoint cp : segment.CodePoints()) if (cp!=0x00A7) cps.Add(cp);
			
			//	Add to output
			output << String(std::move(cps));
		
		};
		
		//	Loop for each token
		for (const auto & token : message.Message) {
		
			switch (token.Type) {
			
				case ChatFormat::PushStyle:
				
					style_stack.Add(token.Style);
					break;
					
				case ChatFormat::PushColour:
				
					colour_stack.Add(token.Colour);
					break;
					
				case ChatFormat::PopColour:
				
					if (colour_stack.Count()!=0) colour_stack.Delete(colour_stack.Count()-1);
					break;
					
				case ChatFormat::PopStyle:
				
					if (style_stack.Count()!=0) style_stack.Delete(style_stack.Count()-1);
					break;
					
				case ChatFormat::DefaultStyle:
				
					style_stack.Add(ChatStyle::Default);
					break;
					
				case ChatFormat::DefaultColour:
				case ChatFormat::LabelColour:
				
					colour_stack.Add(
						(message.To.Count()==0)
							?	ChatColour::None
							:	ChatColour::Pink
					);
					break;
					
				case ChatFormat::LabelStyle:
				
					//	Labels are bold
					style_stack.Add(ChatStyle::Bold);
					break;
				
				case ChatFormat::Label:{
				
					//	Labels are bold, push
					//	that style onto the stack
					style_stack.Add(ChatStyle::Bold);
					
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
					generate(label);
					
					//	Remove bold from the stack
					style_stack.Delete(style_stack.Count()-1);
					
				}break;
					
				case ChatFormat::LabelSeparator:
				
					style_stack.Add(ChatStyle::Bold);
					generate(label_separator);
					style_stack.Delete(style_stack.Count()-1);
					break;
					
				case ChatFormat::Segment:
				
					generate(token.Segment);
					break;
					
				case ChatFormat::Sender:
				
					generate(message.From.IsNull() ? from_server : message.From->GetUsername());
					break;
					
				case ChatFormat::Recipients:
				
					generate(
						recipient_list(
							message.To,
							message.Recipients
						)
					);
					break;
					
				default:break;
			
			}
		
		}
		
		//	JSON chat hack since JSON
		//	chat doesn't work properly
		String json_begin("{\"text\":\"");
		
		Word max_len=static_cast<Word>(
			std::numeric_limits<Int16>::max()
		)-json_begin.Size()-2;	//	Minus 2 for "} to end JSON
		
		//	Is the output too long?
		if (output.Size()>max_len) {
		
			Vector<CodePoint> trimmed(max_len);
			Word i=0;
			for (CodePoint cp : output.CodePoints()) {
			
				trimmed.Add(cp);
				
				if (++i==max_len) break;
			
			}
			
			//	Make sure it doesn't end with a
			//	section sign
			if (trimmed[trimmed.Count()-1]==0x00A7) trimmed.Delete(trimmed.Count()-1);
			
			output=String(std::move(trimmed));
		
		}
		
		//	Done
		return json_begin+output+"\"}";
	
	}
	

}

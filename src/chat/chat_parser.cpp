#include <chat/chat.hpp>
#include <hash.hpp>
#include <server.hpp>
#include <algorithm>
#include <unordered_map>
#include <utility>


namespace MCPP {


	static const String from_server("SERVER");
	static const String label_separator(": ");
	static const String recipient_separator(", ");
	static const String you("You");
	static const String whisper_to("whisper");
	static const String whisper_from("whispers");
	static const String could_not_be_delivered("Delivery failed to the following: ");
	
	
	class RecipientList {
	
	
		private:
		
		
			std::unordered_map<String,String> recipients;
			
			
		public:
		
		
			void Add (String add) {
			
				String lowercase(add);
				//	Special case for message from
				//	the server
				if (add!=from_server) lowercase.ToLower();
				
				if (recipients.count(lowercase)==0) recipients.emplace(
					std::move(lowercase),
					std::move(add)
				);
			
			}
			
			
			Vector<String> Get () {
			
				Vector<String> retr(recipients.size());
				
				for (auto & pair : recipients) retr.Add(std::move(pair.second));
				
				std::sort(retr.begin(),retr.end());
				
				return retr;
			
			}
	
	
	};
	
	
	static Vector<String> get_recipient_list (const ChatMessage & message) {
	
		RecipientList list;
		
		for (const auto & s : message.To) list.Add(s);
		for (const auto & c : message.Recipients) list.Add(
			c.IsNull() ? from_server : c->GetUsername()
		);
		
		return list.Get();
	
	}


	class ChatParser {
	
	
		private:
		
		
			//	The message being formatted
			const ChatMessage & message;
			//	JSON Values which are to be
			//	in the "extra" property of the
			//	root JSON object
			Vector<JSON::Value> root;
			//	A stack of JSON objects which
			//	gives our current position in
			//	the stack-based formatting
			Vector<JSON::Object> stack;
		
		
			//	Determines whether an object is "empty",
			//	i.e. has no text elements
			static bool is_empty (const JSON::Object & obj) {
			
				//	If there are no key/value pairs
				//	in the object, it's empty
				if (!obj.Pairs) return true;
				
				//	Try and find the "extra" property
				auto iter=obj.Pairs->find("extra");
				
				//	If there's no text property, it's
				//	empty, otherwise it has elements
				return iter==obj.Pairs->end();
			
			}
			
			
			//	Gets the property name associated with
			//	a given style
			static String get_style (ChatStyle style) {
			
				switch (style) {
			
					case ChatStyle::Random:return "random";
					case ChatStyle::Bold:return "bold";
					case ChatStyle::Strikethrough:return "strikethrough";
					case ChatStyle::Underline:return "underline";
					case ChatStyle::Italic:
					default:return "italic";
					
				}
			
			}
			
			
			//	Gets the colour code associated with a
			//	given colour
			static String get_colour (ChatStyle style) {
			
				switch (style) {
				
					case ChatStyle::Black:return "black";
					case ChatStyle::DarkBlue:return "dark_blue";
					case ChatStyle::DarkGreen:return "dark_green";
					case ChatStyle::DarkCyan:return "dark_aqua";
					case ChatStyle::DarkRed:return "dark_red";
					case ChatStyle::Purple:return "dark_purple";
					case ChatStyle::Gold:return "gold";
					case ChatStyle::Grey:return "gray";
					case ChatStyle::DarkGrey:return "dark_gray";
					case ChatStyle::Blue:return "blue";
					case ChatStyle::BrightGreen:return "green";
					case ChatStyle::Cyan:return "aqua";
					case ChatStyle::Red:return "red";
					case ChatStyle::Pink:return "light_purple";
					case ChatStyle::Yellow:return "yellow";
					case ChatStyle::White:
					default:return "white";
				
				}
			
			}
			
			
			//	Adds a value to the deepest object in the
			//	stack
			void add (JSON::Value add) {
			
				//	If there's nothing on the stack, we add
				//	to the root
				if (stack.Count()==0) {
				
					root.Add(std::move(add));
				
				} else {
				
					//	Get the deepest object on
					//	the stack
					
					auto & obj=stack[stack.Count()-1];
					
					decltype(obj.Pairs->find("extra")) iter;
					if (
						obj.Pairs &&
						((iter=obj.Pairs->find("extra"))!=obj.Pairs->end())
					) {
					
						//	There's already text elements, so
						//	we append
						
						iter->second.Get<JSON::Array>().Values.Add(std::move(add));
					
					} else {
					
						//	This is the first text element in
						//	this object, we have to create an
						//	array and add that to the "extra"
						//	key
						
						JSON::Array arr;
						arr.Values.Add(std::move(add));
						
						obj.Add("extra",std::move(arr));
					
					}
				
				}
			
			}
			
			
			//	Merges the topmost element of the
			//	stack down
			void merge () {
			
				//	Nothing to merge, bail out
				if (stack.Count()==0) return;
				
				//	Fetch deepest element on the
				//	stack
				Word loc=stack.Count()-1;
				
				auto obj=std::move(stack[loc]);
				stack.Delete(loc);
				
				//	If this element is empty,
				//	discard it
				if (is_empty(obj)) return;
				
				//	Merge
				add(std::move(obj));
			
			}
			
			
			//	Generates a style and pushes it
			//	onto the stack
			void push_style (ChatStyle style) {
			
				JSON::Object obj;
				obj.Add("text",String());
			
				switch (style) {
				
					//	Style
					case ChatStyle::Random:
					case ChatStyle::Bold:
					case ChatStyle::Strikethrough:
					case ChatStyle::Underline:
					case ChatStyle::Italic:
						obj.Add(get_style(style),true);
						break;
					
					//	Colour
					default:
						obj.Add("color",get_colour(style));
						break;
				
				}
				
				//	Push
				stack.Add(std::move(obj));
			
			}
			
			
			String recipients () {
				
				String retr;
				bool first=true;
				for (const auto & s : get_recipient_list(message)) {
				
					if (first) first=false;
					else retr << recipient_separator;
					
					retr << s;
				
				}
				
				return retr;
			
			}
			
			
			void label () {
			
				//	Labels are bold
				push_style(ChatStyle::Bold);
				
				String label;
				
				String from(
					message.From.IsNull()
						//	From server
						?	from_server
						//	From a client
						:	message.From->GetUsername()
				);
				
				//	Is this a whisper?
				if (
					(message.To.Count()==0) &&
					(message.Recipients.Count()==0)
				) {
				
					//	NO -- it's a broadcast
					
					label=std::move(from);
				
				} else {
				
					//	YES
					
					//	Is it an echo -- i.e. a whisper
					//	being sent back to the original
					//	sender for display?
					
					if (message.Echo) label << you << " " << whisper_to << " " << recipients();
					else label << from << " " << whisper_from;
				
				}
				
				add(std::move(label));
				
				//	Pop the bold style
				merge();
			
			}
		
		
		public:
		
		
			ChatParser (const ChatMessage & message) noexcept : message(message) {	} 
		
		
			JSON::Value operator () () {
			
				for (const auto & token : message.Message) {
				
					switch (token.Type) {
					
						case ChatFormat::Push:
							push_style(token.Style);
							break;
							
						case ChatFormat::Pop:
							merge();
							break;
							
						case ChatFormat::Label:
							label();
							break;
							
						case ChatFormat::Sender:
							add(
								message.From.IsNull()
									?	from_server
									:	message.From->GetUsername()
							);
							break;
							
						case ChatFormat::Recipients:
							add(recipients());
							break;
							
						case ChatFormat::LabelSeparator:
							push_style(ChatStyle::Bold);
							add(label_separator);
							merge();
							break;
							
						case ChatFormat::LabelStyle:
							push_style(ChatStyle::Bold);
							break;
							
						case ChatFormat::Segment:
							add(token.Segment);
							break;
							
						default:
							break;
					
					}
				
				}
				
				//	Complete JSON object by merging
				//	the stack
				while (stack.Count()!=0) merge();
				
				//	Create the root JSON object
				JSON::Array arr;
				arr.Values=std::move(root);
				JSON::Object obj;
				obj.Add(
					"extra",std::move(arr),
					"text",String()
				);
				
				return std::move(obj);
			
			}
	
	
	};


	JSON::Value Chat::Format (const ChatMessage & message) {
	
		ChatParser parser(message);
		
		return parser();
	
	}
	
	
	class ChatMessageStringifier {
	
	
		private:
		
		
			Word curr;
			Word max;
			String output;
			
			
			bool check () const noexcept {
			
				return (max==0) || (curr<=max);
			
			}
			
			
			void to_string (const JSON::Value & value) {
			
				++curr;
				
				//	Do not begin unless
				//	we're less than the maximum
				//	depth and this is actually
				//	a JSON object
				if (
					check() &&
					value.Is<JSON::Object>()
				) {
				
					auto & obj=value.Get<JSON::Object>();
					
					//	Make sure this JSON object has
					//	key/value pairs, and has a "extra"
					//	key which is an array
					decltype(obj.Pairs->find("extra")) iter;
					if (
						obj.Pairs &&
						((iter=obj.Pairs->find("extra"))!=obj.Pairs->end()) &&
						iter->second.Is<JSON::Array>()
					) for (const auto & v : iter->second.Get<JSON::Array>().Values) {
					
						if (v.Is<String>()) output << v.Get<String>();
						else to_string(v);
					
					}
				
				}
				
				--curr;
			
			}
			
			
		public:
		
		
			ChatMessageStringifier (Word max_depth) noexcept : curr(0), max(max_depth) {	}
			
			
			String operator () (const JSON::Value & value) {
			
				to_string(value);
			
				return std::move(output);
			
			}
	
	
	};
	
	
	String Chat::ToString (const JSON::Value & value, Word max_depth) {
	
		ChatMessageStringifier to_str(max_depth);
		
		return to_str(value);
	
	}
	
	
	String Chat::ToString (const ChatMessage & message) {
	
		return ToString(Format(message));
	
	}
	
	
	class LogParser {
	
	
		public:
		
		
			String From;
			String Body;
			Vector<String> To;
		
		
			LogParser (const ChatMessage & message) {
			
				To=get_recipient_list(message);
				
				From=message.From.IsNull() ? from_server : message.From->GetUsername();
				
				for (const auto & token : message.Message) {
				
					if (token.Type==ChatFormat::Segment) Body << token.Segment;
				
				}
			
			}
	
	
	};
	
	
	void Chat::Log (const ChatMessage & message) {
	
		LogParser l(message);
		
		Server::Get().WriteChatLog(
			l.From,
			l.To,
			l.Body,
			Nullable<String>()	//	Notes
		);
	
	}
	
	
	void Chat::Log (const ChatMessage & message, String notes) {
	
		LogParser l(message);
		
		Server::Get().WriteChatLog(
			l.From,
			l.To,
			l.Body,
			std::move(notes)
		);
	
	}
	
	
	void Chat::Log (const ChatMessage & message, const Vector<String> & dne) {
	
		LogParser l(message);
		
		Nullable<String> str;
		if (dne.Count()!=0) {
		
			str.Construct();
			
			*str << could_not_be_delivered;
			
			bool first=true;
			for (const auto & s : dne) {
			
				if (first) first=false;
				else *str << recipient_separator;
				
				*str << s;
			
			}
		
		}
		
		Server::Get().WriteChatLog(
			l.From,
			l.To,
			l.Body,
			str
		);
	
	}


}

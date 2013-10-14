#include <json.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <utility>


namespace JSON {


	Error::Error (const char * what) noexcept : what_str(what) {	}
	
	
	const char * Error::what () const noexcept {
	
		return what_str;
	
	}
	
	
	void Object::Construct () {
	
		Pairs=decltype(Pairs)(new decltype(Pairs)::element_type());
	
	}
	
	
	bool Object::IsNull () const noexcept {
	
		return !Pairs;
	
	}


	String Serialize (const String & str) {
	
		String retr("\"");
		
		for (auto cp : str.CodePoints()) {
		
			switch (cp) {
			
				case '"':
					retr << "\\\"";
					break;
				case '\\':
					retr << "\\\\";
					break;
				case '/':
					retr << "\\/";
					break;
				case '\b':
					retr << "\\b";
					break;
				case '\f':
					retr << "\\f";
					break;
				case '\n':
					retr << "\\n";
					break;
				case '\r':
					retr << "\\r";
					break;
				case '\t':
					retr << "\\t";
					break;
				default:{
				
					auto cpi=CodePointInfo::GetInfo(cp);
					
					if (
						(cpi!=nullptr) &&
						(cpi->Category==GeneralCategory::Cc)
					) {
					
						String hex(cp,16);
						
						Word count=hex.Count();
						if (count<4) {
						
							String pad;
							for (Word i=0;i<(4-count);++i) pad << String("0");
							
							hex=pad+hex;
						
						}
						
						retr << "\\u" << hex;
					
					} else {
					
						retr << cp;
					
					}
				
				}break;
			
			}
		
		}
		
		retr << "\"";
		
		return retr;
	
	}
	
	
	static const char * nan="Provided value is not a number";
	static const char * inf="Provided value is infinite";
	static const char * snprintf_error="snprintf returned error";
	
	
	String Serialize (Double dbl) {
	
		if (std::isnan(dbl)) throw Error(nan);
		if (!std::isfinite(dbl)) throw Error(inf);
	
		const char * format="%g";
	
		int size=std::snprintf(nullptr,0,format,dbl);
		
		if (size<0) throw Error(snprintf_error);
		
		SafeInt<int> safe_size(size);
		++safe_size;
		Word safe_w_size(safe_size);
		
		Vector<Byte> buffer(safe_w_size);
		
		if ((size=std::snprintf(
			reinterpret_cast<char *>(
				buffer.begin()
			),
			int(safe_size),
			format,
			dbl
		))<0) throw Error(snprintf_error);
		
		buffer.SetCount(safe_w_size-1);
		
		return UTF8().Decode(
			buffer.begin(),
			buffer.end()
		);
	
	}
	
	
	String Serialize (std::nullptr_t) {
	
		return String("null");
	
	}
	
	
	String Serialize (const Object & obj) {
	
		if (obj.IsNull()) return Serialize(nullptr);
	
		String retr("{");
		
		bool first=true;
		for (const auto & pair : *obj.Pairs) {
		
			if (first) first=false;
			else retr << ",";
		
			retr	<<	Serialize(pair.first)
					<<	":"
					<<	Serialize(pair.second);
		
		}
		
		retr << "}";
		
		return retr;
	
	}
	
	
	String Serialize (const Array & arr) {
	
		String retr("[");
		
		bool first=true;
		for (const auto & value : arr.Values) {
		
			if (first) first=false;
			else retr << ",";
			
			retr << Serialize(value);
		
		}
		
		retr << "]";
		
		return retr;
	
	}
	
	
	String Serialize (bool bln) {
	
		return String(bln ? "true" : "false");
	
	}
	
	
	String Serialize (const Value & value) {
	
		return (
			value.IsNull()
				?	Serialize(nullptr)
				:	(
						value.Is<String>()
							?	Serialize(value.Get<String>())
							:	(
									value.Is<Double>()
										?	Serialize(value.Get<Double>())
										:	(
												value.Is<Object>()
													?	Serialize(value.Get<Object>())
													:	(
															value.Is<Array>()
																?	Serialize(value.Get<Array>())
																:	Serialize(value.Get<bool>())
														)
											)
								)
					)
		);
	
	}
	
	
	static const char * recursion_limit_reached="Recursion limit reached";
	
	
	class ParserState {
	
	
		public:
		
		
			Word Depth;
			Word MaxDepth;
			
			
			void CheckDepth () {
			
				if (!(
					(MaxDepth==0) ||
					(Depth<=MaxDepth)
				)) throw Error(recursion_limit_reached);
			
			}
	
	
	};
	
	
	static const char * invalid_hex="Invalid hexadecimal sequence";
	
	
	static UInt16 get_hex (StringGraphemeClusterIterator & iter) {
	
		String hex;
		for (Word i=0;i<4;++i) {
		
			if (iter.IsEnd()) throw Error(invalid_hex);
			
			hex << *iter;
		
		}
		
		UInt16 retr;
		if (!hex.ToInteger(&retr,16)) throw Error(invalid_hex);
		
		return retr;
	
	}
	
	
	static const char * unterminated_string="Unterminated string";
	static const char * unrecognized_escape="Unrecognized escape sequence";
	static const char * invalid_unicode="Invalid Unicode sequence";
	
	
	static String parse_string (StringGraphemeClusterIterator & iter, ParserState & state) {
	
		String retr;
		
		for (++iter;;++iter) {
		
			//	If we found the end of the
			//	string without finding a
			//	closing quote, that's an
			//	error
			if (iter.IsEnd()) throw Error(unterminated_string);
		
			//	End on closing quote
			if (*iter=='"') break;
			
			//	Backslash escape
			if (*iter=='\\') {
			
				++iter;
				
				//	Make sure we haven't reached the end
				if (iter.IsEnd()) throw Error(unterminated_string);
				
				if (*iter=='"') retr << "\"";
				else if (*iter=='\\') retr << "\\";
				else if (*iter=='/') retr << "/";
				else if (*iter=='b') retr << "\b";
				else if (*iter=='f') retr << "\f";
				else if (*iter=='n') retr << "\n";
				else if (*iter=='r') retr << "\r";
				else if (*iter=='t') retr << "\t";
				else if (*iter=='u') {
				
					//	Unicode escape
					
					UInt16 decode [2];
					
					decode[0]=get_hex(iter);
					
					UTF16 decoder(Endianness::IsBigEndian<UInt16>());
					
					String decoded=decoder.Decode(
						reinterpret_cast<const Byte *>(decode),
						reinterpret_cast<const Byte *>(decode+1)
					);
					if (decoded.Count()==0) {
					
						if (iter.IsEnd()) throw Error(unterminated_string);
						if (*iter!='\\') throw Error(invalid_unicode);
						++iter;
						if (*iter!='u') throw Error(invalid_unicode);
						++iter;
						
						decode[1]=get_hex(iter);
						
						retr << decoder.Decode(
							reinterpret_cast<const Byte *>(decode),
							reinterpret_cast<const Byte *>(decode+2)
						);
					
					} else {
					
						retr << decoded;
					
					}
				
				} else throw Error(unrecognized_escape);
			
				continue;
			
			}
			
			//	Add grapheme verbatim
			retr << *iter;
		
		}
		
		//	Get past closing quote
		++iter;
		
		return retr;
	
	}
	
	
	static void skip_whitespace (StringGraphemeClusterIterator & iter) {
	
		for (;!iter.IsEnd() && (iter->IsWhitespace() || iter->IsLineBreak());++iter);
	
	}
	
	
	static Value parse (StringGraphemeClusterIterator &, ParserState &);
	
	
	static const char * unterminated_array="Unterminated array";
	static const char * invalid_token_in_array="Invalid token in array";
	
	
	static Array parse_array (StringGraphemeClusterIterator & iter, ParserState & state) {
	
		Array retr;
		
		++iter;
		for (bool first=true;;) {
		
			//	Whitespace inside arrays is
			//	legal
			skip_whitespace(iter);
		
			//	We found the end of the string
			//	without finding a closing square
			//	bracket, that's an error
			if (iter.IsEnd()) throw Error(unterminated_array);
			
			//	Is this the end of the array?
			if (*iter==']') break;
			
			if (first) first=false;
			else if (*iter==',') {
			
				++iter;
				
				skip_whitespace(iter);
				
			} else throw Error(invalid_token_in_array);
			
			//	Get value
			retr.Values.Add(parse(iter,state));
		
		}
		
		//	Skip closing square bracket
		++iter;
		
		return retr;
	
	}
	
	
	static const char * unterminated_object="Unterminated object";
	static const char * invalid_key="Invalid key in object";
	static const char * no_value_for_key="No value given for key";
	static const char * duplicate_key="Duplicate key in object";
	static const char * invalid_token_in_object="Invalid token in object";
	
	
	static Object parse_object (StringGraphemeClusterIterator & iter, ParserState & state) {
	
		Object retr;
		retr.Construct();
		
		++iter;
		for (bool first=true;;) {
		
			//	Whitespace inside objects is
			//	legal
			skip_whitespace(iter);
			
			//	String can't end until after
			//	}
			if (iter.IsEnd()) throw Error(unterminated_object);
			
			if (*iter=='}') break;
			
			if (first) first=false;
			else if (*iter==',') {

				++iter;
				
				skip_whitespace(iter);
				
			} else throw Error(invalid_token_in_object);
			
			//	Every item in an object
			//	is identified by a string
			if (*iter!='"') throw Error(invalid_key);
			
			//	Get the key
			String key(parse_string(iter,state));
			
			//	Check for duplicate keys
			if (retr.Pairs->count(key)!=0) throw Error(duplicate_key);
			
			skip_whitespace(iter);
			
			if (iter.IsEnd()) throw Error(unterminated_object);
			
			//	There must be a colon separating
			//	the key and the value
			if (*iter!=':') throw Error(no_value_for_key);
			
			//	Skip over colon
			++iter;
			
			skip_whitespace(iter);
			
			//	Get the value
			Value value(parse(iter,state));
			
			//	Insert
			retr.Pairs->emplace(
				std::move(key),
				std::move(value)
			);
		
		}
		
		//	Skip closing curly brace
		++iter;
		
		return retr;
	
	}
	
	
	static const char * no_value="No value where expected";
	static const char * invalid_value="Unrecognized value";
	
	
	static Value parse_misc (StringGraphemeClusterIterator & iter, ParserState & state) {
	
		String value;
		for (;;++iter) {
		
			if (
				iter.IsEnd() ||
				iter->IsWhitespace() ||
				iter->IsLineBreak() ||
				(*iter=='}') ||
				(*iter==']') ||
				(*iter==',')
			) break;
			
			value << *iter;
		
		}
		
		if (value.Count()==0) throw Error(no_value);
		
		Value retr;
		
		if (value=="true") retr=true;
		else if (value=="false") retr=false;
		else if (value!="null") {
		
			//	See if we have a number
			
			auto c_str=value.ToCString();
			char * end;
			retr=std::strtod(
				static_cast<char *>(c_str),
				&end
			);
			
			if (end==static_cast<char *>(c_str)) throw Error(invalid_value);
		
		}
		
		return retr;
	
	}
	
	
	static Value parse (StringGraphemeClusterIterator & iter, ParserState & state) {
	
		++state.Depth;
		state.CheckDepth();
	
		skip_whitespace(iter);
		
		Value retr;
		
		if (iter.IsEnd()) throw Error(no_value);
		
		if (*iter=='"') retr=parse_string(iter,state);
		else if (*iter=='[') retr=parse_array(iter,state);
		else if (*iter=='{') retr=parse_object(iter,state);
		else retr=parse_misc(iter,state);
		
		--state.Depth;
		
		return retr;
	
	}
	
	
	static const char * too_many_values="Multiple values at root, JSON consists of only one root value";
	
	
	Value Parse (const String & json, Word max_depth) {
	
		auto iter=json.begin();
		
		ParserState state{0,max_depth};
		
		auto retr=parse(iter,state);
		
		skip_whitespace(iter);
		
		if (!iter.IsEnd()) throw Error(too_many_values);
		
		return retr;
	
	}
	
	
	static String print (bool bln) {
	
		String retr("Boolean: ");
		retr << (bln ? "true" : "false");
		
		return retr;
	
	}
	
	
	static String print (Double dbl) {
	
		String retr("Number: ");
		retr << String(dbl);
		
		return retr;
	
	}
	
	
	static String print (const String & str) {
	
		String retr("String: \"");
		retr << str << "\"";
		
		return retr;
	
	}
	
	
	static String print (const Value &);
	
	
	static const Regex newline("^",RegexOptions().SetMultiline());
	static const RegexReplacement newline_replacement("\t");
	
	
	static String print (const Object & obj) {
	
		if (obj.IsNull()) return "null";
		
		String retr("Object:");
		
		if (obj.Pairs->size()==0) {
		
			retr << " <empty>";
			
			return retr;
		
		}
		
		for (const auto & pair : *obj.Pairs) {
		
			String kvp(pair.first);
			kvp << " => " << print(pair.second);
			
			retr << Newline << newline.Replace(
				kvp,
				newline_replacement
			);
		
		}
		
		return retr;
	
	}
	
	
	static String print (const Array & arr) {
	
		String retr("Array: [");
		retr << String(arr.Values.Count()) << "]";
		
		bool first=true;
		for (const auto & value : arr.Values) {
		
			if (first) first=false;
			else retr << ",";
			
			retr << Newline << newline.Replace(
				print(value),
				newline_replacement
			);
		
		}
		
		return retr;
	
	}
	
	
	static String print (const Value & value) {
	
		return (
			value.IsNull()
				?	String("null")
				:	(
						value.Is<String>()
							?	print(value.Get<String>())
							:	(
									value.Is<Double>()
										?	print(value.Get<Double>())
										:	(
												value.Is<Object>()
													?	print(value.Get<Object>())
													:	(
															value.Is<Array>()
																?	print(value.Get<Array>())
																:	print(value.Get<bool>())
														)
											)
								)
					)
		);
	
	}
	
	
	String ToString (const Value & value) {
		
		return print(value);
	
	}


}

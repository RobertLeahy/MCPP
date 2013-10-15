#include <nbt.hpp>
#include <cstring>
#include <type_traits>
#include <utility>


namespace NBT {


	Error::Error (const char * what) noexcept : what_str(what) {	}
	
	
	const char * Error::what () const noexcept {
	
		return what_str;
	
	}
	
	
	static const char * too_deep="Maximum recursion level exceeded";
	static const char * insufficient_bytes="Insufficient bytes to complete parsing";
	static const char * negative_length="Negative length";
	static const char * unknown_type="Unknown tag type";
	static const char * unexpected_tag_end="Unexpected TAG_End";
	static const char * root_not_compound="Root tag not TAG_Compound";
	
	
	class Parser {
	
	
		private:
		
		
			const Byte * & begin;
			const Byte * end;
			Word max_depth;
			Word depth;
			
			
			void check () const {
			
				if (
					(max_depth!=0) &&
					(depth>max_depth)
				) throw Error(too_deep);
			
			}
			
			
			PayloadType get_tag_impl (Byte);
			NamedTag get_named_tag_impl (Byte);
			
			
			template <typename prefix, typename T>
			Vector<T> get_array_impl ();
			
			
			template <typename T>
			T get ();
	
	
		public:
		
		
			Parser (const Byte * & begin, const Byte * end, Word max_depth) noexcept
				:	begin(begin),
					end(end),
					max_depth(max_depth),
					depth(0)
			{	}
			
			
			NamedTag operator () ();
	
	
	};
	
	
	template <typename T>
	T Parser::get () {
	
		alignas(T) Byte buffer [sizeof(T)];
		
		for (Word i=0;i<sizeof(T);++i) {
		
			if (begin==end) throw Error(insufficient_bytes);
			
			//	Populate bytes in reverse order
			//	if system is of wrong endianness
			if (Endianness::IsBigEndian<T>()) buffer[i]=*begin;
			else buffer[sizeof(T)-1-i]=*begin;
			
			++begin;
		
		}
		
		return *reinterpret_cast<T *>(
			reinterpret_cast<void *>(
				buffer
			)
		);
	
	}
	
	
	template <>
	String Parser::get<String> () {
	
		//	Get length prefix
		auto len=get<UInt16>();
		
		//	Make sure there's enough bytes
		if ((end-begin)<len) throw Error(insufficient_bytes);
		
		//	Decode
		String retr(UTF8().Decode(begin,begin+len));
		
		//	Advance pointer
		begin+=len;
	
		return retr;
	
	}
	
	
	template <>
	Vector<SByte> Parser::get<Vector<SByte>> () {
	
		return get_array_impl<Int32,SByte>();
	
	}
	
	
	template <>
	Vector<Int32> Parser::get<Vector<Int32>> () {
	
		return get_array_impl<Int32,Int32>();
	
	}
	
	
	template <>
	NamedTag Parser::get<NamedTag> () {
	
		return get_named_tag_impl(get<Byte>());
	
	}
	
	
	template <>
	Compound Parser::get<Compound> () {
	
		Compound retr;
		
		//	Loop until a TAG_End is found
		for (;;) {
		
			auto tag=get<NamedTag>();
			
			if (tag.Payload.IsNull()) break;
			
			if (!retr) retr=Compound(new std::unordered_map<String,NamedTag>());
			
			String name=tag.Name;
			
			retr->emplace(
				name,
				std::move(tag)
			);
		
		}
		
		return retr;
	
	}
	
	
	template <typename prefix, typename T>
	Vector<T> Parser::get_array_impl () {
	
		//	Get length
		auto len=get<prefix>();
		
		//	Check to make sure signed
		//	prefixes aren't erroneously
		//	negative
		if (
			std::is_signed<prefix>::value &&
			(len<0)
		) throw Error(negative_length);
		
		Vector<T> retr;
		
		//	Get each element
		for (prefix i=0;i<len;++i) retr.Add(get<T>());
		
		return retr;
	
	}
	
	
	PayloadType Parser::get_tag_impl (Byte type) {
	
		++depth;
		check();
		
		PayloadType retr;
		
		switch (type) {
		
			//	Handle unknown/invalid types
			default:throw Error(unknown_type);
			//	TAG_End
			case 0:throw Error(unexpected_tag_end);
			//	TAG_Byte
			case 1:
				retr=get<SByte>();
				break;
			//	TAG_Short
			case 2:
				retr=get<Int16>();
				break;
			//	TAG_Int
			case 3:
				retr=get<Int32>();
				break;
			//	TAG_Long
			case 4:
				retr=get<Int64>();
				break;
			//	TAG_Float
			case 5:
				retr=get<Single>();
				break;
			//	TAG_Double
			case 6:
				retr=get<Double>();
				break;
			case 7:
				retr=get<Vector<SByte>>();
				break;
			//	TAG_String
			case 8:
				retr=get<String>();
				break;
			//	TAG_List
			case 9:{
			
				//	Get the type of the tags
				//	in the list
				auto type=get<Byte>();
				
				//	Get the length of thi list
				auto len=get<Int32>();
				
				//	Negative check
				if (len<0) throw Error(negative_length);
				
				//	Create list
				retr.Construct<List>();
				auto & list=retr.Get<List>();
				list.Type=type;
				
				//	Get elements
				for (decltype(len) i=0;i<len;++i) list.Payload.Add(Tag{get_tag_impl(type)});
			
			}break;
			//	TAG_Compound
			case 10:
				retr=get<Compound>();
				break;
			//	TAG_Int_Array
			case 11:
				retr=get<Vector<Int32>>();
				break;
		
		}
		
		--depth;
		
		return retr;
	
	}
	
	
	NamedTag Parser::get_named_tag_impl (Byte type) {
	
		NamedTag retr;
		
		//	TAG_End does not have a name
		//	or a payload
		if (type==0) return retr;
		
		//	Get name
		retr.Name=get<String>();
		
		//	Get tag
		retr.Payload=get_tag_impl(type);
		
		return retr;
	
	}
	
	
	NamedTag Parser::operator () () {
	
		auto retr=get<NamedTag>();
		
		if (!retr.Payload.Is<Compound>()) throw Error(root_not_compound);
		
		return retr;
	
	}


	NamedTag Parse (const Byte * & begin, const Byte * end, Word max_depth) {
	
		return Parser(begin,end,max_depth)();
	
	}
	
	
	static String get_type (const PayloadType & payload) {
	
		switch (payload.Type()) {
		
			case 0:return "TAG_Byte";
			case 1:return "TAG_Short";
			case 2:return "TAG_Int";
			case 3:return "TAG_Long";
			case 4:return "TAG_Float";
			case 5:return "TAG_Double";
			case 6:return "TAG_Byte_Array";
			case 7:return "TAG_String";
			case 8:return "TAG_List";
			case 9:return "TAG_Compound";
			default:
			case 10:return "TAG_Int_Array";
		
		}
	
	}
	
	
	static String elements (Word count) {
	
		String retr(count);
		retr << " " << ((count==1) ? "element" : "elements");
		
		return retr;
	
	}
	
	
	template <typename T>
	static String print (const Vector<T> & array) {
	
		String retr(elements(array.Count()));
		
		bool first=true;
		for (const auto & i : array) {
		
			if (first) first=false;
			else retr << ",";
			
			String append;
			if (std::is_same<T,SByte>::value) {
			
				append << "0x";
				String b(i,16);
				auto count=b.Count();
				if (count<2) {
				
					Word num=2-count;
					
					for (Word i=0;i<num;++i) append << "0";
				
				}
				append << b;
			
			} else {
			
				append=String(i);
			
			}
			
			retr << Newline << "\t" << append;
		
		}
		
		return retr;
	
	}
	
	
	static String print (const PayloadType &);
	
	
	static const char * null_payload="Null payload";
	
	
	static String print (const Tag & tag) {
	
		if (tag.Payload.IsNull()) throw Error(null_payload);
	
		String retr(get_type(tag.Payload));
		retr << ": " << print(tag.Payload);
		
		return retr;
	
	}
	
	
	static String print (const NamedTag & tag) {
	
		if (tag.Payload.IsNull()) throw Error(null_payload);
		
		String retr(get_type(tag.Payload));
		retr << " (\"" << tag.Name << "\"): " << print(tag.Payload);
		
		return retr;
	
	}
	
	
	static const Regex newline("^",RegexOptions().SetMultiline());
	static const RegexReplacement newline_replacement("\t");
	
	
	static String print (const Compound & compound) {
	
		String retr;
	
		if (!compound) {
		
			retr=elements(0);
			
			return retr;
		
		}
		
		retr=elements(compound->size());
		
		for (const auto & pair : *compound) retr << Newline << newline.Replace(
			print(pair.second),
			newline_replacement
		);
		
		return retr;
	
	}
	
	
	static String print (const List & list) {
	
		String retr(elements(list.Payload.Count()));
		
		for (const auto & tag : list.Payload) retr << Newline << newline.Replace(
			print(tag),
			newline_replacement
		);
		
		return retr;
	
	}
	
	
	static String print (const PayloadType & payload) {
	
		switch (payload.Type()) {
		
			case 0:return String(payload.Get<SByte>());
			case 1:return String(payload.Get<Int16>());
			case 2:return String(payload.Get<Int32>());
			case 3:return String(payload.Get<Int64>());
			case 4:return String(payload.Get<Single>());
			case 5:return String(payload.Get<Double>());
			case 6:return print(payload.Get<Vector<SByte>>());
			case 7:return String("\"")+payload.Get<String>()+"\"";
			case 8:return print(payload.Get<List>());
			case 9:return print(payload.Get<Compound>());
			case 10:
			default:return print(payload.Get<Vector<Int32>>());
		
		}
	
	}
	
	
	String ToString (const NamedTag & tag) {
	
		return print(tag);
	
	}
	
	
	static const char * list_type_error="All tags in a list must by of the same type";
	
	
	class Serializer {
	
	
		private:
		
		
			Vector<Byte> & buffer;
			
			
			template <typename T>
			void serialize (const T & obj) {
			
				while ((buffer.Capacity()-buffer.Count())<sizeof(T)) buffer.SetCapacity();
				
				auto begin=buffer.end();
				
				if (Endianness::IsBigEndian<T>()) {
				
					//	Big endian -- can copy directly
					std::memcpy(begin,&obj,sizeof(T));
				
				} else {
				
					//	Little endian, must reverse byte
					//	order
					for (Word i=0;i<sizeof(T);++i) begin[i]=reinterpret_cast<const Byte *>(
						reinterpret_cast<const void *>(
							&obj
						)
					)[sizeof(T)-i-1];
				
				}
				
				buffer.SetCount(buffer.Count()+sizeof(T));
			
			}
			
			
			void serialize (const String & str) {
			
				auto encoded=UTF8().Encode(str);
				
				serialize(UInt16(SafeWord(encoded.Count())));
				
				for (auto b : encoded) serialize(b);
			
			}
			
			
			template <typename T>
			void serialize (const Vector<T> & array) {
			
				serialize(Int32(SafeWord(array.Count())));
				
				for (const auto & i : array) serialize(i);
			
			}
			
			
			void serialize_type (const PayloadType & payload) {
			
				serialize(static_cast<Byte>(payload.Type()+1));
			
			}
			
			
			void serialize (const List & list) {
			
				//	TAG_End is illegal
				if (list.Type==0) throw Error(unexpected_tag_end);
			
				//	Write type
				serialize(list.Type);
				//	Write list
				serialize(Int32(SafeWord(list.Payload.Count())));
				
				for (const auto & tag : list.Payload) {
				
					//	All tags must be of the same
					//	type
					if ((tag.Payload.Type()+1)!=list.Type) throw Error(list_type_error);
					
					serialize(tag.Payload);
				
				}
			
			}
			
			
			void serialize (const Compound & compound) {
			
				if (compound) for (const auto & pair : *compound) serialize(pair.second);
				
				//	Write TAG_End
				serialize(static_cast<Byte>(0));
			
			}
			
			
			void serialize (const PayloadType & payload) {
			
				switch (payload.Type()) {
				
					case 0:
						serialize(payload.Get<SByte>());
						break;
					case 1:
						serialize(payload.Get<Int16>());
						break;
					case 2:
						serialize(payload.Get<Int32>());
						break;
					case 3:
						serialize(payload.Get<Int64>());
						break;
					case 4:
						serialize(payload.Get<Single>());
						break;
					case 5:
						serialize(payload.Get<Double>());
						break;
					case 6:
						serialize(payload.Get<Vector<SByte>>());
						break;
					case 7:
						serialize(payload.Get<String>());
						break;
					case 8:
						serialize(payload.Get<List>());
						break;
					case 9:
						serialize(payload.Get<Compound>());
						break;
					case 10:
					default:
						serialize(payload.Get<Vector<Int32>>());
						break;
				
				}
			
			}
			
			
			void serialize (const Tag & tag) {
			
				if (tag.Payload.IsNull()) throw Error(null_payload);
				
				serialize(tag.Payload);
			
			}
			
			
			void serialize (const NamedTag & tag) {
			
				if (tag.Payload.IsNull()) throw Error(null_payload);
				
				serialize_type(tag.Payload);
				
				serialize(tag.Name);
				
				serialize(tag.Payload);
			
			}
			
			
		public:
		
		
			Serializer (Vector<Byte> & buffer) noexcept : buffer(buffer) {	}
			
			
			void operator () (const NamedTag & tag) {
			
				serialize(tag);
			
			}
	
	
	};
	
	
	void Serialize (Vector<Byte> & buffer, const NamedTag & tag) {
	
		Serializer serializer(buffer);
		serializer(tag);
	
	}
	
	
	Vector<Byte> Serialize (const NamedTag & tag) {
	
		Vector<Byte> retr;
		
		Serialize(retr,tag);
		
		return retr;
	
	}
	

}

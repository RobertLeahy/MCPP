String & NamedTag::Name () noexcept {

	return name;

}


const String & NamedTag::Name () const noexcept {

	return name;

}


TagType NamedTag::Type () const noexcept {

	return tag.Type();

}


template <typename T>
typename std::enable_if<!std::is_same<T,String>::value,T>::type get (const Byte ** begin, const Byte * end) {

	union {
		T returnthis;
		Byte buffer [sizeof(T)];
	};
	
	for (Word i=0;i<sizeof(T);++i) {
	
		if (*begin==end) throw std::out_of_range(OutOfRangeError);
		
		buffer[i]=**begin;
		
		++(*begin);
	
	}
	
	if (!Endianness::IsBigEndian<T>()) Endianness::FixEndianness<T>(&returnthis);
	
	return returnthis;

}


template <typename T>
typename std::enable_if<std::is_same<T,String>::value,T>::type get (const Byte ** begin, const Byte * end) {

	UInt16 len=get<UInt16>(begin,end);
	
	Vector<Byte> buffer(len);
	
	for (UInt16 i=0;i<len;++i) buffer.Add(get<Byte>(begin,end));
	
	return UTF8().Decode(buffer.begin(),buffer.end());

}


template <typename T>
Vector<T> get_array (const Byte ** begin, const Byte * end) {

	Int32 len=get<Int32>(begin,end);
	
	if (len<0) throw std::invalid_argument(ArgumentError);
	
	Vector<T> returnthis(static_cast<Word>(len));
	
	for (Int32 i=0;i<len;++i) returnthis.Add(get<T>(begin,end));
	
	return returnthis;

}


inline Tag get_tag (const Byte **, const Byte *, TagType);


inline NamedTag create (const Byte ** begin, const Byte * end) {

	TagType type=static_cast<TagType>(get<Byte>(begin,end));
	
	String name=get<String>(begin,end);
	
	return NamedTag(std::move(name),get_tag(begin,end,type));

}


inline Tag get_tag (const Byte ** begin, const Byte * end, TagType type) {

	switch (type) {
	
		//	Invalid enum member
		default:throw std::invalid_argument(ArgumentError);
		
		//	TAG_Byte
		case TagType::Byte:return Tag(get<SByte>(begin,end));
		
		//	TAG_Short
		case TagType::Short:return Tag(get<Int16>(begin,end));
		
		//	TAG_Int
		case TagType::Int:return Tag(get<Int32>(begin,end));
		
		//	TAG_Long
		case TagType::Long:return Tag(get<Int64>(begin,end));
		
		//	TAG_Float
		case TagType::Float:return Tag(get<Single>(begin,end));
		
		//	TAG_Double
		case TagType::Double:return Tag(get<Double>(begin,end));
		
		//	TAG_Byte_Array
		case TagType::ByteArray:return Tag(get_array<SByte>(begin,end));
		
		//	TAG_String
		case TagType::String:return Tag(get<String>(begin,end));
		
		//	TAG_List
		case TagType::List:{
		
			TagType list_type=static_cast<TagType>(get<Byte>(begin,end));
			
			Int32 len=get<Int32>(begin,end);
			
			if (len<0) throw std::invalid_argument(ArgumentError);
			
			Vector<Tag> list(static_cast<Word>(len));
			
			for (Int32 i=0;i<len;++i) list.Add(get_tag(begin,end,list_type));
			
			return Tag(std::move(list));
		
		}
		
		//	TAG_Compound
		case TagType::Compound:{
		
			Vector<NamedTag> compound;
		
			for (;;) {
			
				//	Peek at type of next tag
				if (*begin==end) throw std::invalid_argument(ArgumentError);
				
				//	If it's an end tag, advance past it and
				//	break out of the loop
				if (static_cast<TagType>(**begin)==TagType::End) {
				
					++(*begin);
				
					break;
					
				}
				
				compound.Add(create(begin,end));
			
			}
			
			return Tag(std::move(compound));
		
		}
		
		//	TAG_Int_Array
		case TagType::IntArray:return Tag(get_array<Int32>(begin,end));
	
	}

}


NamedTag NamedTag::Create (const Byte * begin, const Byte * end) {

	if (
		(begin==nullptr) ||
		(end==nullptr)
	) throw std::out_of_range(NullPointerError);
	
	if (begin>=end) throw std::out_of_range(OutOfRangeError);
	
	return create(&begin,end);

}

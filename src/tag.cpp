inline void Tag::destroy () noexcept {

	switch (type) {
	
		//	The trivial types do not require
		//	explicit cleanup
		default:{	}break;
		
		case TagType::ByteArray:{
		
			tag_bytearray.~Vector<SByte>();
		
		}break;
		
		case TagType::String:{
		
			tag_string.~String();
		
		}break;
		
		case TagType::List:{
		
			tag_list.~Vector<Tag>();
		
		}break;
		
		case TagType::Compound:{
		
			tag_compound.~Vector<NamedTag>();
		
		}break;
		
		case TagType::IntArray:{
		
			tag_intarray.~Vector<Int32>();
		
		}break;
	
	}

}


inline void Tag::move (Tag && other) noexcept {

	type=other.type;

	switch (type) {
	
		//	No difference between
		//	copying and moving for
		//	the trivial types
		default:{
		
			copy(other);
		
		}break;
		
		case TagType::ByteArray:{
		
			new (&tag_bytearray) Vector<SByte> (std::move(other.tag_bytearray));
		
		}break;
		
		case TagType::String:{
		
			new (&tag_string) String (std::move(other.tag_string));
		
		}break;
		
		case TagType::List:{
		
			new (&tag_list) Vector<Tag> (std::move(other.tag_list));
		
		}break;
		
		case TagType::Compound:{
		
			new (&tag_compound) Vector<NamedTag> (std::move(other.tag_compound));
		
		}break;
		
		case TagType::IntArray:{
		
			new (&tag_intarray) Vector<Int32> (std::move(other.tag_intarray));
		
		}break;
	
	}

}


inline void Tag::copy (const Tag & other) {

	type=other.type;
	
	switch (type) {
	
		case TagType::Byte:{
		
			tag_byte=other.tag_byte;
		
		}break;
		
		case TagType::Short:{
		
			tag_short=other.tag_short;
		
		}break;
		
		case TagType::Int:{
		
			tag_int=other.tag_int;
		
		}break;
		
		case TagType::Long:{
		
			tag_long=other.tag_long;
		
		}break;
		
		case TagType::Float:{
		
			tag_float=other.tag_float;
		
		}break;
		
		case TagType::Double:{
		
			tag_double=other.tag_double;
		
		}break;
		
		case TagType::ByteArray:{
		
			new (&tag_bytearray) Vector<SByte> (other.tag_bytearray);
		
		}break;
		
		case TagType::String:{
		
			new (&tag_string) String (other.tag_string);
		
		}break;
		
		case TagType::List:{
		
			new (&tag_list) Vector<Tag> (other.tag_list);
		
		}break;
		
		case TagType::Compound:{
		
			new (&tag_compound) Vector<NamedTag> (other.tag_compound);
		
		}break;
		
		default:
		case TagType::IntArray:{
		
			new (&tag_intarray) Vector<Int32> (other.tag_intarray);
		
		}break;
	
	}

}


Tag::Tag (const Tag & other) {

	copy(other);

}


Tag::Tag (Tag && other) noexcept {

	move(std::move(other));

}


Tag & Tag::operator = (const Tag & other) {

	if (&other!=this) {

		destroy();
		
		copy(other);
		
	}
		
	return *this;

}


Tag & Tag::operator = (Tag && other) noexcept {

	if (&other!=this) {

		destroy();
		
		move(std::move(other));
		
	}
	
	return *this;

}


Tag::~Tag () noexcept {

	destroy();

}


TagType Tag::Type () const noexcept {

	return type;

}


Tag::Tag (SByte data) noexcept : type(TagType::Byte), tag_byte(data) {	}


Tag::Tag (Int16 data) noexcept : type(TagType::Short), tag_short(data) {	}


Tag::Tag (Int32 data) noexcept : type(TagType::Int), tag_int(data) {	}


Tag::Tag (Int64 data) noexcept : type(TagType::Long), tag_long(data) {	}


Tag::Tag (Single data) noexcept : type(TagType::Float), tag_float(data) {	}


Tag::Tag (Double data) noexcept : type(TagType::Double), tag_double(data) {	}


Tag::Tag (const Vector<SByte> & data) : type(TagType::ByteArray), tag_bytearray(data) {	}


Tag::Tag (Vector<SByte> && data) noexcept : type(TagType::ByteArray), tag_bytearray(std::move(data)) {	}


Tag::Tag (const String & data) : type(TagType::String), tag_string(data) {	}


Tag::Tag (String && data) noexcept : type(TagType::String), tag_string(std::move(data)) {	}


Tag::Tag (const Vector<Tag> & data) : type(TagType::List), tag_list(data) {	}


Tag::Tag (Vector<Tag> && data) noexcept : type(TagType::List), tag_list(std::move(data)) {	}


Tag::Tag (const Vector<NamedTag> & data) : type(TagType::Compound), tag_compound(data) {	}


Tag::Tag (Vector<NamedTag> && data) noexcept : type(TagType::Compound), tag_compound(std::move(data)) {	}


Tag::Tag (const Vector<Int32> & data) : type(TagType::IntArray), tag_intarray(data) {	}


Tag::Tag (Vector<Int32> && data) noexcept : type(TagType::IntArray), tag_intarray(std::move(data)) {	}


String Tag::TypeToString () const {

	switch (type) {
	
		case TagType::Byte:return String("TAG_Byte");
		
		case TagType::Short:return String("TAG_Short");
		
		case TagType::Int:return String("TAG_Int");
		
		case TagType::Long:return String("TAG_Long");
		
		case TagType::Float:return String("TAG_Float");
		
		case TagType::Double:return String("TAG_Double");
		
		case TagType::ByteArray:return String("TAG_Byte_Array");
		
		case TagType::String:return String("TAG_String");
		
		case TagType::List:return String("TAG_List");
		
		case TagType::Compound:return String("TAG_Compound");
		
		default:
		case TagType::IntArray:return String("TAG_Int_Array");
	
	}

}

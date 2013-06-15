#define p_a(x) case x:return protocol_analysis_impl<x>(packet);


//	Formats a byte for display/logging
static inline String byte_format (Byte b) {

	String returnthis("0x");
	
	String byte(b,16);
	
	if (byte.Count()==1) returnthis << "0";
	
	returnthis << byte;
	
	return returnthis;

}


//	Formats a buffer of bytes for
//	display/logging
static inline String buffer_format (const Vector<Byte> & buffer) {

	String returnthis;

	//	Print each byte is hexadecimal
	//	format
	for (Word i=0;i<buffer.Count();++i) {
	
		//	We don't need a space
		//	or a newline before
		//	the very first byte
		if (i!=0) {
		
			//	End of line
			if ((i%8)==0) returnthis << Newline;
			//	Space before each byte
			//	that isn't the first or
			//	right after a newline
			else returnthis << " ";
		
		}
		
		returnthis << byte_format(buffer[i]);
	
	}
	
	return returnthis;

}


template <typename T>
String render (const T & value) {

	return "UNABLE TO RENDER";

}


static String render (bool value) {

	String returnthis("Boolean: ");
	if (value) returnthis << "TRUE";
	else returnthis << "FALSE";
	
	return returnthis;

}


static String render (const Vector<Byte> & value) {

	String returnthis("Array of bytes (");
	returnthis << String(value.Count()) << " bytes):" << Newline << buffer_format(value);
	
	return returnthis;

}


static String render (Single value) {

	return String::Format(
		"Single-Precision Floating Point Number: {0}",
		value
	);

}


static String render (Double value) {

	return String::Format(
		"Double-Precision Floating Point Number: {0}",
		value
	);

}


static String render (Int16 value) {

	return String::Format(
		"16-bit Signed Integer: {0}",
		value
	);

}


static String render (Int32 value) {

	return String::Format(
		"32-bit Signed Integer: {0}",
		value
	);

}


static String render (Int64 value) {

	return String::Format(
		"64-bit Signed Integer: {0}",
		value
	);

}


static String render (SByte value) {

	return String::Format(
		"Signed Byte: {0}",
		value
	);

}


static String render (const String & value) {

	return String::Format(
		"Unicode String ({0} grapheme{1}, {2} code point{3}): \"{4}\"",
		value.Count(),
		(value.Count()==1) ? "" : "s",
		value.Size(),
		(value.Size()==1) ? "" : "s",
		value
	);

}


static String render (const Vector<String> & value) {

	String returnthis("Array of strings (");
	returnthis << String(value.Count()) << " strings):";
	
	for (const auto & s : value) returnthis << Newline << render(s);
	
	return returnthis;

}


template <typename T>
class is_packet_array {


	public:
	
	
		static const bool Value=false;


};


template <typename TCount, typename TItem>
class is_packet_array<PacketArray<TCount,TItem>> {


	public:
	
	
		static const bool Value=true;


};


template <Word i, typename T>
inline auto protocol_analysis_packet_retrieve_helper (const Packet & packet) -> const typename std::enable_if<
	is_packet_array<T>::Value,
	decltype(packet.Retrieve<T>(i).Payload)
>::type & {

	return packet.Retrieve<decltype(packet.Retrieve<T>(i).Payload)>(i);

}


template <Word i, typename T>
inline const typename std::enable_if<
	!is_packet_array<T>::Value,
	T
>::type & protocol_analysis_packet_retrieve_helper (const Packet & packet) {

	return packet.Retrieve<T>(i);

}


template <Byte type, Word i>
inline String protocol_analysis_render (const Packet & packet) {

	String returnthis(Newline);
	returnthis << "#" << String(i+1) << " - " << render(
		protocol_analysis_packet_retrieve_helper<
			i,
			typename PacketTypeMap<type>::template RetrieveType<i>::Type
		>(packet)
	);
	
	return returnthis;

}


template <Byte type, Word i>
inline typename std::enable_if<
	i!=0,
	String
>::type protocol_analysis_recurse (const Packet & packet) {

	//	Depth first
	String returnthis(
		protocol_analysis_recurse<type,i-1>(packet)
	);
	
	returnthis << protocol_analysis_render<type,i>(packet);
	
	return returnthis;

}


template <Byte type, Word i>
inline typename std::enable_if<
	i==0,
	String
>::type protocol_analysis_recurse (const Packet & packet) {

	return protocol_analysis_render<type,i>(packet);

}


template <Byte type>
inline typename std::enable_if<
	PacketTypeMap<type>::TypesCount!=0,
	String
>::type protocol_analysis_recurse_helper (const Packet & packet) {

	return protocol_analysis_recurse<
		type,
		PacketTypeMap<type>::TypesCount-1
	>(packet);

}


template <Byte type>
inline typename std::enable_if<
	PacketTypeMap<type>::TypesCount==0,
	String
>::type protocol_analysis_recurse_helper (const Packet & packet) {

	return String();

}


template <Byte type>
inline typename std::enable_if<
	std::is_base_of<
		PacketFactory,
		PacketTypeMap<type>
	>::value,
	String
>::type protocol_analysis_impl (const Packet & packet) {

	//	Get the individual bytes of this
	//	packet for analysis
	Vector<Byte> to_bytes(
		packet.ToBytes()
	);
	
	//	Banner for bytes
	String log(
		String::Format(
			"===={0} BYTES====",
			to_bytes.Count()
		)
	);
	log << Newline << buffer_format(to_bytes) << Newline << "====PAYLOAD====";
	
	//	Log the payload
	log << protocol_analysis_recurse_helper<type>(packet);
	
	return log;

}


template <Byte type>
inline typename std::enable_if<
	!std::is_base_of<
		PacketFactory,
		PacketTypeMap<type>
	>::value,
	String
>::type protocol_analysis_impl (const Packet & packet) {

	//	Empty string
	return String();

}


static inline String protocol_analysis (const Packet & packet) {

	switch (packet.Type()) {
	
		p_a(0x00)
		p_a(0x01)
		p_a(0x02)
		p_a(0x03)
		p_a(0x04)
		p_a(0x05)
		p_a(0x06)
		p_a(0x07)
		p_a(0x08)
		p_a(0x09)
		p_a(0x0A)
		p_a(0x0B)
		p_a(0x0C)
		p_a(0x0D)
		p_a(0x0E)
		p_a(0x0F)
		p_a(0x10)
		p_a(0x11)
		p_a(0x12)
		p_a(0x13)
		p_a(0x14)
		p_a(0x15)
		p_a(0x16)
		p_a(0x17)
		p_a(0x18)
		p_a(0x19)
		p_a(0x1A)
		p_a(0x1B)
		p_a(0x1C)
		p_a(0x1D)
		p_a(0x1E)
		p_a(0x1F)
		p_a(0x20)
		p_a(0x21)
		p_a(0x22)
		p_a(0x23)
		p_a(0x24)
		p_a(0x25)
		p_a(0x26)
		p_a(0x27)
		p_a(0x28)
		p_a(0x29)
		p_a(0x2A)
		p_a(0x2B)
		p_a(0x2C)
		p_a(0x2D)
		p_a(0x2E)
		p_a(0x2F)
		p_a(0x30)
		p_a(0x31)
		p_a(0x32)
		p_a(0x33)
		p_a(0x34)
		p_a(0x35)
		p_a(0x36)
		p_a(0x37)
		p_a(0x38)
		p_a(0x39)
		p_a(0x3A)
		p_a(0x3B)
		p_a(0x3C)
		p_a(0x3D)
		p_a(0x3E)
		p_a(0x3F)
		p_a(0x40)
		p_a(0x41)
		p_a(0x42)
		p_a(0x43)
		p_a(0x44)
		p_a(0x45)
		p_a(0x46)
		p_a(0x47)
		p_a(0x48)
		p_a(0x49)
		p_a(0x4A)
		p_a(0x4B)
		p_a(0x4C)
		p_a(0x4D)
		p_a(0x4E)
		p_a(0x4F)
		p_a(0x50)
		p_a(0x51)
		p_a(0x52)
		p_a(0x53)
		p_a(0x54)
		p_a(0x55)
		p_a(0x56)
		p_a(0x57)
		p_a(0x58)
		p_a(0x59)
		p_a(0x5A)
		p_a(0x5B)
		p_a(0x5C)
		p_a(0x5D)
		p_a(0x5E)
		p_a(0x5F)
		p_a(0x60)
		p_a(0x61)
		p_a(0x62)
		p_a(0x63)
		p_a(0x64)
		p_a(0x65)
		p_a(0x66)
		p_a(0x67)
		p_a(0x68)
		p_a(0x69)
		p_a(0x6A)
		p_a(0x6B)
		p_a(0x6C)
		p_a(0x6D)
		p_a(0x6E)
		p_a(0x6F)
		p_a(0x70)
		p_a(0x71)
		p_a(0x72)
		p_a(0x73)
		p_a(0x74)
		p_a(0x75)
		p_a(0x76)
		p_a(0x77)
		p_a(0x78)
		p_a(0x79)
		p_a(0x7A)
		p_a(0x7B)
		p_a(0x7C)
		p_a(0x7D)
		p_a(0x7E)
		p_a(0x7F)
		p_a(0x80)
		p_a(0x81)
		p_a(0x82)
		p_a(0x83)
		p_a(0x84)
		p_a(0x85)
		p_a(0x86)
		p_a(0x87)
		p_a(0x88)
		p_a(0x89)
		p_a(0x8A)
		p_a(0x8B)
		p_a(0x8C)
		p_a(0x8D)
		p_a(0x8E)
		p_a(0x8F)
		p_a(0x90)
		p_a(0x91)
		p_a(0x92)
		p_a(0x93)
		p_a(0x94)
		p_a(0x95)
		p_a(0x96)
		p_a(0x97)
		p_a(0x98)
		p_a(0x99)
		p_a(0x9A)
		p_a(0x9B)
		p_a(0x9C)
		p_a(0x9D)
		p_a(0x9E)
		p_a(0x9F)
		p_a(0xA0)
		p_a(0xA1)
		p_a(0xA2)
		p_a(0xA3)
		p_a(0xA4)
		p_a(0xA5)
		p_a(0xA6)
		p_a(0xA7)
		p_a(0xA8)
		p_a(0xA9)
		p_a(0xAA)
		p_a(0xAB)
		p_a(0xAC)
		p_a(0xAD)
		p_a(0xAE)
		p_a(0xAF)
		p_a(0xB0)
		p_a(0xB1)
		p_a(0xB2)
		p_a(0xB3)
		p_a(0xB4)
		p_a(0xB5)
		p_a(0xB6)
		p_a(0xB7)
		p_a(0xB8)
		p_a(0xB9)
		p_a(0xBA)
		p_a(0xBB)
		p_a(0xBC)
		p_a(0xBD)
		p_a(0xBE)
		p_a(0xBF)
		p_a(0xC0)
		p_a(0xC1)
		p_a(0xC2)
		p_a(0xC3)
		p_a(0xC4)
		p_a(0xC5)
		p_a(0xC6)
		p_a(0xC7)
		p_a(0xC8)
		p_a(0xC9)
		p_a(0xCA)
		p_a(0xCB)
		p_a(0xCC)
		p_a(0xCD)
		p_a(0xCE)
		p_a(0xCF)
		p_a(0xD0)
		p_a(0xD1)
		p_a(0xD2)
		p_a(0xD3)
		p_a(0xD4)
		p_a(0xD5)
		p_a(0xD6)
		p_a(0xD7)
		p_a(0xD8)
		p_a(0xD9)
		p_a(0xDA)
		p_a(0xDB)
		p_a(0xDC)
		p_a(0xDD)
		p_a(0xDE)
		p_a(0xDF)
		p_a(0xE0)
		p_a(0xE1)
		p_a(0xE2)
		p_a(0xE3)
		p_a(0xE4)
		p_a(0xE5)
		p_a(0xE6)
		p_a(0xE7)
		p_a(0xE8)
		p_a(0xE9)
		p_a(0xEA)
		p_a(0xEB)
		p_a(0xEC)
		p_a(0xED)
		p_a(0xEE)
		p_a(0xEF)
		p_a(0xF0)
		p_a(0xF1)
		p_a(0xF2)
		p_a(0xF3)
		p_a(0xF4)
		p_a(0xF5)
		p_a(0xF6)
		p_a(0xF7)
		p_a(0xF8)
		p_a(0xF9)
		p_a(0xFA)
		p_a(0xFB)
		p_a(0xFC)
		p_a(0xFD)
		p_a(0xFE)
		p_a(0xFF)
	
	}

}

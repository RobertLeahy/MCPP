#include <packet.hpp>


using namespace MCPP::PacketImpl;


namespace MCPP {


	void BadFormat::Raise () {
	
		throw BadFormat();
	
	}
	
	
	void InsufficientBytes::Raise () {
	
		throw InsufficientBytes();
	
	}
	
	
	void BadPacketID::Raise () {
	
		throw BadPacketID();
	
	}
	
	
	Packet::Packet (UInt32 id) noexcept : ID(id) {	}


	namespace PacketImpl {
	
	
		inline void PacketContainer::destroy_impl () noexcept {
		
			if (engaged) {
			
				engaged=false;
				
				if (destroy!=nullptr) destroy(storage);
			
			}
		
		}


		PacketContainer::PacketContainer () noexcept : destroy(nullptr), from_bytes(nullptr), engaged(false) {	}
		
		
		PacketContainer::~PacketContainer () noexcept {
		
			destroy_impl();
		
		}
		
		
		void PacketContainer::FromBytes (const Byte * & begin, const Byte * end) {
		
			if (from_bytes!=nullptr) {
			
				destroy_impl();
			
				from_bytes(begin,end,storage);
				
				engaged=true;
			
			}
		
		}
		
		
		Packet & PacketContainer::Get () noexcept {
		
			return packet;
		
		}
		
		
		const Packet & PacketContainer::Get () const noexcept {
		
			return packet;
		
		}
		
		
		void PacketContainer::Imbue (destroy_type destroy, from_bytes_type from_bytes) noexcept {
		
			destroy_impl();
			
			this->destroy=destroy;
			this->from_bytes=from_bytes;
		
		}
		
		
	}
	
	
	template <Word i, typename T>
	static typename std::enable_if<
		i>=T::Count
	>::type destroy_impl (const void *) noexcept {	}
	
	
	template <Word i, typename T>
	static typename std::enable_if<
		i<T::Count
	>::type destroy_impl (void * ptr) noexcept {
	
		typedef typename T::template Types<i> types;
		typedef typename types::Type type;
		
		reinterpret_cast<type *>(
			reinterpret_cast<void *>(
				reinterpret_cast<Byte *>(ptr)+types::Offset
			)
		)->~type();
		
		destroy_impl<i+1,T>(ptr);
	
	}
	
	
	template <typename T>
	static void destroy (void * ptr) noexcept {
	
		destroy_impl<0,T>(ptr);
	
	}
	
	
	template <Word i, typename T>
	static typename std::enable_if<
		i>=T::Count
	>::type deserialize_impl (const Byte * &, const Byte *, const void *) noexcept {	}
	
	
	template <Word i, typename T>
	static typename std::enable_if<
		i<T::Count
	>::type deserialize_impl (const Byte * & begin, const Byte * end, void * ptr) {
	
		typedef typename T::template Types<i> types;
		typedef typename types::Type type;
		
		type * p=reinterpret_cast<type *>(
			reinterpret_cast<void *>(
				reinterpret_cast<Byte *>(ptr)+types::Offset
			)
		);
		
		Serializer<type>::FromBytes(begin,end,p);
		
		try {
		
			deserialize_impl<i+1,T>(begin,end,ptr);
		
		} catch (...) {
		
			p->~type();
			
			throw;
		
		}
	
	}
	
	
	template <typename T>
	static void deserialize (const Byte * & begin, const Byte * end, void * ptr) {
	
		deserialize_impl<0,T>(begin,end,ptr);
	
	}
	
	
	template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
	static bool check (ProtocolState state, ProtocolDirection dir, UInt32 i, PacketContainer & container) {
	
		typedef PacketMap<ps,pd,id> type;
	
		if ((ps==state) && (dir==pd) && (i==id)) {
		
			if (type::IsValid) {
			
				container.Imbue(
					destroy<type>,
					deserialize<type>
				);
				
				return true;
				
			}
			
			BadPacketID::Raise();
		
		}
	
		return false;
	
	}
	
	
	template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
	static typename std::enable_if<
		(ps==LI) && (pd==BO) && (id==LargestID)
	>::type imbue_container (ProtocolState state, ProtocolDirection dir, UInt32 i, PacketContainer & container) {
	
		if (!check<ps,pd,id>(state,dir,i,container)) BadPacketID::Raise();
	
	}
	
	
	template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
	static typename std::enable_if<
		!((ps==LI) && (pd==BO) && (id==LargestID))
	>::type imbue_container (ProtocolState state, ProtocolDirection dir, UInt32 i, PacketContainer & container) {
	
		if (!check<ps,pd,id>(state,dir,i,container)) imbue_container<
			((pd==BO) && (id==LargestID)) ? Next(ps) : ps,
			(id==LargestID) ? Next(pd) : pd,
			(id==LargestID) ? 0 : (id+1)
		>(state,dir,i,container);
	
	}
	
	
	PacketParser::PacketParser () noexcept : in_progress(false) {	}
	
	
	bool PacketParser::FromBytes (Vector<Byte> & buffer, ProtocolState state, ProtocolDirection direction) {
	
		//	Prepare iterators for deserialization
		const Byte * begin=buffer.begin();
	
		if (in_progress) {
		
			//	In progress -- we're waiting for
			//	a certain number of bytes in the
			//	buffer (as specified by the length
			//	header)
		
			if (buffer.Count()>=waiting_for) {
			
				//	Sufficient bytes -- attempt to
				//	deserialize packet
				
				//	Set the end pointer to the number
				//	of bytes past the beginning specified
				//	in the length header
				auto end=begin+waiting_for;
			
				//	Get the ID
				UInt32 id=Deserialize<PacketImpl::VarInt<UInt32>>(begin,end);
				
				//	Prepare the parser/container
				imbue_container<HS,CB,0>(state,direction,id,container);
				
				//	Attempt to populate remainder
				//	of packet
				container.FromBytes(begin,end);
				
				//	Check to make sure we consumed
				//	as many bytes as the length
				//	header specified
				if (begin!=end) BadFormat::Raise();
				
				//	Put the ID in place
				container.Get().ID=id;
				
				//	We are finished with this packet
				in_progress=false;
				
				//	Delete consumed bytes from the
				//	buffer
				buffer.Delete(
					0,
					begin-buffer.begin()
				);
			
			} else {
			
				//	Insufficient bytes
				
				return false;
			
			}
			
			return true;
		
		}
		
		//	Not in progress -- starting a new
		//	packet, get the length header
		//	and see where we can go from there
		
		try {
		
			//	Get the length header
			waiting_for=Deserialize<PacketImpl::VarInt<UInt32>>(begin,buffer.end());
		
		} catch (const InsufficientBytes &) {
		
			//	There's not enough bytes to
			//	get the length header
			
			return false;
		
		}
		
		//	We successfully acquired the
		//	length header, we are now
		//	parsing a packet
		in_progress=true;
		
		//	Delete consumed bytes from
		//	the buffer
		buffer.Delete(
			0,
			begin-buffer.begin()
		);
		
		//	Recurse to see if we can parse the
		//	whole packet
		return FromBytes(buffer,state,direction);
	
	}
	
	
	Packet & PacketParser::Get () noexcept {
	
		return container.Get();
	
	}
	
	
	const Packet & PacketParser::Get () const noexcept {
	
		return container.Get();
	
	}
	
	
	static const Regex newline("^",RegexOptions().SetMultiline());
	static const RegexReplacement newline_replacement("\t");
	
	
	static String add_tabs (const String & str) {
	
		return newline.Replace(
			str,
			newline_replacement
		);
	
	}
	
	
	template <typename T>
	typename std::enable_if<
		std::is_integral<T>::value,
		String
	>::type print_type (const T &) {
	
		return String::Format(
			(
				(sizeof(T)==1)
					?	"{{0}} byte"
					:	String::Format(
							"{0}-bit {{0}} integer",
							sizeof(T)*BitsPerByte()
						)
			),
			std::is_unsigned<T>::value ? "unsigned" : "signed"
		);
	
	}
	
	
	template <typename T>
	typename std::enable_if<
		std::is_floating_point<T>::value,
		String
	>::type print_type (const T &) {
	
		return String::Format(
			"{0}-precision floating point number",
			(
				(sizeof(T)==4)
					?	"Single"
					:	(
							(sizeof(T)==8)
								?	"Double"
								:	"Quadruple"
						)
			)
		);
	
	}
	
	
	static String print_type (const JSON::Value &) {
	
		return "JSON";
	
	}
	
	
	/*static String print_type (const NBT::NamedTag &) {
	
		return "NBT";
	
	}*/
	
	
	template <typename... Args>
	String print_type (const Tuple<Args...> &) {
	
		return "Tuple";
	
	}
	
	
	template <typename T>
	String print_type (const PacketImpl::VarInt<T> & varint) {
	
		return print_type(static_cast<T>(varint));
	
	}
	
	
	static String print_type (const String &) {
	
		return "Unicode String";
	
	}
	
	
	template <typename prefix, typename T>
	String print_type (const Array<prefix,T> &) {
	
		T obj;
	
		return String::Format(
			"Array of {0}",
			print_type(obj)
		);
	
	}
	
	
	static String print_type (const ObjectData &) {
	
		return "Object Data";
	
	}
	
	
	static String print_type (const Metadata &) {
	
		return "Entity Metadata";
	
	}
	
	
	static String print_type (const ProtocolState &) {
	
		return "Protocol Direction";
	
	}
	
	
	static String print_type (const Nullable<Slot> &) {
	
		return "Slot Data";
	
	}
	
	
	static String print_value (const JSON::Value & json) {
	
		return JSON::ToString(json);
	
	}
	
	
	static String print_value (const Metadata &) {
	
		return String();
	
	}
	
	
	static String print_value (const ObjectData &) {
	
		return String();
	
	}
	
	
	template <typename... Args>
	static String print_value (const Tuple<Args...> &) {
	
		return String();
	
	}
	
	
	static String print_value (const NBT::NamedTag & nbt) {
	
		if (nbt.Payload.IsNull()) return "NULL";
		
		String retr;
		retr << Newline << NBT::ToString(nbt);
		
		return retr;
	
	}
	
	
	template <typename T>
	typename std::enable_if<
		std::is_integral<T>::value || std::is_floating_point<T>::value,
		String
	>::type print_value (T value) {
	
		return String(value);
	
	}
	
	
	static String print_value (ProtocolState direction) {
	
		return ToString(direction);
	
	}
	
	
	static String print_value (const Nullable<Slot> & slot) {
	
		if (slot.IsNull()) return "NULL";
		
		String retr;
		retr	<<	Newline
				<<	add_tabs(String::Format(
						"Item ID: {0}",
						print_value(slot->ItemID)
					))
				<<	Newline
				<<	add_tabs(String::Format(
						"Count: {0}",
						print_value(slot->Count)
					))
				<<	Newline
				<<	add_tabs(String::Format(
						"Damage: {0}",
						print_value(slot->Damage)
					))
				<<	Newline
				<<	add_tabs(String::Format(
						"NBT Data: {0}",
						print_value(slot->Data)
					));
					
		return retr;
	
	}
	
	
	template <typename T>
	String print_value (const PacketImpl::VarInt<T> & varint) {
	
		return print_value(static_cast<T>(varint));
	
	}
	
	
	static String print_value (const String & str) {
	
		return String::Format(
			"\"{0}\" ({1} code points, {2} graphemes)",
			str,
			str.Size(),
			str.Count()
		);
	
	}
	
	
	template <typename prefix, typename T>
	String print_value (const Array<prefix,T> & arr) {
	
		String retr(
			String::Format(
				"{0} elements",
				arr.Value.Count()
			)
		);
		
		for (const auto & i : arr.Value) retr << Newline << add_tabs(print_value(i));
		
		return retr;
	
	}
	
	
	template <typename T>
	String print (const T & value) {
	
		return String::Format(
			"{0}: {1}",
			print_type(value),
			print_value(value)
		);
	
	}
	
	
	template <Word i, typename T>
	void print_impl (String & str, const void * ptr) {
	
		typedef typename T::template Types<i> types;
		typedef typename types::Type type;
		
		str << Newline << add_tabs(print(*reinterpret_cast<const type *>(
			reinterpret_cast<const void *>(
				reinterpret_cast<const Byte *>(ptr)+types::Offset
			)
		)));
	
	}
	
	
	template <Word i, typename T>
	typename std::enable_if<
		i>=T::Count
	>::type print_driver (const String &, const void *) noexcept {	}
	
	
	template <Word i, typename T>
	typename std::enable_if<
		i<T::Count
	>::type print_driver (String & str, const void * ptr) {
	
		print_impl<i,T>(str,ptr);
		
		print_driver<i+1,T>(str,ptr);
	
	}
	
	
	template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
	bool print_check (String & str, const Packet & packet, ProtocolState state, ProtocolDirection dir) {
	
		typedef PacketMap<ps,pd,id> type;
	
		if ((ps==state) && (dir==pd) && (packet.ID==id)) {
		
			if (type::IsValid) {
			
				print_driver<0,type>(str,&packet);
			
				return true;
			
			}
			
			BadPacketID::Raise();
		
		}
		
		return false;
	
	}
	
	
	template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
	typename std::enable_if<
		(ps==LI) && (pd==BO) && (id==LargestID)
	>::type print_search (String & str, const Packet & packet, ProtocolState state, ProtocolDirection dir) {
	
		if (!print_check<ps,pd,id>(str,packet,state,dir)) BadPacketID::Raise();
	
	}
	
	
	template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
	typename std::enable_if<
		!((ps==LI) && (pd==BO) && (id==LargestID))
	>::type print_search (String & str, const Packet & packet, ProtocolState state, ProtocolDirection dir) {
	
		if (!print_check<ps,pd,id>(str,packet,state,dir)) print_search<
			((pd==BO) && (id==LargestID)) ? Next(ps) : ps,
			(id==LargestID) ? Next(pd) : pd,
			(id==LargestID) ? 0 : (id+1)
		>(str,packet,state,dir);
	
	}
	
	
	String ToString (const Packet & packet, ProtocolState state, ProtocolDirection dir) {
	
		String retr(
			String::Format(
				"Packet ID: {0}, State: {1}, Direction: {2}",
				packet.ID,
				ToString(state),
				ToString(dir)
			)
		);
		
		print_search<HS,CB,0>(retr,packet,state,dir);
		
		return retr;
	
	}
	
	
	static const String hs_str("Handshaking");
	static const String pl_str("Play");
	static const String li_str("Login");
	static const String st_str("Status");
	
	
	const String & ToString (ProtocolState state) {
	
		switch (state) {
		
			case ProtocolState::Handshaking:return hs_str;
			case ProtocolState::Play:return pl_str;
			case ProtocolState::Login:return li_str;
			case ProtocolState::Status:
			default:return st_str;
		
		}
	
	}
	
	
	static const String cb_str("Server to Client");
	static const String sb_str("Client to Server");
	static const String bo_str("Both");
	
	
	const String & ToString (ProtocolDirection dir) {
	
		switch (dir) {
		
			case ProtocolDirection::Clientbound:return cb_str;
			case ProtocolDirection::Serverbound:return sb_str;
			case ProtocolDirection::Both:
			default:return bo_str;
		
		}
	
	}


}

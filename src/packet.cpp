#include <packet.hpp>


using namespace MCPP::PacketImpl;


namespace MCPP {


	void BadFormat::Raise () {
	
		throw BadFormat();
	
	}
	
	
	void InsufficientBytes::Raise () {
	
		throw BadFormat();
	
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
		
			return *reinterpret_cast<Packet *>(
				reinterpret_cast<void *>(
					storage
				)
			);
		
		}
		
		
		const Packet & PacketContainer::Get () const noexcept {
		
			return *reinterpret_cast<const Packet *>(
				reinterpret_cast<const void *>(
					storage
				)
			);
		
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
		
			if (type::Count==0) BadPacketID::Raise();
			
			container.Imbue(
				destroy<type>,
				deserialize<type>
			);
		
			return true;
		
		}
	
		return false;
	
	}
	
	
	template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
	static typename std::enable_if<
		(ps==LI) && (pd==BO) && (id==Largest)
	>::type imbue_container (ProtocolState state, ProtocolDirection dir, UInt32 i, PacketContainer & container) {
	
		if (!check<ps,pd,id>(state,dir,i,container)) BadPacketID::Raise();
	
	}
	
	
	template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
	static typename std::enable_if<
		!((ps==LI) && (pd==BO) && (id==Largest))
	>::type imbue_container (ProtocolState state, ProtocolDirection dir, UInt32 i, PacketContainer & container) {
	
		if (!check<ps,pd,id>(state,dir,i,container)) imbue_container<
			((pd==BO) && (id==Largest)) ? Next(ps) : ps,
			(id==Largest) ? Next(pd) : pd,
			(id==Largest) ? 0 : (id+1)
		>(state,dir,i,container);
	
	}
	
	
	PacketParser::PacketParser () noexcept : in_progress(false) {	}
	
	
	bool PacketParser::FromBytes (Vector<Byte> & buffer, ProtocolState state, ProtocolDirection direction) {
	
		//	Prepare iterators for deserialization
		const Byte * begin=buffer.begin();
		const Byte * end=buffer.end();
	
		if (in_progress) {
		
			//	In progress -- we're waiting for
			//	a certain number of bytes in the
			//	buffer (as specified by the length
			//	header)
		
			if (buffer.Count()>=waiting_for) {
			
				//	Sufficient bytes -- attempt to
				//	deserialize packet
			
				//	Get the ID
				UInt32 id=Deserialize<PacketImpl::VarInt<UInt32>>(begin,end);
				
				//	Prepare the parser/container
				imbue_container<HS,CB,0>(state,direction,id,container);
				
				//	Attempt to populate remainder
				//	of packet
				container.FromBytes(begin,end);
				//container.FromBytes(begin,end);
				
				//	Put the ID in place
				container.Get().ID=id;
				
				//	We are finished with this packet
				in_progress=false;
				
				//	Delete consumed bytes from the
				//	buffer
				buffer.Delete(
					0,
					static_cast<Word>(begin-buffer.begin())
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
			waiting_for=Deserialize<PacketImpl::VarInt<UInt32>>(begin,end);
		
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
			static_cast<Word>(begin-buffer.begin())
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


}

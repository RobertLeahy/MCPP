/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <compression.hpp>
#include <nbt.hpp>
#include <json.hpp>
#include <traits.hpp>
#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <unordered_map>
#include <utility>


namespace MCPP {


	/**
	 *	Thrown when a packet contains incorrectly
	 *	formatted data.
	 */
	class BadFormat : public std::exception {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			[[noreturn]]
			static void Raise ();
			
			
			/**
			 *	\endcond
			 */
	
	
	};
	
	
	/**
	 *	Thrown when insufficient bytes
	 *	are provided to deserialize a
	 *	packet.
	 *
	 *	Represents a corrupted/bad length
	 *	header.
	 */
	class InsufficientBytes : public std::exception {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			[[noreturn]]
			static void Raise ();
			
			
			/**
			 *	\endcond
			 */
	
	
	};
	
	
	/**
	 *	Thrown when a bad packet ID is
	 *	detected.
	 */
	class BadPacketID : public std::exception {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			[[noreturn]]
			static void Raise ();
			
			
			/**
			 *	\endcond
			 */
	
	
	};


	/**
	 *	Represents the states that the protocol
	 *	may be in.
	 */
	enum class ProtocolState {
	
		Handshaking,	/**<	The client has just connected, and must specify which state it wishes to transition to.	*/
		Play,			/**<	The client is in game.	*/
		Status,			/**<	The client is requesting server status.	*/
		Login			/**<	The client is logging in.	*/
	
	};
	
	
	/**
	 *	Represents the direction that the protocol
	 *	may travel in.
	 */
	enum class ProtocolDirection {
	
		Clientbound,	/**<	Server to client.	*/
		Serverbound,	/**<	Client to server.	*/
		Both			/**<	Server to client and client to server.	*/
	
	};
	
	
	/**
	 *	Contains data about an item.
	 */
	class Slot {
	
	
		public:
		
		
			/**
			 *	The ID of the item.
			 */
			Int16 ItemID;
			/**
			 *	The number of the item in
			 *	this stack.
			 */
			Byte Count;
			/**
			 *	The damage value of the item.
			 */
			Int16 Damage;
			/**
			 *	Any NBT data associated with the
			 *	item.
			 */
			NBT::NamedTag Data;
	
	
	};
	
	
	/**
	 *	A type which may contain entity
	 *	metadata.
	 */
	typedef std::unordered_map<Byte,Variant<Byte,Int16,Int32,Single,String,Nullable<Slot>,Tuple<Int32,Int32,Int32>>> Metadata;
	
	
	typedef Variant<Int32,Tuple<Int32,Int16,Int16,Int16>> ObjectData;
	
	
	/**
	 *	The base class from which all packets
	 *	are derived.
	 */
	class Packet {
	
	
		protected:
		
		
			Packet (UInt32) noexcept;
			
			
			Packet (const Packet &) = default;
			Packet (Packet &&) = default;
			Packet & operator = (const Packet &) = default;
			Packet & operator = (Packet &&) = default;
	
	
		public:
		
		
			Packet () = delete;
		
		
			/**
			 *	The ID of this packet.
			 */
			UInt32 ID;
			
			
			/**
			 *	Retrieves a reference to a particular
			 *	kind of packet.
			 *
			 *	If the packet being retrieved does not
			 *	have the ID given by ID, the behaviour
			 *	of this function is undefined.
			 *
			 *	\tparam T
			 *		The type of the packet to retrieve.
			 *
			 *	\return
			 *		A reference to a packet of type
			 *		\em T.
			 */
			template <typename T>
			typename std::enable_if<
				std::is_base_of<Packet,T>::value,
				typename std::decay<T>::type
			>::type & Get () noexcept {
			
				return *reinterpret_cast<T *>(
					reinterpret_cast<void *>(
						this
					)
				);
			
			}
			
			
			/**
			 *	Retrieves a reference to a particular
			 *	kind of packet.
			 *
			 *	If the packet being retrieved does not
			 *	have the ID given by ID, the behaviour
			 *	of this function is undefined.
			 *
			 *	\tparam T
			 *		The type of the packet to retrieve.
			 *
			 *	\return
			 *		A reference to a packet of type
			 *		\em T.
			 */
			template <typename T>
			const typename std::enable_if<
				std::is_base_of<Packet,T>::value,
				typename std::decay<T>::type
			>::type & Get () const noexcept {
			
				return *reinterpret_cast<const T *>(
					reinterpret_cast<const void *>(
						this
					)
				);
			
			}
	
	
	};


	/**
	 *	\cond
	 */


	namespace PacketImpl {
	
	
		template <typename T>
		class VarInt {
		
		
			private:
			
			
				T value;
				
				
			public:
			
			
				VarInt () = default;
				VarInt (T value) noexcept : value(std::move(value)) {	}
				VarInt & operator = (T value) noexcept {
				
					this->value=value;
					
					return *this;
				
				}
				operator T () const noexcept {
				
					return value;
				
				}
		
		
		};
		
		
	}
	
	
}


namespace std {


	template <typename T>
	struct numeric_limits<MCPP::PacketImpl::VarInt<T>> : public numeric_limits<T> {	};
	template <typename T>
	struct is_signed<MCPP::PacketImpl::VarInt<T>> : public is_signed<T> {	};
	template <typename T>
	struct is_unsigned<MCPP::PacketImpl::VarInt<T>> : public is_unsigned<T> {	};


}


namespace MCPP {


	namespace PacketImpl {
		
		
		template <Word i, typename... Args>
		class MimicLayout : public MimicLayout<i-1,Args...> {
		
			
			public:
			
			
				typename GetType<i,Args...>::Type Value;
			
		
		};
		
		
		template <typename... Args>
		class MimicLayout<0,Args...> {
		
		
			public:
			
			
				typename GetType<0,Args...>::Type Value;
		
		
		};
		
		
		template <Word i>
		class MimicLayout<i> {	};
		
		
		template <Word i, typename... Args>
		class GetOffset {
		
		
			private:
			
			
				typedef MimicLayout<i,Args...> type;
		
		
			public:
			
			
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Winvalid-offsetof"
				constexpr static Word Value=offsetof(type,Value);
				#pragma GCC diagnostic pop
		
		
		};
	
	
		template <typename... Args>
		class PacketType {
		
		
			private:
			
			
				typedef VarInt<UInt32> id_type;
		
		
			public:
			
			
				template <Word i>
				class Types {
				
				
					public:
					
					
						typedef typename GetType<i,Args...>::Type Type;
						
						
						constexpr static Word Offset=GetOffset<i+1,id_type,Args...>::Value;
				
				
				};
				
				
				constexpr static Word Count=sizeof...(Args);
				
				
				constexpr static Word Size=sizeof(MimicLayout<sizeof...(Args),id_type,Args...>);
				
				
				constexpr static bool IsValid=true;
		
		
		};
		
		
		template <typename prefix, typename type>
		class Array {
		
		
			public:
			
			
				Vector<type> Value;
		
		
		};
		
		
		template <ProtocolState, ProtocolDirection, UInt32>
		class PacketMap : public PacketType<> {
		
		
			public:
			
			
				constexpr static bool IsValid=false;
		
		
		};
		
		
		constexpr ProtocolState HS=ProtocolState::Handshaking;
		constexpr ProtocolState PL=ProtocolState::Play;
		constexpr ProtocolState ST=ProtocolState::Status;
		constexpr ProtocolState LI=ProtocolState::Login;
		
		
		constexpr ProtocolDirection CB=ProtocolDirection::Clientbound;
		constexpr ProtocolDirection SB=ProtocolDirection::Serverbound;
		constexpr ProtocolDirection BO=ProtocolDirection::Both;
		
		
		typedef Int32 PLACEHOLDER;	//	Placeholder for unimplemented types
		
		
		//	HANDSHAKE
		
		//	SERVERBOUND
		template <> class PacketMap<HS,SB,0x00> : public PacketType<VarInt<UInt32>,String,UInt16,ProtocolState> {	};
		
		//	PLAY
		
		//	CLIENTBOUND
		template <> class PacketMap<PL,CB,0x00> : public PacketType<Int32> {	};
		template <> class PacketMap<PL,CB,0x01> : public PacketType<Int32,Byte,SByte,Byte,Byte,String> {	};
		template <> class PacketMap<PL,CB,0x02> : public PacketType<JSON::Value> {	};
		template <> class PacketMap<PL,CB,0x03> : public PacketType<Int64,Int64> {	};
		template <> class PacketMap<PL,CB,0x04> : public PacketType<Int32,Int16,Nullable<Slot>> {	};
		template <> class PacketMap<PL,CB,0x05> : public PacketType<Int32,Int32,Int32> {	};
		template <> class PacketMap<PL,CB,0x06> : public PacketType<Single,Int16,Single> {	};
		template <> class PacketMap<PL,CB,0x07> : public PacketType<Int32,Byte,Byte> {	};
		template <> class PacketMap<PL,CB,0x08> : public PacketType<Double,Double,Double,Single,Single,bool> {	};
		template <> class PacketMap<PL,CB,0x09> : public PacketType<Byte> {	};
		template <> class PacketMap<PL,CB,0x0A> : public PacketType<Int32,Int32,Byte,Int32> {	};
		template <> class PacketMap<PL,CB,0x0B> : public PacketType<VarInt<UInt32>,Byte> {	};
		template <> class PacketMap<PL,CB,0x0C> : public PacketType<VarInt<UInt32>,String,String,Int32,Int32,Int32,SByte,SByte,Int16,Metadata> {	};
		template <> class PacketMap<PL,CB,0x0D> : public PacketType<Int32,Int32> {	};
		template <> class PacketMap<PL,CB,0x0E> : public PacketType<VarInt<UInt32>,Byte,Int32,Int32,Int32,SByte,SByte,ObjectData> {	};
		template <> class PacketMap<PL,CB,0x0F> : public PacketType<VarInt<UInt32>,Byte,Int32,Int32,Int32,SByte,SByte,SByte,Int16,Int16,Int16,Metadata> {	};
		template <> class PacketMap<PL,CB,0x10> : public PacketType<VarInt<UInt32>,String,Int32,Int32,Int32,Int32> {	};
		template <> class PacketMap<PL,CB,0x11> : public PacketType<VarInt<UInt32>,Int32,Int32,Int32,Int16> {	};
		template <> class PacketMap<PL,CB,0x12> : public PacketType<Int32,Int16,Int16,Int16> {	};
		template <> class PacketMap<PL,CB,0x13> : public PacketType<Array<SByte,Int32>> {	};
		template <> class PacketMap<PL,CB,0x14> : public PacketType<Int32> {	};
		template <> class PacketMap<PL,CB,0x15> : public PacketType<Int32,SByte,SByte,SByte> {	};
		template <> class PacketMap<PL,CB,0x16> : public PacketType<Int32,SByte,SByte> {	};
		template <> class PacketMap<PL,CB,0x17> : public PacketType<Int32,SByte,SByte,SByte,SByte,SByte> {	};
		template <> class PacketMap<PL,CB,0x18> : public PacketType<Int32,Int32,Int32,Int32,SByte,SByte> {	};
		template <> class PacketMap<PL,CB,0x19> : public PacketType<Int32,SByte> {	};
		template <> class PacketMap<PL,CB,0x1A> : public PacketType<Int32,Byte> {	};
		template <> class PacketMap<PL,CB,0x1B> : public PacketType<Int32,Int32,bool> {	};
		template <> class PacketMap<PL,CB,0x1C> : public PacketType<Int32,Metadata> {	};
		template <> class PacketMap<PL,CB,0x1D> : public PacketType<Int32,Byte,Byte,Int16> {	};
		template <> class PacketMap<PL,CB,0x1E> : public PacketType<Int32,Byte> {	};
		template <> class PacketMap<PL,CB,0x1F> : public PacketType<Single,Int16,Int16> {	};
		template <> class PacketMap<PL,CB,0x20> : public PacketType<Int32,Array<Int32,Tuple<String,Double,Array<Int16,Tuple<UInt128,Double,Byte>>>>> {	};
		template <> class PacketMap<PL,CB,0x21> : public PacketType<Int32,Int32,bool,UInt16,UInt16,Array<Int32,Byte>> {	};
		//	0x22
		template <> class PacketMap<PL,CB,0x23> : public PacketType<Int32,Byte,UInt32,VarInt<UInt32>,Byte> {	};
		template <> class PacketMap<PL,CB,0x24> : public PacketType<Int32,Int16,Int32,Byte,Byte,VarInt<UInt32>> {	};
		template <> class PacketMap<PL,CB,0x25> : public PacketType<VarInt<UInt32>,Int32,Int32,Int32,Byte> {	};
		//	0x26
		template <> class PacketMap<PL,CB,0x27> : public PacketType<Single,Single,Single,Single,Array<Int32,Tuple<SByte,SByte,SByte>>,Single,Single,Single> {	};
		template <> class PacketMap<PL,CB,0x28> : public PacketType<Int32,Int32,Byte,Int32,Int32,bool> {	};
		template <> class PacketMap<PL,CB,0x29> : public PacketType<String,Int32,Int32,Int32,Single,Byte,Byte> {	};
		template <> class PacketMap<PL,CB,0x2A> : public PacketType<String,Single,Single,Single,Single,Single,Single,Single,Int32> {	};
		template <> class PacketMap<PL,CB,0x2B> : public PacketType<Byte,Single> {	};
		template <> class PacketMap<PL,CB,0x2C> : public PacketType<VarInt<UInt32>,Byte,Int32,Int32,Int32> {	};
		template <> class PacketMap<PL,CB,0x2D> : public PacketType<Byte,Byte,String,Byte,bool,Int32> {	};
		template <> class PacketMap<PL,CB,0x2E> : public PacketType<Byte> {	};
		template <> class PacketMap<PL,CB,0x2F> : public PacketType<Byte,Int16,Nullable<Slot>> {	};
		template <> class PacketMap<PL,CB,0x30> : public PacketType<Byte,Array<Int16,Nullable<Slot>>> {	};
		template <> class PacketMap<PL,CB,0x31> : public PacketType<Byte,Int16,Int16> {	};
		template <> class PacketMap<PL,CB,0x32> : public PacketType<Byte,Int16,bool> {	};
		template <> class PacketMap<PL,CB,0x33> : public PacketType<Int32,Int16,Int32,String,String,String,String> {	};
		template <> class PacketMap<PL,CB,0x34> : public PacketType<VarInt<UInt32>,Array<Int16,Byte>> {	};
		template <> class PacketMap<PL,CB,0x35> : public PacketType<Int32,Int16,Int32,Byte,NBT::NamedTag> {	};
		template <> class PacketMap<PL,CB,0x36> : public PacketType<Int32,Int32,Int32> {	};
		template <> class PacketMap<PL,CB,0x37> : public PacketType<Array<VarInt<UInt32>,Tuple<String,VarInt<UInt32>>>> {	};
		template <> class PacketMap<PL,CB,0x38> : public PacketType<String,bool,Int16> {	};
		template <> class PacketMap<PL,CB,0x39> : public PacketType<Byte,Single,Single> {	};
		template <> class PacketMap<PL,CB,0x3A> : public PacketType<Array<VarInt<UInt32>,String>> {	};
		template <> class PacketMap<PL,CB,0x3B> : public PacketType<String,String,Byte> {	};
		template <> class PacketMap<PL,CB,0x3C> : public PacketType<String,Byte,String,Int32> {	};
		template <> class PacketMap<PL,CB,0x3D> : public PacketType<Byte,String> {	};
		//	Conditional parsing mess
		//template <> class PacketMap<PL,CB,0x3E> : public PacketType<String,Byte,String,String,String,Byte,Array<Int16,String>> {	};
		template <> class PacketMap<PL,CB,0x3F> : public PacketType<String,Array<Int16,Byte>> {	};
		template <> class PacketMap<PL,CB,0x40> : public PacketType<String> {	};
		
		//	SERVERBOUND
		
		template <> class PacketMap<PL,SB,0x00> : public PacketType<Int32> {	};
		template <> class PacketMap<PL,SB,0x01> : public PacketType<String> {	};
		template <> class PacketMap<PL,SB,0x02> : public PacketType<Int32,Byte> {	};
		template <> class PacketMap<PL,SB,0x03> : public PacketType<bool> {	};
		template <> class PacketMap<PL,SB,0x04> : public PacketType<Double,Double,Double,Double,bool> {	};
		template <> class PacketMap<PL,SB,0x05> : public PacketType<Single,Single,bool> {	};
		template <> class PacketMap<PL,SB,0x06> : public PacketType<Double,Double,Double,Double,Single,Single,bool> {	};
		template <> class PacketMap<PL,SB,0x07> : public PacketType<Byte,Int32,Byte,Int32,Byte> {	};
		template <> class PacketMap<PL,SB,0x08> : public PacketType<Int32,Byte,Int32,SByte,Nullable<Slot>,SByte,SByte,SByte> {	};
		template <> class PacketMap<PL,SB,0x09> : public PacketType<Int16> {	};
		template <> class PacketMap<PL,SB,0x0A> : public PacketType<Int32,Byte> {	};
		template <> class PacketMap<PL,SB,0x0B> : public PacketType<Int32,Byte,Int32> {	};
		template <> class PacketMap<PL,SB,0x0C> : public PacketType<Single,Single,bool,bool> {	};
		template <> class PacketMap<PL,SB,0x0D> : public PacketType<Byte> {	};
		template <> class PacketMap<PL,SB,0x0E> : public PacketType<Byte,Int16,Byte,Int16,Byte,Nullable<Slot>> {	};
		template <> class PacketMap<PL,SB,0x0F> : public PacketType<Byte,Int16,bool> {	};
		template <> class PacketMap<PL,SB,0x10> : public PacketType<Int16,Nullable<Slot>> {	};
		template <> class PacketMap<PL,SB,0x11> : public PacketType<Byte,Byte> {	};
		template <> class PacketMap<PL,SB,0x12> : public PacketType<Int32,Int16,Int32,String,String,String,String> {	};
		template <> class PacketMap<PL,SB,0x13> : public PacketType<Byte,Single,Single> {	};
		template <> class PacketMap<PL,SB,0x14> : public PacketType<String> {	};
		template <> class PacketMap<PL,SB,0x15> : public PacketType<String,Byte,Byte,bool,Byte,bool> {	};
		template <> class PacketMap<PL,SB,0x16> : public PacketType<Byte> {	};
		template <> class PacketMap<PL,SB,0x17> : public PacketType<String,Array<Int16,Byte>> {	};
		
		//	STATUS
		
		//	CLIENTBOUND
		template <> class PacketMap<ST,CB,0x00> : public PacketType<JSON::Value> {	};
		template <> class PacketMap<ST,CB,0x01> : public PacketType<Int64> {	};
		
		//	SERVERBOUND
		template <> class PacketMap<ST,SB,0x00> : public PacketType<> {	};
		template <> class PacketMap<ST,SB,0x01> : public PacketType<Int64> {	};
		
		//	LOGIN
		
		//	CLIENTBOUND
		template <> class PacketMap<LI,CB,0x00> : public PacketType<JSON::Value> {	};
		template <> class PacketMap<LI,CB,0x01> : public PacketType<String,Array<Int16,Byte>,Array<Int16,Byte>> {	};
		template <> class PacketMap<LI,CB,0x02> : public PacketType<String,String> {	};
		
		//	SERVERBOUND
		template <> class PacketMap<LI,SB,0x00> : public PacketType<String> {	};
		template <> class PacketMap<LI,SB,0x01> : public PacketType<Array<Int16,Byte>,Array<Int16,Byte>> {	};
		
		
		constexpr UInt32 LargestID=0x40;
		
		
		constexpr ProtocolState Next (ProtocolState ps) noexcept {
		
			return (ps==HS) ? PL : ((ps==PL) ? ST : LI);
		
		}
		
		
		constexpr ProtocolDirection Next (ProtocolDirection pd) noexcept {
		
			return (pd==CB) ? SB : ((pd==SB) ? BO : CB);
		
		}
		
		
		template <typename T>
		constexpr T Max (T a, T b) noexcept {
		
			return (a>b) ? a : b;
		
		}
		
		
		template <ProtocolState ps, ProtocolDirection pd, UInt32 id>
		class LargestImpl {
		
		
			public:
			
			
				constexpr static Word Value=Max(
					PacketMap<ps,pd,id>::Size,
					LargestImpl<ps,pd,id+1>::Value
				);
		
		
		};
		
		
		template <ProtocolState ps, ProtocolDirection pd>
		class LargestImpl<ps,pd,LargestID> {
		
		
			public:
			
			
				constexpr static Word Value=Max(
					PacketMap<ps,pd,LargestID>::Size,
					LargestImpl<
						(pd==BO) ? Next(ps) : ps,
						Next(pd),
						0
					>::Value
				);
		
		
		};
		
		
		template <>
		class LargestImpl<LI,BO,LargestID> {
		
		
			public:
			
			
				constexpr static Word Value=PacketMap<LI,BO,LargestID>::Size;
		
		
		};
		
		
		constexpr Word Largest=LargestImpl<HS,CB,0>::Value;
		
		
		class PacketContainer {
		
		
			public:
			
			
				typedef void (*destroy_type) (void *);
				typedef void (*from_bytes_type) (const Byte * &, const Byte *, void *);
			
			
				destroy_type destroy;
				from_bytes_type from_bytes;
				bool engaged;
				union {
					Byte storage [Largest];
					Packet packet;
				};
				
				
				inline void destroy_impl () noexcept;
		
		
			public:
			
			
				PacketContainer () noexcept;
			
			
				PacketContainer (const PacketContainer &) = delete;
				PacketContainer (PacketContainer &&) = delete;
				PacketContainer & operator = (const PacketContainer &) = delete;
				PacketContainer & operator = (PacketContainer &&) = delete;
			
			
				~PacketContainer () noexcept;
				
				
				void FromBytes (const Byte * &, const Byte *);
				Packet & Get () noexcept;
				const Packet & Get () const noexcept;
				
				
				void Imbue (destroy_type, from_bytes_type) noexcept;
		
		
		};
		
		
		template <typename T>
		class Serializer {
		
		
			public:
			
			
				constexpr static Word Size (const T &) noexcept {
				
					return sizeof(T);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Make sure there's enough space
					//	to extract an object of this
					//	type from the buffer
					if ((end-begin)<sizeof(T)) InsufficientBytes::Raise();
					
					if (Endianness::IsBigEndian<T>()) {
					
						//	If the system is big endian, simply
						//	copy directly from the buffer to
						//	the object pointer
					
						std::memcpy(ptr,begin,sizeof(T));
					
					} else {
					
						//	Otherwise reverse byte order as we
						//	copy
						
						for (Word i=0;i<sizeof(T);++i) reinterpret_cast<Byte *>(ptr)[i]=begin[sizeof(T)-i-1];
					
					}
					
					//	Advance pointer
					begin+=sizeof(T);
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const T & obj) {
				
					//	Resize buffer as/if necessary
					while ((buffer.Capacity()-buffer.Count())<sizeof(T)) buffer.SetCapacity();
					
					auto end=buffer.end();
					
					//	If the system is big endian, we can
					//	copy directly, otherwise reverse
					//	byte order as we copy
					if (Endianness::IsBigEndian<T>()) std::memcpy(end,&obj,sizeof(T));
					else for (Word i=0;i<sizeof(T);++i) end[i]=reinterpret_cast<const Byte *>(
						reinterpret_cast<const void *>(
							&obj
						)
					)[sizeof(T)-i-1];
					
					//	Set buffer count
					buffer.SetCount(buffer.Count()+sizeof(T));
				
				}
		
		
		};
		
		
		template <typename T>
		T Deserialize (const Byte * & begin, const Byte * end) {
		
			alignas(T) Byte buffer [sizeof(T)];
			
			Serializer<T>::FromBytes(begin,end,buffer);
			
			Nullable<T> retr;
			T * ptr=reinterpret_cast<T *>(
				reinterpret_cast<void *>(
					buffer
				)
			);
			
			try {
			
				retr.Construct(std::move(*ptr));
				
			} catch (...) {
			
				ptr->~T();
				
				throw;
			
			}
			
			ptr->~T();
			
			return std::move(*retr);
		
		}
		
		
		template <typename T>
		class Serializer<VarInt<T>> {
		
		
			private:
			
			
				typedef typename std::make_unsigned<T>::type type;
				
				
				constexpr static bool is_signed=std::is_signed<T>::value;
				
				
				constexpr static Word bits=sizeof(T)*BitsPerByte();
				
				
				constexpr static Word max_bytes=((bits%7)==0) ? (bits/7) : ((bits/7)+1);
				
				
				constexpr static Byte get_final_mask_impl (Word target, Word i) noexcept {
				
					return (i==target) ? 0 : ((static_cast<Byte>(1)<<i)|get_final_mask_impl(target,i+1));
				
				}
				
				
				constexpr static Byte get_final_mask () noexcept {
				
					return (~get_final_mask_impl(bits%7,0))&127;
				
				}
		
		
			public:
			
			
				static Word Size (VarInt<T> obj) noexcept {
				
					union {
						type value;
						T s_value;
					};
					
					s_value=obj;
					
					UInt32 mask=127;
					for (Word i=1;i<(max_bytes-1);++i) {
					
						if ((value&~mask)==0) return i;
						
						mask<<=7;
						mask|=127;
					
					}
					
					return max_bytes;
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Build a value from the bytes in
					//	the buffer -- start at zero
					type value=0;
					
					//	When this is set to true, we've
					//	reached the end of the VarInt,
					//	i.e. the msb was not set
					bool complete=false;
					for (
						Word i=1;
						!((begin==end) || complete);
						++i
					) {
					
						Byte b=*(begin++);
						
						//	If msb is set, we're done
						//	after considering this byte
						if ((b&128)==0) complete=true;
						//	Otherwise we ignore msb
						else b&=~static_cast<Byte>(128);
						
						//	On the last byte we do some
						//	special processing to make sure
						//	that the integer we're decoding
						//	doesn't overflow
						if (
							(i==max_bytes) &&
							//	We need to make sure bits
							//	that would cause an overflow
							//	are not set
							((i&get_final_mask())!=0)
						) BadFormat::Raise();
						
						//	Add the value of this byte to
						//	the value we're building
						value|=static_cast<type>(b)<<(7*(i-1));
						
						//	If this is the last possible
						//	byte, and we've not reached
						//	the end of the VarInt, there's
						//	a problem with the format of
						//	the data
						if (
							!complete &&
							(i==max_bytes)
						) BadFormat::Raise();
					
					}
					
					//	If the VarInt is not complete,
					//	we didn't finish
					if (!complete) InsufficientBytes::Raise();
					
					//	Convert bitwise representation over
					//	to signed if necessary
					union {
						type built;
						T final;
					};
					
					built=value;
					
					new (ptr) VarInt<T> (final);
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, VarInt<T> obj) {
				
					union {
						type value;
						T s_value;
					};
					
					s_value=obj;
					
					do {
					
						//	Get bottom seven bits
						Byte b=static_cast<Byte>(value&127);
						
						//	Discard bottom seven bits
						value>>=7;
						
						//	Set msb if necessary
						if (value!=0) b|=128;
						
						//	Add
						buffer.Add(b);
					
					} while (value!=0);
				
				}
		
		
		};
		
		
		template <>
		class Serializer<String> {
		
		
			private:
			
			
				typedef UInt32 size_type;
				typedef VarInt<size_type> var_int_type;
		
		
			public:
			
			
				static Word Size (const String & obj) {
				
					SafeWord safe(obj.Size());
					
					//	Maximum number of UTF-8 code units per code point
					safe*=4;
					
					//	Size of the VarInt which describes the maximum
					//	number of code units (i.e. bytes) in the string
					var_int_type var_int=size_type(safe);
					
					//	Add VarInt byte count to code unit (i.e. byte)
					//	count
					safe+=SafeWord(Serializer<decltype(var_int)>::Size(var_int));
					
					return Word(safe);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get length of string in bytes
					var_int_type len=Deserialize<decltype(len)>(begin,end);
					
					//	Check to make sure there's enough
					//	bytes for this string to actually
					//	be in the buffer
					if ((end-begin)<size_type(len)) InsufficientBytes::Raise();
					
					auto start=begin;
					begin+=size_type(len);
					
					//	Decode
					new (ptr) String (UTF8().Decode(start,begin));
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const String & obj) {
				
					//	Get UTF-8 encoding of string
					auto encoded=UTF8().Encode(obj);
					
					//	Place the VarInt encoding of the
					//	string in the buffer
					Serializer<var_int_type>::ToBytes(
						buffer,
						size_type(SafeWord(encoded.Count()))
					);
					
					//	Make enough space in the buffer
					//	for the string
					while ((buffer.Capacity()-buffer.Count())<encoded.Count()) buffer.SetCapacity();
					
					//	Copy
					std::memcpy(
						buffer.end(),
						encoded.begin(),
						encoded.Count()
					);
					
					buffer.SetCount(buffer.Count()+encoded.Count());
				
				}
		
		
		};
		
		
		template <>
		class Serializer<JSON::Value> {
		
		
			private:
			
			
				//	Maximum recursion the JSON parser
				//	will be willing to go through before
				//	bailing out and throwing when parsing
				//	incoming JSON
				constexpr static Word max_recursion=10;
		
		
			public:
			
			
				constexpr static Word Size (const JSON::Value &) noexcept {
				
					//	This is unknowable without actually
					//	serializing
					return 0;
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get the string from which we'll
					//	extract JSON
					auto str=Deserialize<String>(begin,end);
					
					//	Parse JSON
					new (ptr) JSON::Value (JSON::Parse(str,max_recursion));
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const JSON::Value & obj) {
				
					Serializer<String>::ToBytes(
						buffer,
						JSON::Serialize(obj)
					);
				
				}
		
		
		};
		
		
		template <>
		class Serializer<ProtocolState> {
		
		
			public:
			
			
				constexpr static Word Size (ProtocolState) noexcept {
				
					//	Only ever 1 byte
					return 1;
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get the raw byte
					auto b=Deserialize<Byte>(begin,end);
					
					//	Only valid values are 1 or 2
					if (!((b==1) || (b==2))) BadFormat::Raise();
					
					//	Create
					new (ptr) ProtocolState ((b==1) ? ProtocolState::Status : ProtocolState::Login);
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, ProtocolState state) {
				
					Byte b;
					switch (state) {
					
						case ProtocolState::Status:
							b=1;
							break;
						case ProtocolState::Login:
							b=2;
							break;
						default:
							BadFormat::Raise();
					
					}
					
					Serializer<Byte>::ToBytes(buffer,b);
				
				}
		
		
		};
		
		
		template <typename T>
		class GetIntegerType {
		
		
			public:
			
			
				typedef T Type;
		
		
		};
		
		
		template <typename T>
		class GetIntegerType<VarInt<T>> {
		
		
			public:
			
			
				typedef T Type;
		
		
		};

		
		template <typename prefix, typename inner>
		class Serializer<Array<prefix,inner>> {
		
		
			private:
			
			
				typedef Array<prefix,inner> type;
				typedef typename GetIntegerType<prefix>::Type int_type;
		
		
			public:
			
			
				static Word Size (const type & obj) {
				
					SafeWord size=Serializer<prefix>::Size(obj.Value.Count());
					
					for (const auto & i : obj.Value) size+=SafeWord(Serializer<inner>::Size(i));
					
					return Word(size);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get array count
					auto count=Deserialize<prefix>(begin,end);
					
					//	Check to make sure array
					//	count is not negative
					if (
						std::is_signed<prefix>::value &&
						(count<0)
					) BadFormat::Raise();
					
					Vector<inner> vec;
					
					//	Loop and get each element in
					//	the array
					for (Word i=0;i<count;++i) vec.Add(Deserialize<inner>(begin,end));
					
					new (ptr) Vector<inner> (std::move(vec));
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const type & obj) {
				
					//	Serialize count
					Serializer<prefix>::ToBytes(
						buffer,
						int_type(SafeWord(obj.Value.Count()))
					);
					
					//	Serialize each object
					for (const auto & i : obj.Value) Serializer<inner>::ToBytes(buffer,i);
				
				}
		
		
		};
		
		
		template <typename... Args>
		class Serializer<Tuple<Args...>> {
		
		
			private:
			
			
				typedef Tuple<Args...> type;
				
				
				template <Word i>
				constexpr static typename std::enable_if<
					i>=sizeof...(Args),
					Word
				>::type size_impl (const type &) noexcept {
				
					return 0;
				
				}
			
			
				template <Word i>
				static typename std::enable_if<
					i<sizeof...(Args),
					Word
				>::type size_impl (const type & obj) {
				
					const auto & curr=obj.template Item<i>();
					
					return Word(
						SafeWord(Serializer<typename std::decay<decltype(curr)>::type>::Size(curr))+
						SafeWord(size_impl<i+1>(obj))
					);
				
				}
				
				
				template <Word i>
				static typename std::enable_if<
					i>=sizeof...(Args)
				>::type serialize_impl (Vector<Byte> &, const type &) noexcept {	}
				
				
				template <Word i>
				static typename std::enable_if<
					i<sizeof...(Args)
				>::type serialize_impl (Vector<Byte> & buffer, const type & obj) {
				
					const auto & curr=obj.template Item<i>();
					
					Serializer<typename std::decay<decltype(curr)>::type>::ToBytes(buffer,curr);
					
					serialize_impl<i+1>(buffer,obj);
				
				}
				
				
				template <Word i>
				static typename std::enable_if<
					i>=sizeof...(Args)
				>::type deserialize_impl (const Byte * &, const Byte *, type &) noexcept {	}
				
				
				template <Word i>
				static typename std::enable_if<
					i<sizeof...(Args)
				>::type deserialize_impl (const Byte * & begin, const Byte * end, type & obj) {
				
					auto & curr=obj.template Item<i>();
					
					typedef typename std::decay<decltype(curr)>::type curr_type;
					
					Serializer<curr_type>::FromBytes(begin,end,&curr);
					
					try {
					
						deserialize_impl<i+1>(begin,end,obj);
					
					} catch (...) {
					
						curr.~curr_type();
						
						throw;
					
					}
				
				}
		
			
			public:
			
			
				static Word Size (const type & obj) {
				
					return size_impl<0>(obj);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					deserialize_impl<0>(
						begin,
						end,
						*reinterpret_cast<type *>(ptr)
					);
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const type & obj) {
				
					serialize_impl<0>(buffer,obj);
				
				}
			
		
		};
		
		
		template <>
		class Serializer<ObjectData> {
		
		
			private:
			
			
				typedef Tuple<Int32,Int16,Int16,Int16> inner;
		
		
			public:
			
			
				static Word Size (const ObjectData & obj) {
				
					if (obj.Is<Int32>()) return Serializer<Int32>::Size(obj.Get<Int32>());
					
					return Serializer<inner>::Size(obj.Get<inner>());
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Always begins with a 32-bit
					//	signed integer
					auto leading=Deserialize<Int32>(begin,end);
					
					//	If it's zero, nothing follows
					if (leading==0) {
					
						new (ptr) ObjectData (leading);
						
						return;
					
					}
					
					//	Otherwise a tuple follows
					
					new (ptr) ObjectData ();
					ObjectData & obj=*reinterpret_cast<ObjectData *>(ptr);
					
					//	We're responsible for the lifetime
					//	of the variant
					try {
					
						obj.Construct<inner>();
						auto & t=obj.Get<inner>();
						
						t.Item<0>()=leading;
						t.Item<1>()=Deserialize<Int16>(begin,end);
						t.Item<2>()=Deserialize<Int16>(begin,end);
						t.Item<3>()=Deserialize<Int16>(begin,end);
					
					} catch (...) {
					
						obj.~ObjectData();
						
						throw;
					
					}
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const ObjectData & obj) {
				
					//	Get/send the leading integer
				
					auto leading=obj.Is<Int32>() ? obj.Get<Int32>() : obj.Get<inner>().Item<0>();
					
					Serializer<Int32>::ToBytes(buffer,leading);
					
					if (leading==0) return;
					
					//	If the leading integer is not
					//	zero, we must send a tuple
					//	regardless of the in memory
					//	structure
					
					Tuple<Int16,Int16,Int16> t(0,0,0);
					
					if (obj.Is<inner>()) {
					
						auto & i=obj.Get<inner>();
						
						t.Item<0>()=i.Item<1>();
						t.Item<1>()=i.Item<2>();
						t.Item<2>()=i.Item<3>();
					
					}
					
					Serializer<decltype(t)>::ToBytes(buffer,t);
				
				}
		
		
		};
		
		
		template <>
		class Serializer<NBT::NamedTag> {
		
		
			private:
			
			
				//	Maximum recursive depth to which the
				//	NBT parser will descend.
				constexpr static Word max_recursion=10;
		
		
			public:
			
			
				constexpr static Word Size (const NBT::NamedTag &) noexcept {
				
					//	We'd have to parse out the whole
					//	thing and compress it -- just return
					//	zero
					return 0;
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Read length
					auto len=Deserialize<Int16>(begin,end);
					
					//	Is there NBT data to read?
					if (len<0) {
					
						//	NO
						
						new (ptr) NBT::NamedTag ();
						
						return;
					
					}
					
					//	YES
					
					//	Is there a sufficient number of bytes?
					if ((end-begin)<len) InsufficientBytes::Raise();
					
					//	Decompress and parse
					
					auto decompressed=Inflate(begin,begin+len);
					
					const Byte * nbt_begin=decompressed.begin();
					
					new (ptr) NBT::NamedTag (
						NBT::Parse(
							nbt_begin,
							decompressed.end(),
							max_recursion
						)
					);
					
					//	Advance pointer
					begin+=len;
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const NBT::NamedTag & obj) {
				
					//	If there's no data, we just write out
					//	-1 as the length and bail out
					if (obj.Payload.IsNull()) {
					
						Serializer<Int16>::ToBytes(buffer,-1);
						
						return;
					
					}
					
					//	There's NBT data to write
					
					Int16 size;
					
					//	Make enough space for the size
					//	(since it's unknown at the present
					//	time we'll add it after serializing
					//	and compressing)
					while ((buffer.Capacity()-buffer.Count())<Serializer<Int16>::Size(size)) buffer.SetCapacity();
					//	Record position so we can come back
					//	here after serializing and compressing
					Word size_pos=buffer.Count();
					//	Advance position
					buffer.SetCount(buffer.Count()+Serializer<Int16>::Size(size));
					//	Record start of NBT for size calculation
					Word nbt_start=buffer.Count();
					
					//	Serialize
					auto nbt=NBT::Serialize(obj);
					
					//	Compress (GZip)
					Deflate(nbt.begin(),nbt.end(),&buffer,true);
					
					//	Output size
					size=Int16(SafeWord(buffer.Count()-nbt_start));
					Word nbt_end=buffer.Count();
					buffer.SetCount(size_pos);
					Serializer<Int16>::ToBytes(buffer,size);
					buffer.SetCount(nbt_end);
				
				}
		
		
		};
		
		
		template <>
		class Serializer<Nullable<Slot>> {
		
		
			public:
			
			
				static Word Size (const Nullable<Slot> & obj) {
				
					if (obj.IsNull()) return Serializer<Int16>::Size(-1);
					
					SafeWord size(Serializer<Int16>::Size(obj->ItemID));
					size+=Serializer<Byte>::Size(obj->Count);
					size+=Serializer<Int16>::Size(obj->Damage);
					
					//	Don't bother with the NBT, we'd
					//	have to serialize and compress it,
					//	too expensive
					
					return Word(size);
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					//	Get the item ID
					auto id=Deserialize<Int16>(begin,end);
					
					//	Initialize item nulled
					new (ptr) Nullable<Slot> ();
					
					//	If the item ID is negative,
					//	there's no slot data
					if (id<0) return;
					
					auto & slot=*reinterpret_cast<Nullable<Slot> *>(ptr);
					
					//	We're responsible for the
					//	above placement new constructed
					//	object's lifetime
					try {
					
						//	Default construct the slot
						//	object
						slot.Construct();
						
						slot->ItemID=id;
						slot->Count=Deserialize<Byte>(begin,end);
						slot->Damage=Deserialize<Int16>(begin,end);
						slot->Data=Deserialize<NBT::NamedTag>(begin,end);
					
					} catch (...) {
					
						slot.~Nullable<Slot>();
						
						throw;
					
					}
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const Nullable<Slot> & obj) {
				
					//	If the slot is null, just write -1
					//	and bail out
					if (obj.IsNull()) {
					
						Serializer<Int16>::ToBytes(buffer,-1);
						
						return;
					
					}
					
					//	Otherwise we have to write out the
					//	whole object
					Serializer<Int16>::ToBytes(buffer,obj->ItemID);
					Serializer<Byte>::ToBytes(buffer,obj->Count);
					Serializer<Int16>::ToBytes(buffer,obj->Damage);
					Serializer<NBT::NamedTag>::ToBytes(buffer,obj->Data);
				
				}
		
		
		};
		
		
		template <>
		class Serializer<Metadata> {
		
		
			private:
			
			
				typedef Metadata::value_type::second_type inner;
				typedef Tuple<Int32,Int32,Int32> coord;
		
		
			public:
			
			
				constexpr static Word Size () noexcept {
				
					//	Too expensive to calculate, just
					//	return zero
					return 0;
				
				}
				
				
				static void FromBytes (const Byte * & begin, const Byte * end, void * ptr) {
				
					new (ptr) Metadata ();
					Metadata & dict=*reinterpret_cast<Metadata *>(ptr);
					
					//	We're responsible for this
					//	object's lifetime
					try {
					
						//	Parse until 127 is read
						Byte b;
						while ((b=Deserialize<Byte>(begin,end))!=127) {
						
							//	Decompose byte
							Byte key=b&31;
							
							//	If the key already exists in the
							//	dictionary, bail out immediately
							if (dict.count(key)!=0) BadFormat::Raise();
							
							Byte type=b&224;
							
							inner variant;
							
							switch (type) {
							
								//	Byte
								case 0:
									variant=Deserialize<Byte>(begin,end);
									break;
								//	Signed 16-bit integer
								case 1:
									variant=Deserialize<Int16>(begin,end);
									break;
								//	Signed 32-bit integer
								case 2:
									variant=Deserialize<Int32>(begin,end);
									break;
								//	Single-precision floating point number
								case 3:
									variant=Deserialize<Single>(begin,end);
									break;
								//	String
								case 4:
									variant=Deserialize<String>(begin,end);
									break;
								//	Slot data
								case 5:
									variant=Deserialize<Nullable<Slot>>(begin,end);
									break;
								break;
								//	Coordinates
								case 6:
									variant=Deserialize<coord>(begin,end);
									break;
								default:
									BadFormat::Raise();
							
							}
							
							dict.emplace(
								key,
								std::move(variant)
							);
						
						}
					
					} catch (...) {
					
						dict.~Metadata();
						
						throw;
					
					}
				
				}
				
				
				static void ToBytes (Vector<Byte> & buffer, const Metadata & obj) {
				
					//	Loop over each pair
					for (const auto & pair : obj) {
					
						auto & v=pair.second;
					
						if (
							//	If the key is over 31, we can't
							//	encode
							(pair.first>31) ||
							//	We have no way of representing
							//	nulls
							v.IsNull()
						) BadFormat::Raise();
						
						//	Get the type of this object
						Byte type=static_cast<Byte>(v.Type());
						
						//	Output key and type packed
						//	into a single byte
						Serializer<Byte>::ToBytes(buffer,pair.first|type);
						
						//	Serialize/get type
						switch (type) {

							//	Byte
							case 0:
								Serializer<Byte>::ToBytes(buffer,v.Get<Byte>());
								break;
							//	Signed 16-bit integer
							case 1:
								Serializer<Int16>::ToBytes(buffer,v.Get<Int16>());
								break;
							//	Signed 32-bit integer
							case 2:
								Serializer<Int32>::ToBytes(buffer,v.Get<Int32>());
								break;
							//	Single-precision floating point number
							case 3:
								Serializer<Single>::ToBytes(buffer,v.Get<Single>());
								break;
							//	String
							case 4:
								Serializer<String>::ToBytes(buffer,v.Get<String>());
								break;
							//	Slot data
							case 5:
								Serializer<Nullable<Slot>>::ToBytes(buffer,v.Get<Nullable<Slot>>());
								break;
							//	Coordinates
							case 6:
							default:
								Serializer<coord>::ToBytes(buffer,v.Get<coord>());
								break;
						
						}
					
					}
					
					//	Write a terminating 127 byte
					Serializer<Byte>::ToBytes(buffer,127);
				
				}
		
		
		};
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	 
	
	/**
	 *	Parses packets from a buffer of bytes.
	 */
	class PacketParser {
	
	
		private:
		
		
			PacketImpl::PacketContainer container;
			bool in_progress;
			Word waiting_for;
			
			
		public:
		
		
			/**
			 *	Creates a new packet parser.
			 */
			PacketParser () noexcept;
		
		
			/**
			 *	Attempts to construct a packet from
			 *	a buffer of bytes.
			 *
			 *	\param [in,out] buffer
			 *		A buffer of bytes.  Consumed bytes
			 *		will be removed.
			 *	\param [in] state
			 *		The current state of the protocol.
			 *	\param [in] direction
			 *		The direction of the buffer of bytes
			 *		to parse.
			 *
			 *	\return
			 *		\em true if a packet was parsed,
			 *		\em false otherwise.  Whether \em true
			 *		or \em false is returned, \em buffer
			 *		may have been altered.
			 */
			bool FromBytes (Vector<Byte> & buffer, ProtocolState state, ProtocolDirection direction);
			
			
			/**
			 *	Returns a reference to the internally
			 *	allocated packet.
			 *
			 *	The reference returned by this function
			 *	always refers to the internally allocated
			 *	packet, therefore accessing it, and calling
			 *	this function, is only defined while this
			 *	object exists, and after a call to FromBytes
			 *	returns \em true, and before a subsequenc call
			 *	to FromBytes.
			 *
			 *	\return
			 *		A reference to an internally allocated
			 *		packet.
			 */
			Packet & Get () noexcept;
			/**
			 *	Returns a reference to the internally
			 *	allocated packet.
			 *
			 *	The reference returned by this function
			 *	always refers to the internally allocated
			 *	packet, therefore accessing it, and calling
			 *	this function, is only defined while this
			 *	object exists, and after a call to FromBytes
			 *	returns \em true, and before a subsequenc call
			 *	to FromBytes.
			 *
			 *	\return
			 *		A reference to an internally allocated
			 *		packet.
			 */
			const Packet & Get () const noexcept;
	
	
	};
	
	
	namespace Packets {
	
	
		/**
		 *	\cond
		 */
	
	
		class HSPacket {
		
		
			public:
			
			
				constexpr static ProtocolState State=ProtocolState::Handshaking;
		
		
		};
		
		
		class PLPacket {
		
		
			public:
			
			
				constexpr static ProtocolState State=ProtocolState::Play;
		
		
		};
		
		
		class STPacket {
		
		
			public:
			
			
				constexpr static ProtocolState State=ProtocolState::Status;
		
		
		};
		
		
		class LIPacket {
		
		
			public:
			
			
				constexpr static ProtocolState State=ProtocolState::Login;
		
		
		};
		
		
		class CBPacket {
		
		
			public:
			
			
				constexpr static ProtocolDirection Direction=ProtocolDirection::Clientbound;
		
		
		};
		
		
		class SBPacket {
		
		
			public:
			
			
				constexpr static ProtocolDirection Direction=ProtocolDirection::Serverbound;
		
		
		};
		
		
		class BOPacket {
		
		
			public:
			
			
				constexpr static ProtocolDirection Direction=ProtocolDirection::Both;
		
		
		};
		
		
		template <UInt32 id>
		class IDPacket : public Packet {
		
		
			public:
			
			
				constexpr static UInt32 PacketID=id;
				
				
				IDPacket () noexcept : Packet(id) {	}
		
		
		};
		
		
		/**
		 *	\endcond
		 */
	
	
		namespace Handshaking {
		
		
			namespace Serverbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public HSPacket, public SBPacket {	};
				
				
				/**
				 *	\endcond
				 */
			
			
				class Handshake : public Base, public IDPacket<0x00> {
				
				
					public:
					
					
						UInt32 ProtocolVersion;
						String ServerAddress;
						UInt16 ServerPort;
						ProtocolState CurrentState;
				
				
				};
			
			
			}
		
		
		}
		
		
		namespace Play {
		
		
			namespace Clientbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public PLPacket, public CBPacket {	};
				
				
				/**
				 *	\endcond
				 */
				 
				 
				class KeepAlive : public Base, public IDPacket<0x00> {
				
				
					public:
					
					
						Int32 KeepAliveID;
				
				
				};
				
				
				class JoinGame : public Base, public IDPacket<0x01> {
				
				
					public:
					
					
						Int32 EntityID;
						Byte GameMode;
						SByte Dimension;
						Byte Difficulty;
						Byte MaxPlayers;
						String LevelType;
				
				
				};
				
				
				class ChatMessage : public Base, public IDPacket<0x02> {
				
				
					public:
					
					
						JSON::Value Value;
				
				
				};
				
				
				class TimeUpdate : public Base, public IDPacket<0x03> {
				
				
					public:
					
					
						Int64 Age;
						Int64 Time;
				
				
				};
				
				
				class EntityEquipment : public Base, public IDPacket<0x04> {
				
				
					public:
					
					
						Int32 EntityID;
						Int16 EquipmentSlot;
						Nullable<Slot> Data;
				
				
				};
				
				
				class SpawnPosition : public Base, public IDPacket<0x05> {
				
				
					public:
					
					
						Int32 X;
						Int32 Y;
						Int32 Z;
				
				
				};
				
				
				class UpdateHealth : public Base, public IDPacket<0x06> {
				
				
					public:
					
					
						Single Health;
						Int16 Food;
						Single Saturation;
				
				
				};
				
				
				class Respawn : public Base, public IDPacket<0x07> {
				
				
					public:
					
					
						Int32 Dimension;
						Byte Difficulty;
						Byte GameMode;
				
				
				};
				
				
				class PlayerPositionAndLook : public Base, public IDPacket<0x08> {
				
				
					public:
					
					
						Double X;
						Double Y;
						Double Z;
						Single Yaw;
						Single Pitch;
						bool OnGround;
				
				
				};
				
				
				class HeldItemChange : public Base, public IDPacket<0x09> {
				
				
					public:
					
					
						Byte Selected;
				
				
				};
				
				
				class UseBed : public Base, public IDPacket<0x0A> {
				
				
					public:
					
					
						Int32 EntityID;
						Int32 X;
						Byte Y;
						Int32 Z;
				
				
				};
				
				
				class Animation : public Base, public IDPacket<0x0B> {
				
				
					public:
					
					
						Int32 EntityID;
						Byte AnimationID;
				
				
				};
				
				
				class SpawnPlayer : public Base, public IDPacket<0x0C> {
				
				
					public:
					
					
						UInt32 EntityID;
						String UUID;
						String Name;
						Int32 X;
						Int32 Y;
						Int32 Z;
						Single Yaw;
						Single Pitch;
						Int16 CurrentItem;
				
				
				};
				
				
				class CollectItem : public Base, public IDPacket<0x0D> {
				
				
					public:
					
					
						Int32 Collected;
						Int32 Collector;
				
				
				};
				
				
				class SpawnObject : public Base, public IDPacket<0x0E> {
				
				
					public:
					
					
						UInt32 EntityID;
						Byte Type;
						Int32 X;
						Int32 Y;
						Int32 Z;
						Byte Pitch;
						Byte Yaw;
						Variant<
							Int32,
							Tuple<Int32,Int16,Int16,Int16>
						> Data;
				
				
				};
				
				
				class ChunkData : public Base, public IDPacket<0x21> {
				
				
					public:
					
					
						Int32 X;
						Int32 Z;
						bool Continuous;
						UInt16 Primary;
						UInt16 Add;
						Vector<Byte> Data;
				
				
				};
				
				
				class BlockChange : public Base, public IDPacket<0x23> {
				
				
					public:
					
					
						Int32 X;
						Byte Y;
						Int32 Z;
						UInt32 Type;
						Byte Metadata;
				
				
				};
				
				
				class TabComplete : public Base, public IDPacket<0x3A> {
				
				
					public:
					
					
						Vector<String> Match;
				
				
				};
			
			
			}
			
			
			namespace Serverbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public PLPacket, public SBPacket {	};
				
				
				/**
				 *	\endcond
				 */
				 
				 
				class KeepAlive : public Base, public IDPacket<0x00> {
				
				
					public:
					
					
						Int32 KeepAliveID;
				
				
				};
			
			
				class ChatMessage : public Base, public IDPacket<0x01> {
				
				
					public:
					
					
						String Value;
				
				
				};
				
				
				class UseEntity : public Base, public IDPacket<0x02> {
				
				
					public:
					
					
						Int32 Target;
						Byte Mouse;
				
				
				};
				
				
				class Player : public Base, public IDPacket<0x03> {
				
				
					public:
					
					
						bool OnGround;
				
				
				};
				
				
				class PlayerPosition : public Base, public IDPacket<0x04> {
				
				
					public:
					
					
						Double X;
						Double Y;
						Double Stance;
						Double Z;
						bool OnGround;
				
				
				};
				
				
				class PlayerLook : public Base, public IDPacket<0x05> {
				
				
					public:
					
					
						Single Yaw;
						Single Pitch;
						bool OnGround;
				
				
				};
				
				
				class PlayerPositionAndLook : public Base, public IDPacket<0x06> {
				
				
					public:
					
					
						Double X;
						Double Y;
						Double Stance;
						Double Z;
						Single Yaw;
						Single Pitch;
						bool OnGround;
				
				
				};
				
				
				class TabComplete : public Base, public IDPacket<0x14> {
				
				
					public:
					
					
						String Text;
				
				
				};
			
			
			}
		
		
		}
	
	
		namespace Status {
		
		
			namespace Clientbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public STPacket, public CBPacket {	};
				
				
				/**
				 *	\endcond
				 */
				 
				 
				class Response : public Base, public IDPacket<0x00> {
				
				
					public:
					
					
						JSON::Value Value;
				
				
				};
				
				
				class Ping : public Base, public IDPacket<0x01> {
				
				
					public:
					
					
						Int64 Time;
				
				
				};
			
			
			}
			
			
			namespace Serverbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public STPacket, public SBPacket {	};
				
				
				/**
				 *	\endcond
				 */
				 
				 
				class Request : public Base, public IDPacket<0x00> {	};
				
				
				class Ping : public Base, public IDPacket<0x01> {
				
				
					public:
					
					
						Int64 Time;
				
				
				};
			
			
			}
		
		
		}
		
		
		namespace Login {
		
		
			namespace Clientbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public LIPacket, public CBPacket {	};
				
				
				/**
				 *	\endcond
				 */
				 
				 
				class Disconnect : public Base, public IDPacket<0x00> {
				
				
					public:
					
					
						JSON::Value Value;
				
				
				};
				
				
				class EncryptionResponse : public Base, public IDPacket<0x01> {
				
				
					public:
					
					
						String ServerID;
						Vector<Byte> PublicKey;
						Vector<Byte> VerifyToken;
				
				
				};
				
				
				class LoginSuccess : public Base, public IDPacket<0x02> {
				
				
					public:
					
					
						String UUID;
						String Username;
				
				
				};
			
			
			}
			
			
			namespace Serverbound {
			
			
				/**
				 *	\cond
				 */
				 
				 
				class Base : public LIPacket, public CBPacket {	};
				
				
				/**
				 *	\endcond
				 */
				 
				 
				class LoginStart : public Base, public IDPacket<0x00> {
				
				
					public:
					
					
						String Name;
				
				
				};
				
				
				class EncryptionRequest : public Base, public IDPacket<0x01> {
				
				
					public:
					
					
						Vector<Byte> Secret;
						Vector<Byte> VerifyToken;
				
				
				};
			
			
			}
		
		
		}
	
	
	}
	
	
	/**
	 *	\cond
	 */
	
	
	namespace PacketImpl {
	
	
		template <Word i, typename T>
		constexpr typename std::enable_if<
			i>=T::Count,
			Word
		>::type SizeImpl (const void *) noexcept {
		
			return 0;
		
		}
		
		
		template <Word i, typename T>
		typename std::enable_if<
			i<T::Count,
			Word
		>::type SizeImpl (const void * ptr) {
		
			typedef typename T::template Types<i> types;
			typedef typename types::Type type;
		
			return Word(
				SafeWord(
					Serializer<type>::Size(
						*reinterpret_cast<const type *>(
							reinterpret_cast<const void *>(
								reinterpret_cast<const Byte *>(ptr)+types::Offset
							)
						)
					)
				)+
				SafeWord(SizeImpl<i+1,T>(ptr))
			);
		
		}
		
		
		template <Word i, typename T>
		typename std::enable_if<
			i>=T::Count
		>::type SerializeImpl (const Vector<Byte> &, const void *) noexcept {	}
		
		
		template <Word i, typename T>
		typename std::enable_if<
			i<T::Count
		>::type SerializeImpl (Vector<Byte> & buffer, const void * ptr) {
		
			typedef typename T::template Types<i> types;
			typedef typename types::Type type;
			
			Serializer<type>::ToBytes(
				buffer,
				*reinterpret_cast<const type *>(
					reinterpret_cast<const void *>(
						reinterpret_cast<const Byte *>(ptr)+types::Offset
					)
				)
			);
			
			SerializeImpl<i+1,T>(buffer,ptr);
		
		}
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Serializes a packet to bytes.
	 *
	 *	\tparam T
	 *		The type of packet to serialize.
	 *
	 *	\param [in] packet
	 *		The packet to serialize.
	 *
	 *	\return
	 *		A buffer of bytes containing the
	 *		Minecraft protocol representation
	 *		of \em packet.
	 */
	template <typename T>
	typename std::enable_if<
		PacketImpl::PacketMap<T::State,T::Direction,T::PacketID>::IsValid,
		Vector<Byte>
	>::type Serialize (const T & packet) {
	
		using namespace PacketImpl;
	
		typedef PacketMap<T::State,T::Direction,T::PacketID> type;
		typedef Serializer<VarInt<UInt32>> serializer;
		
		//	Obtain a buffer reasonably sized
		//	for the packet-in-question
		Vector<Byte> buffer(
			Word(
				SafeWord(SizeImpl<0,type>(&packet))+
				SafeWord(serializer::Size(T::PacketID))
			)
		);
		
		//	Place the ID in the buffer
		serializer::ToBytes(buffer,T::PacketID);
		
		//	Place all elements in the buffer
		SerializeImpl<0,type>(buffer,&packet);
		
		//	Get the length of the buffer,
		//	for the length header
		UInt32 len=UInt32(
			SafeWord(
				buffer.Count()
			)
		);
		
		//	Determine how many bytes it will
		//	take to serialize this length
		Word len_len=serializer::Size(len);
		
		//	Calculate final size of the buffer
		Word final_count=Word(
			SafeWord(len_len)+
			SafeWord(buffer.Count())
		);
		
		//	Make sure there's enough
		//	space for that plus the payload
		//	in the buffer
		buffer.SetCapacity(final_count);
		
		//	Move the contents of the buffer
		//	backwards to make space for the
		//	length header at the beginning
		std::memmove(
			buffer.begin()+len_len,
			buffer.begin(),
			buffer.Count()
		);
		
		//	Serialize the length header to
		//	the beginning of the buffer
		buffer.SetCount(0);
		serializer::ToBytes(buffer,len);
		buffer.SetCount(final_count);
		
		return buffer;
	
	}
	
	
	/**
	 *	Generates a string representation of
	 *	a packet.
	 *
	 *	\param [in] packet
	 *		The packet to represent.
	 *	\param [in] state
	 *		The state \em packet requires
	 *		the protocol to be in to be valid.
	 *	\param [in] dir
	 *		The direction the protocol may
	 *		move in the protocol.
	 *
	 *	\return
	 *		A string representation of
	 *		\em packet.
	 */
	String ToString (const Packet & packet, ProtocolState state, ProtocolDirection dir);
	/**
	 *	Retrieves a string representation of
	 *	a member of the ProtocolState
	 *	enumeration.
	 *
	 *	\param [in] state
	 *		A protocol state.
	 *
	 *	\return
	 *		A string representation of \em state.
	 */
	const String & ToString (ProtocolState state);
	/**
	 *	Retrieves a string representation of
	 *	a member of the ProtocolDirection
	 *	enumeration.
	 *
	 *	\param [in] dir
	 *		A protocol direction.
	 *
	 *	\return
	 *		A string representation of \em dir.
	 */
	const String & ToString (ProtocolDirection dir);
	/**
	 *	Generates a string representation of a
	 *	packet.
	 *
	 *	\tparam T
	 *		The type of \em packet, which must
	 *		derive from the Packet class.
	 *
	 *	\param [in] packet
	 *		The packet of type \em T to
	 *		represent.
	 *
	 *	\return
	 *		A string representation of \em packet.
	 */
	template <typename T>
	typename std::enable_if<
		std::is_base_of<Packet,T>::value,
		String
	>::type ToString (const T & packet) {
	
		return ToString(packet,T::State,T::Direction);
	
	}
	
	
	/**
	 *	The Minecraft protocol version this
	 *	server is compatible with.
	 */
	extern const Word ProtocolVersion;
	/**
	 *	The major version number associated
	 *	with the version of vanilla Minecraft
	 *	this protocol implementation is
	 *	compatible with.
	 */
	extern const Word MinecraftMajorVersion;
	/**
	 *	The minor version number associated
	 *	with the version of vanilla Minecraft
	 *	this protocol implementation is
	 *	compatible with.
	 */
	extern const Word MinecraftMinorVersion;
	/**
	 *	The patch number associated with the
	 *	version of vanilla Minecraft this
	 *	protocol implementation is compatible
	 *	with.
	 */
	extern const Word MinecraftPatch;


}

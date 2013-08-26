/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <metadata.hpp>
#include <compression.hpp>
#include <functional>
#include <stdexcept>
#include <utility>
#include <type_traits>
#include <new>
#include <limits>
 
 
namespace MCPP {


	/**
	 *	\cond
	 */


	class Packet;


	class PacketFactory : public Memory::Managed {
	
	
		public:
		
		
			static PacketFactory * Make (Byte type);
		
		
			PacketFactory () noexcept;
			PacketFactory (const PacketFactory &) = delete;
			PacketFactory (PacketFactory &&) = delete;
			PacketFactory & operator = (const PacketFactory &) = delete;
			PacketFactory & operator = (PacketFactory &&) = delete;
		
		
			virtual ~PacketFactory () noexcept;
			
			
			virtual void Install (Packet & packet) const = 0;
			
			
			virtual bool FromBytes (
				Packet & packet,
				Vector<Byte> & buffer
			) const = 0;
			
			
			virtual Byte Type () const noexcept = 0;
			
			
			virtual Vector<Byte> ToBytes (const Packet & packet) const = 0;
			
			
			virtual Word Size (const Packet & packet) const = 0;
	
	
	};
	 
	 
	template <Byte, typename...>
	class PacketType;
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	Represents a single packet in the Minecraft
	 *	protocol.
	 */
	class Packet {
	
	
		template <Byte, typename...>
		friend class PacketType;
	
	
		private:
		

			//	Buffer of bytes where objects
			//	reside
			Byte * buffer;
			//	Information about each object
			//	in the buffer
			//
			//	0 - The offset (in bytes) of that
			//	object within the buffer.
			//
			//	1 - Whether that object is
			//	constructed or not.
			//
			//	2 - The cleanup function for that
			//	object.
			Vector<
				Tuple<
					Word,
					bool,
					std::function<void (void *)>
				>
			> metadata;
			//	The progress the packet factory has
			//	made in constructing objects in
			//	the buffer
			Word curr;
			//	Whether the packet has been built
			bool complete;
			//	The packet factory which shall be
			//	used to manipulate the data in a
			//	type-erased manner.
			const PacketFactory * factory;
			
			
			void destroy_buffer () noexcept;
			void destroy () noexcept;
			inline void bounds_check (Word offset) const {
			
				if (offset>=metadata.Count()) throw std::out_of_range("Index out of bounds");
			
			}
			template <typename T>
			inline void init (Word offset) const {
			
				//	If object is unconstructed
				if (!metadata[offset].Item<1>()) {
			
					//	Get around constness
					Packet * m_this=const_cast<Packet *>(this);
					
					//	Default initializer
					new (m_this->buffer+metadata[offset].Item<0>()) T ();
					
					//	Flag as constructed
					m_this->metadata[offset].Item<1>()=true;
					
				}
			
			}
			
			
		public:
		
		
			/**
			 *	Constructs a new, empty packet which
			 *	has not yet been bound to a type.
			 */
			Packet () noexcept;
			Packet (const Packet &) = delete;
			Packet & operator = (const Packet &) = delete;
			/**
			 *	Creates a packet by moving another packet.
			 *
			 *	\param [in] other
			 *		The packet to move.  This packet shall
			 *		be returned to the state it would be in
			 *		after being default constructed.
			 */
			Packet (Packet && other) noexcept;
			/**
			 *	Replaces this packet with the contents
			 *	of another packet.
			 *
			 *	\param [in] other
			 *		The packet to move.  This packet shall
			 *		be returned to the state it would be in
			 *		after being default constructed.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			Packet & operator = (Packet && other) noexcept;
			/**
			 *	Destroys this packet.
			 */
			~Packet () noexcept;
			
			
			/**
			 *	Retrieves an element of this packet.
			 *
			 *	If the element has not been accessed or
			 *	written, it will be returned default
			 *	constructed.
			 *
			 *	\tparam T
			 *		The type of the element to retrieve.
			 *		Setting this inappropriately leads to
			 *		undefined behaviour.
			 *
			 *	\param [in] offset
			 *		The zero-relative offset of the element
			 *		to retrieve.
			 *
			 *	\return
			 *		A reference to this requested element.
			 */
			template <typename T>
			T & Retrieve (Word offset) {
			
				bounds_check(offset);
				
				init<T>(offset);
				
				return *reinterpret_cast<T *>(buffer+metadata[offset].Item<0>());
			
			}
			
			
			/**
			 *	Retrieves an element of this packet.
			 *
			 *	If the element has not been accessed or
			 *	written, it will be returned default
			 *	constructed.
			 *
			 *	\tparam T
			 *		The type of the element to retrieve.
			 *		Setting this inappropriately leads to
			 *		undefined behaviour.
			 *
			 *	\param [in] offset
			 *		The zero-relative offset of the element
			 *		to retrieve.
			 *
			 *	\return
			 *		A reference to this requested element.
			 */
			template <typename T>
			const T & Retrieve (Word offset) const {
			
				bounds_check(offset);
				
				init<T>(offset);
				
				return *reinterpret_cast<const T *>(buffer+metadata[offset].Item<0>());
			
			}
			
			
			/**
			 *	Retrieves an element of this packet.
			 *
			 *	If the element has not been accessed or
			 *	written, it will be returned default
			 *	constructed.
			 *
			 *	\tparam T
			 *		The type that this packet has been
			 *		imbued with.
			 *	\tparam offset
			 *		The zero-relative offset of the item
			 *		to retrieve.
			 *
			 *	\return
			 *		A reference to the requested element.
			 */
			template <typename T, Word offset>
			typename T::template RetrieveType<offset>::Type & Retrieve () {
				
				init<typename T::template RetrieveType<offset>::Type>(offset);
				
				return *reinterpret_cast<typename T::template RetrieveType<offset>::Type *>(
					buffer+metadata[offset].Item<0>()
				);
			
			}
			
			
			/**
			 *	Retrieves an element of this packet.
			 *
			 *	If the element has not been accessed or
			 *	written, it will be returned default
			 *	constructed.
			 *
			 *	\tparam T
			 *		The type that this packet has been
			 *		imbued with.
			 *	\tparam offset
			 *		The zero-relative offset of the item
			 *		to retrieve.
			 *
			 *	\return
			 *		A reference to the requested element.
			 */
			template <typename T, Word offset>
			const typename T::template RetrieveType<offset>::Type & Retrieve () const {
				
				init<typename T::template RetrieveType<offset>::Type>(offset);
				
				return *reinterpret_cast<typename T::template RetrieveType<offset>::Type *>(
					buffer+metadata[offset].Item<0>()
				);
			
			}
			
			
			/**
			 *	Imbues this packet with a type.
			 *
			 *	\tparam T
			 *		The type to imbue this packet with.
			 */
			template <typename T>
			typename std::enable_if<
				std::is_base_of<PacketFactory,T>::value
			>::type SetType () {
			
				PacketFactory * factory=new T();
				
				factory->Install(*this);
			
			}
			
			
			/**
			 *	Converts raw bytes from the network into
			 *	a packet.
			 *
			 *	Once this function returns \em true, all
			 *	further calls to this function will
			 *	clear the packet and begin building another
			 *	packet inside it from the new buffer.
			 *
			 *	A return value of \em false does not mean that
			 *	\em buffer is unaffected.  This function
			 *	builds the packet incrementally.  All consumed
			 *	bytes will be removed as they are parsed.
			 *
			 *	\param [in] buffer
			 *		A buffer of bytes.  Bytes shall be
			 *		removed from this buffer appropriately
			 *		as the packet is incrementally built.
			 *
			 *	\return
			 *		\em false if the packet could not be
			 *		built completely, \em true otherwise.
			 */
			bool FromBytes (Vector<Byte> & buffer);
			
			
			/**
			 *	Retrieves the type of this packet.
			 *
			 *	Raises an error if the packet has not been
			 *	given a type.
			 *
			 *	Corresponds to the byte identifying each
			 *	packet sent on the network as part of the
			 *	Minecraft protocol.
			 *
			 *	\return
			 *		The type of this packet.
			 */
			Byte Type () const;
			
			
			/**
			 *	Retrieves the number of bytes that would
			 *	be required to transmit this packet.
			 *
			 *	Raises an error if any element in this
			 *	packet is not in a state fit for network
			 *	transmission.
			 *
			 *	\return
			 *		The number of bytes in the serialization
			 *		of this packet.
			 */
			Word Size () const;
			
			
			/**
			 *	Serializes this packet for transmission
			 *	over the network.
			 *
			 *	\return
			 *		A buffer of bytes representing this
			 *		packet.
			 */
			Vector<Byte> ToBytes () const;
			
			
			/**
			 *	Obtains a string representation of this
			 *	packet.
			 *
			 *	\return
			 *		A string representation of this packet.
			 */
			String ToString () const;
			
			
			/**
			 *	Obtains a string representation of this
			 *	packet.
			 *
			 *	\return
			 *		A string representation of this packet.
			 */
			explicit operator String () const;
	
	
	};
	
	
	/**
	 *	A thin wrapper for a Vector of elements of
	 *	type \em TItem.
	 *
	 *	A valid specialization of PacketHelper must
	 *	exist for \em TCount and \em TItem for this
	 *	to be successfully serialized and unserialized.
	 *
	 *	The Minecraft protocol requires arrays to be
	 *	sent and received, however in typical Notch
	 *	fashion these arrays are not of uniform size,
	 *	nor are they prepended with a standardized
	 *	length format.  Some use signed bytes
	 *	as the length type, some use signed 16-bit
	 *	integers.  Therefore a wrapper pairing a
	 *	Vector with a type to render its count as
	 *	is required to fully genericize the protocol.
	 *
	 *	\tparam TCount
	 *		The type to serialize the Vector's count
	 *		as.  Must be a recognized integer type.
	 *	\tparam TItem
	 *		The type of the Vector to serialize.
	 */
	template <typename TCount, typename TItem>
	class PacketArray {
	
	
		static_assert(
			std::numeric_limits<TCount>::is_integer,
			"Count type must be integer"
		);
	
	
		public:
		
		
			Vector<TItem> Payload;
		
		
			PacketArray () = default;
			PacketArray (Vector<TItem> vec) noexcept : Payload(std::move(vec)) {	}
	
	
	};
	
	
	/**
	 *	An helper class which provides a default
	 *	serialization/unserialization mechanism for
	 *	types.
	 *
	 *	This default mechanism is absolutely unsafe for
	 *	non-trivial types, and if non-trivial types
	 *	must be serialized appropriate specializations
	 *	must be provided.
	 *
	 *	By default specializations exist such that all
	 *	types used in the default Minecraft protocol
	 *	may be serialized and unserialized.
	 *
	 *	\tparam T
	 *		The type the helper should serialize and
	 *		unserialize.
	 */
	template <typename T>
	class PacketHelper {
	
	
		public:
		
		
			/**
			 *	Whether this type is trivial, i.e. whether
			 *	it can be simply copied/endian-corrected
			 *	to serialize/unserialize, and whether its
			 *	size on the wire is identical to its
			 *	size in memory.
			 */
			static const bool Trivial=true;
			
			
			/**
			 *	Obtains the size of an element.
			 *
			 *	The default implementation does not throw, but
			 *	specializations may.
			 *
			 *	The default implementation is \em constexpr, but
			 *	specializations are not necessarily.
			 *
			 *	\param [in] obj
			 *		The object to determine the size of,
			 *		ignored in the default implementation.
			 *
			 *	\return
			 *		The number of bytes required to serialize
			 *		this object.
			 */
			constexpr static Word Size (const T & obj) noexcept {
			
				return sizeof(T);
			
			}
			
			
			/**
			 *	Adds the serialized representation of an object
			 *	to a buffer for transmission over the network.
			 *
			 *	\param [in] obj
			 *		The object to serialize.
			 *	\param [in] buffer
			 *		The buffer in which to place the serialization
			 *		of \em obj.
			 */
			static void ToBytes (const T & obj, Vector<Byte> & buffer) {
			
				//	If big endian, we can copy without
				//	an intermediary copy
				if (Endianness::IsBigEndian<T>()) {
				
					const Byte * obj_ptr=reinterpret_cast<const Byte *>(&obj);
				
					buffer.Add(
						obj_ptr,
						obj_ptr+sizeof(T)
					);
				
				//	Otherwise we have to swap the byte
				//	order
				} else {
				
					union {
						T obj_copy;
						Byte obj_bytes [sizeof(T)];
					};
					
					memcpy(
						&obj_copy,
						&obj,
						sizeof(T)
					);
					
					Endianness::FixEndianness(&obj_copy);
					
					buffer.Add(
						&obj_bytes[0],
						&obj_bytes[0]+sizeof(T)
					);
				
				}
			
			}
			
			
			/**
			 *	Attempts to unserialize an object from a buffer of
			 *	bytes.
			 *
			 *	The default implementation does not throw, but
			 *	specialized implementations may.
			 *
			 *	\param [in,out] begin
			 *		A pointer to a pointer to the current location
			 *		in the buffer.  Must be updated to point
			 *		after all bytes consumed by this unserialization.
			 *	\param [in] end
			 *		The end of the buffer.
			 *	\param [in] ptr
			 *		A pointer to an area of memory large enough
			 *		and of sufficient alignment to construct an
			 *		object of type \em T.  No object shall be
			 *		constructed in this memory, and therefore
			 *		placement new construction must be used
			 *		to safely place objects in this memory.
			 *
			 *	\return
			 *		\em true if one object of type \em T was
			 *		unserialized and placed in the memory pointed
			 *		to by \em ptr, \em false otherwise.
			 */
			static bool FromBytes (const Byte ** begin, const Byte * end, T * ptr) noexcept {
			
				//	Verify there's enough space
				if ((end-(*begin))<sizeof(T)) return false;
				
				//	Copy from source buffer
				//	to destination
				memcpy(
					ptr,
					*begin,
					sizeof(T)
				);
				
				//	Advance iterator
				*begin+=sizeof(T);
				
				//	Swap byte order if necessary
				if (!Endianness::IsBigEndian<T>()) Endianness::FixEndianness(ptr);
				
				return true;
			
			}
	
	
	};
	
	
	/**
	 *	\cond
	 */
	
	
	template <typename T, std::size_t len>
	class PacketHelper<T [len]> {
	
	
		public:
		
		
			static const bool Trivial=PacketHelper<T>::Trivial;
			
			
			static Word Size (const T * obj) noexcept(Trivial) {
			
				//	If we're trivial it means that
				//	we contain objects that always
				//	reduce to the same number of bytes
				if (Trivial) return sizeof(T [len]);
				
				//	Otherwise we have to iterate
				//	and obtain the size of each element
				
				SafeWord returnthis;
				
				for (Word i=0;i<len;++i) {
				
					returnthis+=SafeWord(PacketHelper<T>::Size(obj[i]));
				
				}
				
				return Word(returnthis);
			
			}
			
			
			static void ToBytes (const T * obj, Vector<Byte> & buffer) {
			
				//	Iterate for each element and
				//	convert it into bytes
				for (Word i=0;i<len;++i) PacketHelper<T>::ToBytes(obj[i],buffer);
			
			}
			
			
			static bool FromBytes (const Byte ** begin, const Byte * end, T (*ptr) [len]) noexcept(noexcept(PacketHelper<T>::FromBytes)) {
			
				//	Iterate for each element and
				//	have it form itself in place
				//	from the bytes in the buffer
				for (Word i=0;i<len;++i) {
				
					if (!PacketHelper<T>::FromBytes(begin,end,&(*ptr)[i])) return false;
				
				}
				
				return true;
			
			}
	
	
	};
	
	
	template <>
	class PacketHelper<String> {
	
	
		public:
		
		
			static const bool Trivial=false;
			
			
			static Word Size (const String & obj) {
			
				//	Unimplemented
				return 0;
			
			}
			
			
			static void ToBytes (const String & obj, Vector<Byte> & buffer) {
			
				//	Obtain the encoding
				Vector<Byte> encoded(UTF16(true).Encode(obj));
				
				//	Output length
				PacketHelper<Int16>::ToBytes(
					Int16(SafeWord(encoded.Count()/sizeof(UTF16CodeUnit))),
					buffer
				);
				
				//	Copy encoding into buffer
				for (Byte b : encoded) buffer.Add(b);
			
			}
			
			
			static bool FromBytes (const Byte ** begin, const Byte * end, String * ptr) {
			
				//	Attempt to retrieve the length of
				//	the string
				Int16 len;
				if (!PacketHelper<Int16>::FromBytes(begin,end,&len)) return false;
				
				//	Sanity check on the length
				//	made necessary but the utter
				//	stupidity of Java in disallowing
				//	unsigned types
				if (len<0) throw std::runtime_error("Protocol error");
				
				//	Check to see if the buffer is long
				//	enough to contain a string of that
				//	length
				Word byte_len=Word(
					SafeWord(
						Word(
							SafeInt<Int16>(len)
						)
					)*
					SafeWord(
						sizeof(UTF16CodeUnit)
					)
				);
				
				auto buffer_len=end-(*begin);
				
				if (
					Word(SafeInt<decltype(buffer_len)>(buffer_len))<
					byte_len
				) return false;
				
				//	Decode
				new (ptr) String (UTF16().Decode(*begin,*begin+byte_len));
				
				//	Advance pointer
				*begin+=byte_len;
				
				return true;
			
			}
	
	
	};
	
	
	template <typename TCount, typename TItem>
	class PacketHelper<PacketArray<TCount,TItem>> {
	
		
		public:
		
		
			static const bool Trivial=false;
			
			
			static Word Size (const PacketArray<TCount,TItem> & obj) {
			
				//	Size of the leading count
				SafeWord size(sizeof(TCount));
				
				//	If the contained type is trivial,
				//	we can simply take the count of items
				//	and multiply
				if (PacketHelper<TItem>::Trivial) {
				
					size+=SafeWord(sizeof(TItem))*SafeWord(obj.Payload.Count());
				
				//	Otherwise we have to iterate and
				//	take the size of each.
				} else {
				
					for (const TItem & item : obj.Payload) {
					
						size+=SafeWord(PacketHelper<TItem>::Size(item));
					
					}
				
				}
				
				//	Return total size
				return Word(size);
			
			}
			
			
			static void ToBytes (const PacketArray<TCount,TItem> & obj, Vector<Byte> & buffer) {
			
				//	Insert size of leading count
				PacketHelper<TCount>::ToBytes(
					TCount(SafeWord(obj.Payload.Count())),
					buffer
				);
				
				//	Insert each element
				for (const TItem & item : obj.Payload) {
				
					PacketHelper<TItem>::ToBytes(
						item,
						buffer
					);
				
				}
			
			}
			
			
			static bool FromBytes (const Byte ** begin, const Byte * end, PacketArray<TCount,TItem> * ptr) {
			
				//	Read in the size
				TCount size;
				if (!PacketHelper<TCount>::FromBytes(begin,end,&size)) return false;
				
				//	Safely convert the size
				//	to a word
				Word count=Word(SafeInt<TCount>(size));
				
				//	Create buffer to hold
				//	unserialized items
				Vector<TItem> buffer(count);
				
				//	Unserialize each item
				for (Word i=0;i<count;++i) {
				
					//	Vector's underlying storage
					//	is necessarily at least
					//	as large as the number of
					//	elements thanks to careful
					//	constructor call above,
					//	therefore we can use the buffer
					//	features of the RLeahyLib vector
					//	to instantiate objects in place
					//	which eliminates copying and
					//	moving, allowing even uncopyable
					//	and unmovable objects to be
					//	unserialized.
					
					if (!PacketHelper<TItem>::FromBytes(
						begin,
						end,
						static_cast<TItem *>(buffer)+i
					)) return false;
					
					//	Expand Vector's count so that
					//	it takes ownership of the newly-
					//	constructed item.
					buffer.SetCount(i+1);
				
				}
				
				//	PacketArray<TCount,TItem> has an identical
				//	memory layout to Vector<TItem> given that
				//	it's a very thin wrapper around Vector<TItem>.
				//
				//	Therefore treat ptr as a ptr to a Vector<TItem>
				//	and move
				new (ptr) Vector<TItem> (std::move(buffer));
				
				//	Success!
				return true;
			
			}
		
	
	};
	
	
	template <>
	class PacketHelper<Coordinates> {
	
	
		public:
		
		
			static const bool Trivial=true;
			
			
			constexpr static Word Size (const Coordinates & obj) noexcept {
			
				return sizeof(Coordinates);
			
			}
			
			
			static void ToBytes (const Coordinates & obj, Vector<Byte> & buffer) {
			
				PacketHelper<Int32>::ToBytes(obj.X,buffer);
				PacketHelper<Int32>::ToBytes(obj.Y,buffer);
				PacketHelper<Int32>::ToBytes(obj.Z,buffer);
			
			}
			
			
			static bool FromBytes (const Byte ** begin, const Byte * end, Coordinates * ptr) noexcept {
			
				return (
					PacketHelper<Int32>::FromBytes(begin,end,&(ptr->X)) &&
					PacketHelper<Int32>::FromBytes(begin,end,&(ptr->Y)) &&
					PacketHelper<Int32>::FromBytes(begin,end,&(ptr->Z))
				);
			
			}
	
	
	};
	
	
	template <>
	class PacketHelper<Vector<Metadatum>> {
	
	
		public:
		
		
			static const bool Trivial=false;
			
			
			static Word Size (const Vector<Metadatum> & obj) {
			
				//	Always at least one for
				//	the trailing 127
				SafeWord size(1);
				
				for (const Metadatum & metadata : obj) {
				
					//	Always at least one for
					//	the leading pair of
					//	key and type packed into
					//	a single byte
					size+=SafeWord(1);
				
					switch (metadata.Type()) {
					
						case MetadatumType::Byte:
							size+=SafeWord(sizeof(Byte));
							break;
						case MetadatumType::Short:
							size+=SafeWord(sizeof(Int16));
							break;
						case MetadatumType::Int:
							size+=SafeWord(sizeof(Int32));
							break;
						case MetadatumType::Float:
							size+=SafeWord(sizeof(Single));
							break;
						case MetadatumType::String:
							size+=SafeWord(PacketHelper<String>::Size(metadata.Value<String>()));
							break;
						//	TODO: Add slot
						default:
							size+=SafeWord(PacketHelper<Coordinates>::Size(metadata.Value<Coordinates>()));
							break;
					
					}
				
				}
				
				//	Return
				return Word(size);
			
			}
			
			
			static void ToBytes (const Vector<Metadatum> & obj, Vector<Byte> & buffer) {
			
				//	Loop for each piece of metadata
				for (const Metadatum & metadata : obj) {
				
					//	Pack type and key
					buffer.Add(metadata.Key()|(static_cast<Byte>(metadata.Type())<<5));
					
					//	Serialize payload
					switch (metadata.Type()) {
					
						case MetadatumType::Byte:
							PacketHelper<SByte>::ToBytes(
								metadata.Value<SByte>(),
								buffer
							);
							break;
						case MetadatumType::Short:
							PacketHelper<Int16>::ToBytes(
								metadata.Value<Int16>(),
								buffer
							);
							break;
						case MetadatumType::Int:
							PacketHelper<Int32>::ToBytes(
								metadata.Value<Int32>(),
								buffer
							);
							break;
						case MetadatumType::Float:
							PacketHelper<Single>::ToBytes(
								metadata.Value<Single>(),
								buffer
							);
							break;
						case MetadatumType::String:
							PacketHelper<String>::ToBytes(
								metadata.Value<String>(),
								buffer
							);
							break;
						//	TODO: Add slot
						default:
							PacketHelper<Coordinates>::ToBytes(
								metadata.Value<Coordinates>(),
								buffer
							);
							break;
							
					}
				
				}
				
				//	Add the final trailing byte
				buffer.Add(127);
			
			}
			
			
			static bool FromBytes (const Byte ** begin, const Byte * end, Vector<Metadatum> * ptr) {
			
				Vector<Metadatum> metadata;
			
				while (!(
					//	We need to be able to read at least
					//	one byte
					(*begin==end) ||
					//	127 signals end of metadata
					(**begin==127)
				)) {
					
					//	Unpack the leading byte
					Byte key=**begin&31;
					Byte type=**begin>>5;
					
					++(*begin);
					
					//	Verify the type
					if (type>6) throw std::runtime_error("Protocol error");
					
					//	Extract what follows based on type
					switch (static_cast<MetadatumType>(type)) {
					
						case MetadatumType::Byte:{
						
							SByte sbyte;
							if (!PacketHelper<SByte>::FromBytes(begin,end,&sbyte)) return false;
							
							metadata.EmplaceBack(
								key,
								sbyte
							);
						
						}break;
						
						case MetadatumType::Short:{
						
							Int16 int16;
							if (!PacketHelper<Int16>::FromBytes(begin,end,&int16)) return false;
							
							metadata.EmplaceBack(
								key,
								int16
							);
						
						}break;
						
						case MetadatumType::Int:{
						
							Int32 int32;
							if (!PacketHelper<Int32>::FromBytes(begin,end,&int32)) return false;
							
							metadata.EmplaceBack(
								key,
								int32
							);
						
						}break;
						
						case MetadatumType::Float:{
						
							Single single;
							if (!PacketHelper<Single>::FromBytes(begin,end,&single)) return false;
							
							metadata.EmplaceBack(
								key,
								single
							);
						
						}break;
						
						case MetadatumType::String:{
						
						}break;
						
						//	TODO: Add slot
						
						default:{
						
							Coordinates coords;
							if (!PacketHelper<Coordinates>::FromBytes(begin,end,&coords)) return false;
							
							metadata.EmplaceBack(
								key,
								coords
							);
						
						}break;
					
					}
				
				}
				
				//	Why'd we exit the loop?
				
				//	Not enough bytes?
				if (*begin==end) return false;
				
				//	Must've found the 127
				++(*begin);
				
				//	Move the metadata into place
				new (ptr) Vector<Metadatum> (std::move(metadata));
				
				return true;
			
			}
	
	
	};
	
	
	template <typename... Args>
	class PacketHelper<Tuple<Args...>> {
	
	
		private:
		
		
			template <Word i>
			constexpr static inline typename std::enable_if<
				i>=sizeof...(Args),
				Word
			>::type size (const Tuple<Args...> &) {
			
				return 0;
			
			}
			
			
			template <Word i>
			static inline typename std::enable_if<
				i<sizeof...(Args),
				Word
			>::type size (const Tuple<Args...> & obj) {
			
				SafeWord recurse(
					size<i+1>(obj)
				);
				
				SafeWord curr(
					PacketHelper<
						typename std::decay<
							decltype(
								obj.template Item<i>()
							)
						>::type
					>::Size(
						obj.template Item<i>()
					)
				);
				
				return Word(recurse+curr);
			
			}
			
			
			template <Word i>
			static inline typename std::enable_if<
				i>=sizeof...(Args)
			>::type to_bytes (const Tuple<Args...> &, Vector<Byte> &) noexcept {	}
			
			
			template <Word i>
			static inline typename std::enable_if<
				i<sizeof...(Args)
			>::type to_bytes (const Tuple<Args...> & obj, Vector<Byte> & buffer) {
			
				PacketHelper<
					typename std::decay<
						decltype(
							obj.template Item<i>()
						)
					>::type
				>::ToBytes(
					obj.template Item<i>(),
					buffer
				);
				
				to_bytes<i+1>(obj,buffer);
			
			}
			
			
			template <Word i>
			constexpr static inline typename std::enable_if<
				i>=sizeof...(Args),
				bool
			>::type from_bytes (const Byte **, const Byte *, Tuple<Args...> *) noexcept {
			
				return true;
			
			}
			
			
			template <Word i>
			static inline typename std::enable_if<
				i<sizeof...(Args),
				bool
			>::type from_bytes (const Byte ** begin, const Byte * end, Tuple<Args...> * ptr) {
			
				if (!PacketHelper<
					typename std::decay<
						decltype(
							ptr->template Item<i>()
						)
					>::type
				>::FromBytes(
					begin,
					end,
					&(ptr->template Item<i>())
				)) return false;
				
				return from_bytes<i+1>(begin,end,ptr);
			
			}
	
	
		public:
		
		
			static const bool Trivial=false;
			
			
			static Word Size (const Tuple<Args...> & obj) {
			
				return size<0>(obj);
			
			}
			
			
			static void ToBytes (const Tuple<Args...> & obj, Vector<Byte> & buffer) {
			
				to_bytes<0>(obj,buffer);
			
			}
			
			
			static bool FromBytes (const Byte ** begin, const Byte * end, Tuple<Args...> * ptr) {
			
				return from_bytes<0>(begin,end,ptr);
			
			}
	
	
	};
	
	
	/**
	 *	\endcond
	 */
	
	
	template <Byte type, typename... Types>
	class PacketType : public PacketFactory {
	
	
		public:
		
		
			/**
			 *	The number of objects in the payload
			 *	of this packet type.
			 */
			static const Word TypesCount=sizeof...(Types);
			/**
			 *	A helper class which allows access to
			 *	arbitrary types within the payload
			 *	of this packet type.
			 *
			 *	\tparam i
			 *		The zero-relative index of the type
			 *		to retrieve.
			 */
			template <Word i>
			class RetrieveType {
			
			
				public:
				
				
					/**
					 *	Specifies the type of the \em ith
					 *	item in the payload of the containing
					 *	packet type.
					 */
					typedef typename GetParameterType<i,Types...>::Type Type;
			
			
			};
	
	
		private:
	
	
			template <Word i>
			typename std::enable_if<
				i<GetParameterCount<Types...>::Value,
				Word
			>::type memory (Word curr, Packet & packet) const {
			
				typedef typename GetParameterType<i,Types...>::Type curr_type;
				
				//	Cleanup function
				std::function<void (void *)> cleanup;
				
				//	If this type isn't trivial, we'll have
				//	to have it cleaned up
				if (!std::is_trivial<curr_type>::value) {
				
					cleanup=[] (void * ptr) {	reinterpret_cast<curr_type *>(ptr)->~curr_type();	};
				
				}
			
				//	Check to see if placing this
				//	object adjacent to the last one
				//	would result in alignment
				//	issues
				if ((curr%alignof(curr_type))!=0) {
				
					//	Adjust size to ensure this
					//	object will be properly aligned
					
					curr=Word(
						SafeWord(alignof(curr_type))*(
							SafeWord(curr/alignof(curr_type))+
							SafeWord(1)
						)
					);
				
				}
				
				//	Create metadata
				packet.metadata.EmplaceBack(
					curr,
					false,	//	Not yet constructed
					std::move(cleanup)
				);
				
				//	Recurse and return
				return memory<i+1>(
					Word(
						SafeWord(curr)+
						SafeWord(sizeof(curr_type))
					),
					packet
				);
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i>=GetParameterCount<Types...>::Value,
				Word
			>::type memory (Word curr, Packet & packet) const noexcept {
			
				return curr;
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i<GetParameterCount<Types...>::Value,
				bool
			>::type from_bytes (const Byte ** begin, const Byte * end, Packet & packet) const {
			
				//	Skip if this object has already
				//	been constructed
				if (packet.curr<=i) {
				
					//	Save begin pointer in case this
					//	fails
					const Byte * before=*begin;
				
					if (
						//	Was this object previously
						//	constructed?
						packet.metadata[i].Item<1>() &&
						//	Does it have a cleanup function?
						packet.metadata[i].Item<2>()
					) {
					
						//	Destroy it
						packet.metadata[i].Item<2>()(
							packet.buffer+packet.metadata[i].Item<0>()
						);
					
					}
					
					typedef typename GetParameterType<i,Types...>::Type curr_type;
					
					//	Attempt to get the object from
					//	the buffer
					if (!PacketHelper<curr_type>::FromBytes(
						begin,
						end,
						reinterpret_cast<curr_type *>(
							packet.buffer+packet.metadata[i].Item<0>()
						)
					)) {
					
						//	Restore begin
						*begin=before;
						
						//	Fail
						return false;
						
					}
					
					//	Object constructed
					packet.metadata[i].Item<1>()=true;
					
					//	Success, update count of items
					//	retrieved
					packet.curr=i+1;
				
				}
				
				//	Recurse
				return from_bytes<i+1>(begin,end,packet);
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i>=GetParameterCount<Types...>::Value,
				bool
			>::type from_bytes (const Byte **, const Byte *, Packet & packet) const noexcept {
			
				//	Done
				packet.curr=GetParameterCount<Types...>::Value;
				packet.complete=true;
			
				return true;
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i<GetParameterCount<Types...>::Value,
				Word
			>::type size (Word curr, const Packet & packet) const {
			
				typedef typename GetParameterType<i,Types...>::Type curr_type;
				
				//	If the object is not trivial,
				//	and is uninitialized, that's
				//	an error
				if (!(
					PacketHelper<curr_type>::Trivial ||
					packet.metadata[i].Item<1>()
				)) throw std::runtime_error("Non-trivial object uninitialized");
				
				//	Get the size, recurse, and return
				return Word(
					SafeWord(PacketHelper<curr_type>::Size(
						*reinterpret_cast<curr_type *>(
							packet.buffer+packet.metadata[i].Item<0>()
						)
					))+
					SafeWord(size<i+1>(curr,packet))
				);
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i>=GetParameterCount<Types...>::Value,
				Word
			>::type size (Word curr, const Packet &) const noexcept {
			
				return curr;
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i<GetParameterCount<Types...>::Value
			>::type to_bytes (Vector<Byte> & buffer, const Packet & packet) const {
			
				typedef typename GetParameterType<i,Types...>::Type curr_type;
			
				//	Load buffer
				PacketHelper<curr_type>::ToBytes(
					*reinterpret_cast<curr_type *>(
						packet.buffer+packet.metadata[i].Item<0>()
					),
					buffer
				);
				
				//	Recurse
				to_bytes<i+1>(buffer,packet);
			
			}
			
			
			template <Word i>
			constexpr typename std::enable_if<
				i>=GetParameterCount<Types...>::Value
			>::type to_bytes (Vector<Byte> &, const Packet &) const noexcept {	}
		
		
		public:
		
		
			virtual void Install (Packet & packet) const override {
			
				//	First clean up the packet
				packet.destroy();
				packet.metadata.Delete(0,packet.metadata.Count());
				packet.factory=this;
			
				//	We haven't started anything yet
				packet.curr=0;
				packet.complete=false;
				
				try {
				
					//	Allocate memory and get metadata
					packet.buffer=Memory::Allocate<Byte>(
						memory<0>(0,packet)
					);
					
				} catch (...) {
				
					//	Put packet back in consistent state
					delete this;
					packet.factory=nullptr;
					
					throw;
				
				}
			
			}

			
			virtual bool FromBytes (Packet & packet, Vector<Byte> & buffer) const override {
			
				//	Get iterator
				const Byte * begin=buffer.begin();
				
				//	Call helper function
				bool success=from_bytes<0>(&begin,buffer.end(),packet);
				
				//	Delete consumed bytes
				buffer.Delete(
					0,
					begin-buffer.begin()
				);
				
				//	Return
				return success;
			
			}
			
			
			virtual Byte Type () const noexcept override {
			
				return type;
			
			}
			
			
			virtual Word Size (const Packet & packet) const override {
			
				return Word(SafeWord(1)+SafeWord(size<0>(0,packet)));
			
			}
			
			
			virtual Vector<Byte> ToBytes (const Packet & packet) const {
			
				//	Create buffer large enough
				//
				//	This has the side effect of checking
				//	for incomplete non-trivial objects
				Vector<Byte> buffer(Size(packet));
				
				//	Add initial type byte
				buffer.Add(type);
				
				//	Recurse and load buffer
				to_bytes<0>(buffer,packet);
				
				//	Return loaded buffer
				return buffer;
			
			}
	
	
	};
	
	
	typedef Int32 PLACEHOLDER;
	
	
	template <Byte i>
	class PacketTypeMap {	};
	
	
	/**
	 *	\cond
	 */
	
	
	template <> class PacketTypeMap<0x00> : public PacketType<0x00,Int32> {	};
	template <> class PacketTypeMap<0x01> : public PacketType<0x01,Int32,String,SByte,SByte,SByte,SByte,SByte> {	};
	template <> class PacketTypeMap<0x02> : public PacketType<0x02,SByte,String,String,Int32> {	};
	template <> class PacketTypeMap<0x03> : public PacketType<0x03,String> {	};
	template <> class PacketTypeMap<0x04> : public PacketType<0x04,Int64,Int64> {	};
	template <> class PacketTypeMap<0x05> : public PacketType<0x05,Int32,Int16,PLACEHOLDER> {	};
	template <> class PacketTypeMap<0x06> : public PacketType<0x06,Int32,Int32,Int32> { };
	template <> class PacketTypeMap<0x07> : public PacketType<0x07,Int32,Int32,bool> {	};
	template <> class PacketTypeMap<0x08> : public PacketType<0x08,Int16,Int16,Single> { };
	template <> class PacketTypeMap<0x09> : public PacketType<0x09,Int32,SByte,SByte,Int16,String> { };
	template <> class PacketTypeMap<0x0A> : public PacketType<0x0A,bool> { };
	template <> class PacketTypeMap<0x0B> : public PacketType<0x0B,Double,Double,Double,Double,bool> { };
	template <> class PacketTypeMap<0x0C> : public PacketType<0x0C,Single,Single,bool> { };
	template <> class PacketTypeMap<0x0D> : public PacketType<0x0D,Double,Double,Double,Double,Single,Single,bool> { };
	template <> class PacketTypeMap<0x0E> : public PacketType<0x0E,SByte,Int32,SByte,Int32,SByte> { };
	template <> class PacketTypeMap<0x0F> : public PacketType<0x0F,Int32,Byte,Int32,SByte,PLACEHOLDER,SByte,SByte,SByte> { };
	template <> class PacketTypeMap<0x10> : public PacketType<0x10,Int16> { };
	template <> class PacketTypeMap<0x11> : public PacketType<0x11,Int32,SByte,Int32,SByte,Int32> { };
	template <> class PacketTypeMap<0x12> : public PacketType<0x12,Int32,SByte> { };
	template <> class PacketTypeMap<0x13> : public PacketTypeMap<0x12> { };
	template <> class PacketTypeMap<0x14> : public PacketType<0x14,Int32,String,Int32,Int32,Int32,SByte,SByte,Int16,Vector<Metadatum>> { };
	//	0x15
	template <> class PacketTypeMap<0x16> : public PacketType<0x16,Int32,Int32> { };
	template <> class PacketTypeMap<0x17> : public PacketType<0x17,Int32,SByte,Int32,Int32,Int32,SByte,SByte,PLACEHOLDER> { };
	template <> class PacketTypeMap<0x18> : public PacketType<0x18,Int32,SByte,Int32,Int32,Int32,SByte,SByte,SByte,Int16,Int16,Int16,Vector<Metadatum>> { };
	template <> class PacketTypeMap<0x19> : public PacketType<0x19,Int32,String,Int32,Int32,Int32,Int32> { };
	template <> class PacketTypeMap<0x1A> : public PacketType<0x1A,Int32,Int32,Int32,Int32,Int16> { };
	//	0x1B
	template <> class PacketTypeMap<0x1C> : public PacketType<0x1C,Int32,Int16,Int16,Int16> { };
	template <> class PacketTypeMap<0x1D> : public PacketType<0x1D,PacketArray<SByte,Int32>> { };
	template <> class PacketTypeMap<0x1E> : public PacketType<0x1E,Int32> { };
	template <> class PacketTypeMap<0x1F> : public PacketType<0x1F,Int32,SByte,SByte,SByte> { };
	template <> class PacketTypeMap<0x20> : public PacketType<0x20,Int32,SByte,SByte> { };
	template <> class PacketTypeMap<0x21> : public PacketType<0x21,Int32,SByte,SByte,SByte,SByte,SByte> { };
	template <> class PacketTypeMap<0x22> : public PacketType<0x22,Int32,Int32,Int32,Int32,SByte,SByte> { };
	template <> class PacketTypeMap<0x23> : public PacketType<0x23,Int32,SByte> { };
	//	0x24
	//	0x25
	template <> class PacketTypeMap<0x26> : public PacketType<0x26,Int32,SByte> { };
	template <> class PacketTypeMap<0x27> : public PacketType<0x27,Int32,Int32> { };
	template <> class PacketTypeMap<0x28> : public PacketType<0x28,Int32,Vector<Metadatum>> { };
	template <> class PacketTypeMap<0x29> : public PacketType<0x29,Int32,SByte,SByte,Int16> { };
	template <> class PacketTypeMap<0x2A> : public PacketType<0x2A,Int32,SByte> { };
	template <> class PacketTypeMap<0x2B> : public PacketType<0x2B,Single,Int16,Int16> { };
	template <> class PacketTypeMap<0x2C> : public PacketType<0x2C,PacketArray<Int32,Tuple<String,Double,PacketArray<Int16,Tuple<UInt128,Double,SByte>>>>> {	};
	//	0x2D
	//	0x2F
	//	0x30
	//	0x31
	//	0x32
	//	0x33
	//	0x34
	template <> class PacketTypeMap<0x35> : public PacketType<0x35,Int32,Byte,Int32,Int16,Byte> {	};
	template <> class PacketTypeMap<0x36> : public PacketType<0x36,Int32,Int16,Int32,Byte,Byte,Int16> {	};
	template <> class PacketTypeMap<0x37> : public PacketType<0x37,Int32,Int32,Int32,Int32,Byte> {	};
	//	0x38
	//	0x39
	//	0x3A
	//	0x3B
	template <> class PacketTypeMap<0x3C> : public PacketType<0x3C,Double,Double,Double,Single,PacketArray<Int32,SByte [3]>,Single,Single,Single> {	};
	template <> class PacketTypeMap<0x3D> : public PacketType<0x3D,Int32,Int32,SByte,Int32,Int32,bool> {	};
	template <> class PacketTypeMap<0x3E> : public PacketType<0x3E,String,Int32,Int32,Int32,Single,SByte> {	};
	template <> class PacketTypeMap<0x3F> : public PacketType<0x3F,String,Single,Single,Single,Single,Single,Single,Single,Int32> {	};
	//	0x40
	//	0x41
	//	0x42
	//	0x43
	//	0x44
	//	0x45
	template <> class PacketTypeMap<0x46> : public PacketType<0x46,SByte,SByte> {	};
	template <> class PacketTypeMap<0x47> : public PacketType<0x47,Int32,SByte,Int32,Int32,Int32> {	};
	//	0x48
	//	0x49
	//	0x4A
	//	0x4B
	//	0x4C
	//	0x4D
	//	0x4E
	//	0x4F
	//	0x50
	//	0x51
	//	0x52
	//	0x53
	//	0x54
	//	0x55
	//	0x56
	//	0x57
	//	0x58
	//	0x59
	//	0x5A
	//	0x5B
	//	0x5C
	//	0x5D
	//	0x5F
	//	0x60
	//	0x61
	//	0x62
	//	0x63
	template <> class PacketTypeMap<0x64> : public PacketType<0x64,SByte,SByte,String,SByte,bool> {	};
	template <> class PacketTypeMap<0x65> : public PacketType<0x65,SByte> {	};
	template <> class PacketTypeMap<0x66> : public PacketType<0x66,SByte,Int16,SByte,Int16,SByte,PLACEHOLDER> {	};
	template <> class PacketTypeMap<0x67> : public PacketType<0x67,SByte,Int16,PLACEHOLDER> {	};
	template <> class PacketTypeMap<0x68> : public PacketType<0x68,SByte,PacketArray<Int16,PLACEHOLDER>> {	};
	template <> class PacketTypeMap<0x69> : public PacketType<0x69,SByte,Int16,Int16> {	};
	template <> class PacketTypeMap<0x6A> : public PacketType<0x6A,SByte,Int16,bool> {	};
	template <> class PacketTypeMap<0x6B> : public PacketType<0x6B,Int16,PLACEHOLDER> {	};
	template <> class PacketTypeMap<0x6C> : public PacketType<0x6C,SByte,SByte> {	};
	//	0x6D
	//	0x6E
	//	0x6F
	//	0x70
	//	0x71
	//	0x72
	//	0x73
	//	0x74
	//	0x75
	//	0x76
	//	0x77
	//	0x78
	//	0x79
	//	0x7A
	//	0x7B
	//	0x7C
	//	0x7D
	//	0x7E
	//	0x7F
	//	0x80
	//	0x81
	template <> class PacketTypeMap<0x82> : public PacketType<0x82,Int32,Int16,Int32,String,String,String,String> {	};
	template <> class PacketTypeMap<0x83> : public PacketType<0x83,Int16,Int16,PacketArray<Int16,Byte>> {	};
	template <> class PacketTypeMap<0x84> : public PacketType<0x84,Int32,Int16,Int32,SByte,PLACEHOLDER> {	};
	//	...
	template <> class PacketTypeMap<0xC9> : public PacketType<0xC9,String,bool,Int16> {	};
	template <> class PacketTypeMap<0xCA> : public PacketType<0xCA,Byte,SByte,SByte> {	};
	template <> class PacketTypeMap<0xCB> : public PacketType<0xCB,String> {	};
	template <> class PacketTypeMap<0xCC> : public PacketType<0xCC,String,SByte,SByte,SByte,bool> {	};
	template <> class PacketTypeMap<0xCD> : public PacketType<0xCD,SByte> {	};
	template <> class PacketTypeMap<0xCE> : public PacketType<0xCE,String,String,SByte> {	};
	template <> class PacketTypeMap<0xCF> : public PacketType<0xCF,String,SByte,String,Int32> {	};
	template <> class PacketTypeMap<0xD0> : public PacketType<0xD0,SByte,String> {	};
	template <> class PacketTypeMap<0xD1> : public PacketType<0xD1,String,SByte,String,String,String,SByte,PacketArray<Int16,String>> {	};
	//	...
	template <> class PacketTypeMap<0xFA> : public PacketType<0xFA,String,PacketArray<Int16,Byte>> {	};
	//	0xFB
	template <> class PacketTypeMap<0xFC> : public PacketType<0xFC,PacketArray<Int16,Byte>,PacketArray<Int16,Byte>> {	};
	template <> class PacketTypeMap<0xFD> : public PacketType<0xFD,String,PacketArray<Int16,Byte>,PacketArray<Int16,Byte>> {	};
	template <> class PacketTypeMap<0xFE> : public PacketType<0xFE,SByte,Byte,String,Int16,Byte,String,Int32> {	};
	template <> class PacketTypeMap<0xFF> : public PacketType<0xFF,String> {	};
	
	
	/**
	 *	\endcond
	 */
	 
	 
	/**
	 *	Current Minecraft protocol
	 *	version.
	 */
	extern const Word ProtocolVersion;
	/**
	 *	Current Minecraft major version
	 *	number.
	 */
	extern const Word MinecraftMajorVersion;
	/**
	 *	Current Minecraft minor version
	 *	number.
	 */
	extern const Word MinecraftMinorVersion;
	/**
	 *	Current Minecraft sub-minor version
	 *	number.
	 */
	extern const Word MinecraftSubminorVersion;
	

}

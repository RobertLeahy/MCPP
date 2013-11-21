/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>


namespace MCPP {


	class SerializerError : public std::runtime_error {
	
	
		public:
		
		
			using std::runtime_error::runtime_error;
		
		
			static const char * InsufficientBytes;
			static const char * InvalidBoolean;
	
	
	};
	
	
	class ByteBufferError : public std::exception {
	
	
		private:
		
		
			const char * what_str;
			Word where;
			
			
		public:
		
		
			ByteBufferError (const char * what_str, Word where) noexcept;
			
			
			virtual const char * what () const noexcept override;
			Word Where () const noexcept;
	
	
	};


	template <typename T>
	class Serializer {
	
	
		public:
		
		
			static T FromBytes (const Byte * & begin, const Byte * end) {
			
				if ((end-begin)<sizeof(T)) throw SerializerError(SerializerError::InsufficientBytes);
				
				union {
					T retr;
					Byte buffer [sizeof(T)];
				};
				
				if (Endianness::IsBigEndian<T>()) {
				
					std::memcpy(buffer,begin,sizeof(T));
					
					begin+=sizeof(T);
				
				} else {
				
					for (Word i=0;i<sizeof(T);++i) buffer[sizeof(T)-1-i]=*(begin++);
				
				}
				
				return retr;
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const T & obj) {
			
				while ((buffer.Capacity()-buffer.Count())<sizeof(T)) buffer.SetCapacity();
				
				auto curr=buffer.end();
				
				if (Endianness::IsBigEndian<T>()) std::memcpy(curr,&obj,sizeof(T));
				else for (Word i=0;i<sizeof(T);++i) curr[i]=reinterpret_cast<const Byte *>(&obj)[sizeof(T)-1-i];
				
				buffer.SetCount(buffer.Count()+sizeof(T));
			
			}
	
	
	};
	
	
	/**
	 *	\cond
	 */
	 
	 
	template <>
	class Serializer<bool> {
	
	
		public:
		
		
			static bool FromBytes (const Byte * & begin, const Byte * end) {
			
				if (begin==end) throw SerializerError(SerializerError::InsufficientBytes);
				
				auto b=*(begin++);
				
				if (b==0) return false;
				if (b==1) return true;
				
				throw SerializerError(SerializerError::InvalidBoolean);
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, bool obj) {
			
				buffer.Add(obj ? 1 : 0);
			
			}
	
	
	};
	
	
	template <>
	class Serializer<String> {
	
	
		public:
		
		
			static String FromBytes (const Byte * & begin, const Byte * end) {
			
				auto len=Serializer<UInt32>::FromBytes(begin,end);
				
				if ((end-begin)<len) throw SerializerError(SerializerError::InsufficientBytes);
				
				auto end_str=begin+len;
				
				auto retr=UTF8().Decode(
					begin,
					end_str
				);
				
				begin=end_str;
				
				return retr;
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const String & obj) {
			
				auto encoded=UTF8().Encode(obj);
				
				Serializer<UInt32>::ToBytes(
					buffer,
					static_cast<UInt32>(SafeWord(encoded.Count()))
				);
				
				while ((buffer.Capacity()-buffer.Count())<encoded.Count()) buffer.SetCapacity();
				
				std::memcpy(
					buffer.end(),
					encoded.begin(),
					encoded.Count()
				);
				
				buffer.SetCount(buffer.Count()+encoded.Count());
			
			}
	
	
	};
	
	
	template <>
	class Serializer<IPAddress> {
	
	
		public:
		
		
			static IPAddress FromBytes (const Byte * & begin, const Byte * end) {
			
				if (Serializer<bool>::FromBytes(begin,end)) return Serializer<UInt128>::FromBytes(begin,end);
				
				return Serializer<UInt32>::FromBytes(begin,end);
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const IPAddress & ip) {
			
				auto v6=ip.IsV6();
				
				Serializer<bool>::ToBytes(buffer,v6);
				
				if (v6) Serializer<UInt128>::ToBytes(buffer,ip);
				else Serializer<UInt32>::ToBytes(buffer,ip);
			
			}
	
	
	};
	
	
	template <typename T>
	class Serializer<Vector<T>> {
	
	
		private:
		
		
			typedef typename std::decay<T>::type type;
	
	
		public:
		
		
			static Vector<T> FromBytes (const Byte * & begin, const Byte * end) {
			
				auto len=static_cast<Word>(
					SafeInt<UInt32>(
						Serializer<UInt32>::FromBytes(begin,end)
					)
				);
				
				Vector<T> retr(len);
				
				for (Word i=0;i<len;++i) retr.Add(
					Serializer<type>::FromBytes(begin,end)
				);
				
				return retr;
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const Vector<T> & obj) {
			
				Serializer<UInt32>::ToBytes(
					buffer,
					static_cast<UInt32>(SafeWord(obj.Count()))
				);
				
				for (const auto & o : obj) Serializer<type>::ToBytes(buffer,o);
			
			}
	
	
	};
	
	
	template <typename T>
	class Serializer<std::unordered_set<T>> {
	
	
		private:
		
		
			typedef typename std::decay<T>::type type;
	
	
		public:
		
		
			static std::unordered_set<T> FromBytes (const Byte * & begin, const Byte * end) {
			
				auto len=Serializer<UInt32>::FromBytes(begin,end);
				
				std::unordered_set<T> retr;
				
				for (UInt32 i=0;i<len;++i) retr.insert(
					Serializer<type>::FromBytes(begin,end)
				);
				
				return retr;
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const std::unordered_set<T> & obj) {
			
				Serializer<UInt32>::ToBytes(
					buffer,
					static_cast<UInt32>(SafeInt<std::size_t>(obj.size()))
				);
				
				for (const auto & o : obj) Serializer<type>::ToBytes(buffer,o);
			
			}
	
	
	};
	
	
	template <typename First, typename Second>
	class Serializer<std::pair<First,Second>> {
	
	
		private:
		
		
			typedef typename std::decay<First>::type first;
			typedef typename std::decay<Second>::type second;
			
			
		public:
		
		
			static std::pair<First,Second> FromBytes (const Byte * & begin, const Byte * end) {
			
				auto one=Serializer<first>::FromBytes(begin,end);
				return std::pair<First,Second>(
					std::move(one),
					Serializer<second>::FromBytes(begin,end)
				);
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const std::pair<First,Second> & obj) {
			
				Serializer<first>::ToBytes(buffer,obj.first);
				Serializer<second>::ToBytes(buffer,obj.second);
			
			}
	
	
	};
	
	
	template <typename Key, typename Value>
	class Serializer<std::unordered_map<Key,Value>> {
	
	
		private:
		
		
			typedef std::unordered_map<Key,Value> type;
			typedef typename type::value_type value_type;
	
	
		public:
		
		
			static type FromBytes (const Byte * & begin, const Byte * end) {
			
				auto len=Serializer<UInt32>::FromBytes(begin,end);
				
				type retr;
				
				for (UInt32 i=0;i<len;++i) retr.insert(
					Serializer<value_type>::FromBytes(begin,end)
				);
				
				return retr;
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const type & obj) {
			
				Serializer<UInt32>::ToBytes(
					buffer,
					static_cast<UInt32>(SafeInt<std::size_t>(obj.size()))
				);
				
				for (const auto & o : obj) Serializer<value_type>::ToBytes(buffer,o);
			
			}
	
	
	};
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	Holds, saves, parses, and serializes to
	 *	an internal buffer of bytes.
	 */
	class ByteBuffer {
	
	
		private:
		
		
			Vector<Byte> buffer;
			const Byte * loc;
			
			
			template <typename T>
			using type=typename std::decay<T>::type;
			
			
		public:
		
		
			/**
			 *	Create a ByteBuffer instance by loading
			 *	the binary data associated with a certain
			 *	key from the backing store.
			 *
			 *	\param [in] key
			 *		The key whose associated binary data
			 *		shall be loaded.
			 *
			 *	\return
			 *		A ByteBuffer containing the binary
			 *		data associated with \em key, or no
			 *		data, if nothing was associated with
			 *		\em key.
			 */
			static ByteBuffer Load (const String & key);
		
		
			/**
			 *	Creates an empty ByteBuffer.
			 */
			ByteBuffer () noexcept;
			
			
			/**
			 *	Attempts to retrieve an object of a certain
			 *	type from the current location in the buffer.
			 *
			 *	Advances the buffer past the object-in-question
			 *	once it is retrieved.
			 *
			 *	\tparam T
			 *		The type of object to retrieve.
			 *
			 *	\return
			 *		An object of type \em T parsed from the
			 *		underlying buffer of bytes.
			 */
			template <typename T>
			T FromBytes () {
			
				if (loc==nullptr) loc=buffer.begin();
				
				try {
				
					return Serializer<type<T>>::FromBytes(
						loc,
						buffer.end()
					);
					
				} catch (const SerializerError & e) {
				
					throw ByteBufferError(
						e.what(),
						loc-buffer.begin()
					);
				
				}
			
			}
			
			
			/**
			 *	Adds an object of a certain type to the end
			 *	of the buffer.
			 *
			 *	\tparam T
			 *		The type of the object to serialize.
			 */
			template <typename T>
			void ToBytes (const T & obj) {
			
				Serializer<type<T>>::ToBytes(
					buffer,
					obj
				);
			
			}
			
			
			/**
			 *	Save this ByteBuffer to the binary store
			 *	with the binary data within being associated
			 *	with a certain key.
			 *
			 *	\param [in] key
			 *		The key with which to associated this
			 *		ByteBuffer's data.
			 */
			void Save (const String & key);
			
			
			/**
			 *	Returns the number of bytes currently contained
			 *	in this byte buffer.
			 *
			 *	\return
			 *		The number of bytes in the buffer.
			 */
			Word Count () const noexcept;
	
	
	};


}

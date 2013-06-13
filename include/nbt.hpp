/**
 *	\file
 */
 
 
#pragma once
 
 
#include <common.hpp>
#include <type_traits>
#include <typeinfo>
#include <utility>
 
 
namespace MCPP {


	namespace NBT {


		/**
		 *	The types of a named binary tag.
		 */
		enum class TagType : Byte {
		
			End=0,			/**<	An end tag.  Unnamed.	*/
			Byte=1,			/**<	A single signed byte.	*/
			Short=2,		/**<	A single signed 16-bit integer.	*/
			Int=3,			/**<	A single signed 32-bit integer.	*/
			Long=4,			/**<	A single signed 64-bit integer.	*/
			Float=5,		/**<	An IEEE 754 32-bit floating point number.	*/
			Double=6,		/**<	An IEEE 754 64-bit floating point number.	*/
			ByteArray=7,	/**<	An array of bytes.	*/
			String=8,		/**<	A string of Unicode characters.	*/
			List=9,			/**<	A sequential list of unnamed tags.	*/
			Compound=10,	/**<	A sequential list of named tags.	*/
			IntArray=11		/**<	An array of 32-bit integers.	*/
		
		};
		
		
		/**
		 *	\cond
		 */
		
		
		class NamedTag;
		
		
		/**
		 *	\endcond
		 */
		
		
		/**
		 *	An unnamed binary tag.
		 */
		class Tag {
		
		
			private:
			
			
				TagType type;
				union {
					SByte tag_byte;
					Int16 tag_short;
					Int32 tag_int;
					Int64 tag_long;
					Single tag_float;
					Double tag_double;
					Vector<SByte> tag_bytearray;
					String tag_string;
					Vector<Tag> tag_list;
					Vector<NamedTag> tag_compound;
					Vector<Int32> tag_intarray;
				};
				
				
				inline void destroy () noexcept;
				inline void move (Tag &&) noexcept;
				inline void copy (const Tag &);
				template <typename T>
				const T & get () const;
				
				
			public:
			
			
				Tag () = delete;
				
				
				/**
				 *	Copies a tag.
				 */
				Tag (const Tag & other);
				/**
				 *	Moves a tag.
				 *
				 *	\param [in] other
				 *		The tag to move.
				 */
				Tag (Tag && other) noexcept;
				/**
				 *	Replaces this tag with a
				 *	copy of another tag.
				 *
				 *	\param [in] other
				 *		The tag to copy.
				 *
				 *	\return
				 *		A reference to this object.
				 */
				Tag & operator = (const Tag & other);
				/**
				 *	Replaces this tag by moving
				 *	another tag.
				 *
				 *	\param [in] other
				 *		The tag to move.
				 *
				 *	\return
				 *		A reference to this object.
				 */
				Tag & operator = (Tag && other) noexcept;
			
			
				Tag (SByte data) noexcept;
				Tag (Int16 data) noexcept;
				Tag (Int32 data) noexcept;
				Tag (Int64 data) noexcept;
				Tag (Single data) noexcept;
				Tag (Double data) noexcept;
				Tag (Vector<SByte> data) noexcept;
				Tag (String data) noexcept;
				Tag (Vector<Tag> data) noexcept;
				Tag (Vector<NamedTag> data) noexcept;
				Tag (Vector<Int32> data) noexcept;
				
				
				/**
				 *	Destroys this tag.
				 */
				~Tag () noexcept;
				
				
				/**
				 *	Retrieves the type of this tag.
				 */
				TagType Type () const noexcept;
				/**
				 *	Gets the value of the tag.
				 *
				 *	\tparam T
				 *		The type of value to retrieve.
				 *
				 *	\return
				 *		The value encapsulated by
				 *		this tag.
				 */
				template <typename T>
				T & Value ();
				/**
				 *	Gets the value of the tag.
				 *
				 *	\tparam T
				 *		The type of value to retrieve.
				 *
				 *	\return
				 *		The value encapsulated by
				 *		this tag.
				 */
				template <typename T>
				const T & Value () const;
				
				
				/**
				 *	Retrieves a string representation
				 *	of the type of this tag.
				 *
				 *	\return
				 *		A string representing the type
				 *		of this tag.
				 */
				String TypeToString () const;
				/**
				 *	Retrieves a string representation
				 *	of the value of this tag.
				 *
				 *	\return
				 *		A string representing the value
				 *		of this tag.
				 */
				String ValueToString () const;
		
		
		};
		
		
		template <typename T>
		const T & Tag::Value () const {
		
			return get<T>();
		
		}
		
		
		template <typename T>
		T & Tag::Value () {
		
			return const_cast<T &>(get<T>());
		
		}
		
		
		template <typename T>
		const T & Tag::get () const {
		
			//	Type check
		
			//	TAG_Byte
			if (!(
				//	TAG_Byte
				(
					(type==TagType::Byte) &&
					std::is_same<SByte,T>::value
				) ||
				//	TAG_Short
				(
					(type==TagType::Short) &&
					std::is_same<Int16,T>::value
				) ||
				//	TAG_Int
				(
					(type==TagType::Int) &&
					std::is_same<Int32,T>::value
				) ||
				//	TAG_Long
				(
					(type==TagType::Long) &&
					std::is_same<Int64,T>::value
				) ||
				//	TAG_Float
				(
					(type==TagType::Float) &&
					std::is_same<Single,T>::value
				) ||
				//	TAG_Double
				(
					(type==TagType::Double) &&
					std::is_same<Double,T>::value
				) ||
				//	TAG_Byte_Array
				(
					(type==TagType::ByteArray) &&
					std::is_same<Vector<SByte>,T>::value
				) ||
				//	TAG_String
				(
					(type==TagType::String) &&
					std::is_same<String,T>::value
				) ||
				//	TAG_List
				(
					(type==TagType::List) &&
					std::is_same<Vector<Tag>,T>::value
				) ||
				//	TAG_Compound
				(
					(type==TagType::Compound) &&
					std::is_same<Vector<NamedTag>,T>::value
				) ||
				//	TAG_IntArray
				(
					(type==TagType::IntArray) &&
					std::is_same<Vector<Int32>,T>::value
				)
			)) throw std::bad_cast();
			
			return *reinterpret_cast<const T *>(&tag_byte);
		
		}
		
		
		/**
		 *	A named binary tag.
		 */
		class NamedTag {
		
		
			private:
			
			
				String name;
				Tag tag;
				
				
			public:
			
			
				/**
				 *	Creates a named tag from a buffer
				 *	of bytes.
				 *
				 *	\param [in] begin
				 *		A pointer to the beginning of
				 *		a buffer of bytes.
				 *	\param [in] end
				 *		A pointer to the end of a buffer
				 *		of bytes.
				 *
				 *	\return
				 *		A named tag created from the buffer
				 *		of bytes.
				 */
				static NamedTag Create (const Byte * begin, const Byte * end);
			
			
				NamedTag () = delete;
			
			
				/**
				 *	Creates a new named tag.
				 *
				 *	\tparam T
				 *		The type of the parameter
				 *		\em data, which will be used to
				 *		create the Tag object which this
				 *		object names.
				 *
				 *	\param [in] name
				 *		The name of this tag.
				 *	\param [in] data
				 *		The payload of this tag.
				 */
				template <typename T>
				NamedTag (const String & name, T && data);
				/**
				 *	Creates a new named tag.
				 *
				 *	\tparam T
				 *		The type of the parameter
				 *		\em data, which will be used to
				 *		create the Tag object which this
				 *		object names.
				 *
				 *	\param [in] name
				 *		The name of this tag.
				 *	\param [in] data
				 *		The payload of this tag.
				 */
				template <typename T>
				NamedTag (String && name, T && data);
				
				
				/**
				 *	Retrieves the name of this tag.
				 *
				 *	\return
				 *		The name of this tag.
				 */
				String & Name () noexcept;
				/**
				 *	Retrieves the name of this tag.
				 *
				 *	\return
				 *		The name of this tag.
				 */
				const String & Name () const noexcept;
				/**
				 *	Gets the value of the named tag.
				 *
				 *	\tparam T
				 *		The type of value to retrieve.
				 *
				 *	\return
				 *		The value encapsulated by
				 *		this tag.
				 */
				template <typename T>
				T & Value ();
				/**
				 *	Gets the value of the named tag.
				 *
				 *	\tparam T
				 *		The type of value to retrieve.
				 *
				 *	\return
				 *		The value encapsulated by
				 *		this tag.
				 */
				template <typename T>
				const T & Value () const;
				/**
				 *	Retrieves the type of this tag.
				 */
				TagType Type () const noexcept;
		
		
		};
		
		
		template <typename T>
		NamedTag::NamedTag (const String & name, T && data) : name(name), tag(std::forward<T>(data)) {	}
		
		
		template <typename T>
		NamedTag::NamedTag (String && name, T && data) : name(std::move(name)), tag(std::forward<T>(data)) {	}
		
		
		template <typename T>
		T & NamedTag::Value () {
		
			return tag.Value<T>();
		
		}
		
		
		template <typename T>
		const T & NamedTag::Value () const {
		
			return tag.Value<T>();
		
		}
		
		
	}


}
 
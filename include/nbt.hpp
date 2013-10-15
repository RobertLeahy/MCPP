/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>
#include <hash.hpp>
#include <variant.hpp>
#include <exception>
#include <memory>
#include <unordered_map>
#include <utility>
 

namespace NBT {


	/**
	 *	The types that a tag may take on.
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


	template <typename... Args>
	using Variant=MCPP::Variant<Args...>;
	
	
	class NamedTag;
	class Tag;
	
	
	/**
	 *	\endcond
	 */
	 
	
	/**
	 *	A list of unnamed tags.
	 *
	 *	All tags must be of the same type.
	 */
	class List {
	
	
		public:
		
		
			/**
			 *	The type of the tags this list
			 *	contains.
			 */
			TagType Type;
			/**
			 *	The tags this list contains.
			 */
			Vector<Tag> Payload;
	
	
	};
	 
	
	/**
	 *	The type of a TAG_Compound.
	 */
	typedef std::unique_ptr<std::unordered_map<String,NamedTag>> Compound;
	/**
	 *	The type of a tag's payload.
	 */
	typedef Variant<
		SByte,
		Int16,
		Int32,
		Int64,
		Single,
		Double,
		Vector<SByte>,
		String,
		List,
		Compound,
		Vector<Int32>
	> PayloadType;
	
	
	/**
	 *	Thrown when an error occurs during an
	 *	NBT operation.
	 */
	class Error : public std::exception {
	
	
		private:
		
		
			const char * what_str;
			
			
		public:
		
		
			Error (const char * what) noexcept;
			
			
			virtual const char * what () const noexcept override;
	
	
	};
	
	
	/**
	 *	An unnamed tag.
	 */
	class Tag {
	
	
		public:
		
		
			Tag () = default;
			Tag (const Tag &) = default;
			Tag (Tag &&) = default;
			Tag & operator = (const Tag &) = default;
			Tag & operator = (Tag &&) = default;
			
			
			/**
			 *	Initializes an unnamed tag with
			 *	a given payload.
			 *
			 *	\param [in] payload
			 *		The payload to initialize this
			 *		tag with.
			 */
			Tag (PayloadType payload) noexcept;
		
		
			/**
			 *	The payload of the tag.
			 */
			PayloadType Payload;
			
			
			/**
			 *	Accesses the payload of this tag.
			 *
			 *	\return
			 *		The payload of this tag.
			 */
			PayloadType & operator * () noexcept;
			/**
			 *	Accesses the payload of this tag.
			 *
			 *	\return
			 *		The payload of this tag.
			 */
			const PayloadType & operator * () const noexcept;
			/**
			 *	Accesses the payload of this tag.
			 *
			 *	\return
			 *		The payload of this tag.
			 */
			PayloadType * operator -> () noexcept;
			/**
			 *	Accesses the payload of this tag.
			 *
			 *	\return
			 *		The payload of this tag.
			 */
			const PayloadType * operator -> () const noexcept;
	
	
	};
	
	
	/**
	 *	A named tag.
	 */
	class NamedTag : public Tag {
	
	
		public:
		
		
			NamedTag () = default;
			NamedTag (const NamedTag &) = default;
			NamedTag (NamedTag &&) = default;
			NamedTag & operator = (const NamedTag &) = default;
			NamedTag & operator = (NamedTag &&) = default;
		
		
			/**
			 *	Initializes a named tag with a given
			 *	name and payload.
			 *
			 *	\param [in] name
			 *		The name of this tag.
			 *	\param [in] payload
			 *		The payload of this tag.
			 */
			NamedTag (String name, PayloadType payload) noexcept;
		
		
			/**
			 *	The name of this tag.
			 */
			String Name;
	
	
	};
	
	
	/**
	 *	The type of the unordered map which
	 *	contains the key/value pairs which
	 *	make up a TAG_Compound.
	 */
	typedef Compound::element_type KeyValue;
	
	
	/**
	 *	\cond
	 */
	
	
	namespace NBTImpl {
	
	
		inline void Add (const Compound &) noexcept {	}
	
	
		template <typename T, typename... Args>
		void Add (Compound & compound, T && tag, Args &&... args) {
		
			String name=tag.Name;
		
			compound->emplace(
				name,
				std::forward<T>(tag)
			);
			
			Add(compound,std::forward<Args>(args)...);
		
		}
	
	
	}
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Adds named tags to a TAG_Compound.
	 *
	 *	\param [in,out] compound
	 *		The TAG_Compound to which tags
	 *		shall be added.
	 *	\param [in] args
	 *		Any number of named tags which
	 *		shall be added to the collection.
	 *
	 *	\return
	 *		A reference to \em compound.
	 */
	template <typename... Args>
	Compound & Add (Compound & compound, Args &&... args) {
	
		if (!compound) compound=Compound(new KeyValue());
		
		NBTImpl::Add(compound,std::forward<Args>(args)...);
		
		return compound;
	
	}
	
	
	/**
	 *	Creates a new TAG_Compound and initializes
	 *	it with named tags.
	 *
	 *	\param [in] args
	 *		Any number of named tags which shall
	 *		be added to the collection.
	 *
	 *	\return
	 *		A newly-created TAG_Compound with \em args
	 *		added to it.
	 */
	template <typename... Args>
	Compound Make (Args &&... args) {
	
		Compound retr;
		
		Add(retr,std::forward<Args>(args)...);
		
		return retr;
	
	}
	
	
	/**
	 *	Serializes NBT.
	 *
	 *	\param [in] tag
	 *		The named tag which is the root
	 *		of the NBT structure.
	 *
	 *	\return
	 *		A buffer containing bytes which
	 *		may be parsed to obtain \em tag.
	 */
	Vector<Byte> Serialize (const NamedTag & tag);
	/**
	 *	Serializes NBT.
	 *
	 *	\param [in,out] buffer
	 *		The buffer to which bytes shall
	 *		be appended.
	 *	\param [in] tag
	 *		The named tag which is the root
	 *		of the NBT structure.
	 */
	void Serialize (Vector<Byte> & buffer, const NamedTag & tag);
	
	
	/**
	 *	Parses bytes into NBT.
	 *
	 *	\param [in,out] begin
	 *		An iterator to the beginning
	 *		of a buffer of bytes.  This
	 *		iterator shall be advanced as
	 *		bytes are consumed.
	 *	\param [in] end
	 *		An iterator to the end of a buffer
	 *		of bytes.
	 *	\param [in] max_depth
	 *		The maximum recursive depth to
	 *		the parser shall descend before
	 *		throwing an exception.  Defaults
	 *		to zero, which is infinite depth.
	 *
	 *	\return
	 *		A named tag which contains the data
	 *		serialized between \em begin and
	 *		\em end.
	 */
	NamedTag Parse (const Byte * & begin, const Byte * end, Word max_depth=0);
	
	
	/**
	 *	Renders a named tag as a string.
	 *
	 *	\param [in] tag
	 *		A named tag.
	 *
	 *	\return
	 *		A string representation of the
	 *		NBT structure which descends
	 *		from \em tag.
	 */
	String ToString (const NamedTag & tag);
	
	
}
 
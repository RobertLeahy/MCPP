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
 

namespace NBT {


	template <typename... Args>
	using Variant=MCPP::Variant<Args...>;
	
	
	/**
	 *	\cond
	 */
	
	
	class NamedTag;
	class Tag;
	
	
	/**
	 *	\endcond
	 */
	 
	 
	class List {
	
	
		public:
		
		
			Byte Type;
			Vector<Tag> Payload;
	
	
	};
	 
	 
	typedef std::unique_ptr<
		std::unordered_map<
			String,
			NamedTag
		>
	> Compound;
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
	 
	 
	class Error : public std::exception {
	
	
		private:
		
		
			const char * what_str;
			
			
		public:
		
		
			Error (const char * what) noexcept;
			
			
			virtual const char * what () const noexcept override;
	
	
	};
	
	
	class Tag {
	
	
		public:
		
		
			PayloadType Payload;
	
	
	};
	
	
	class NamedTag : public Tag {
	
	
		public:
		
		
			String Name;
	
	
	};
	
	
	Vector<Byte> Serialize (const NamedTag & tag);
	void Serialize (Vector<Byte> & buffer, const NamedTag & tag);
	
	
	NamedTag Parse (const Byte * & begin, const Byte * end, Word max_depth=0);
	
	
	String ToString (const NamedTag & tag);
	
	
}
 
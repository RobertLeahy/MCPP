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
 

/**
 *	Contains utilities for working with
 *	JSON data.
 */
namespace JSON {


	/**
	 *	\cond
	 */


	template <typename... Args>
	using Variant=MCPP::Variant<Args...>;


	class Object;
	
	
	/**
	 *	\endcond
	 */
	 
	
	/**
	 *	Thrown when any JSON method encounters
	 *	an error.
	 */
	class Error : public std::exception {
	
	
		private:
		
		
			const char * what_str;
	
	
		public:
		
		
			Error (const char * what) noexcept;
			
			
			virtual const char * what () const noexcept override;
	
	
	};
	
	
	/**
	 *	Represents a JSON array.
	 */
	class Array {
	
	
		public:
		
		
			/**
			 *	The values in the JSON array.
			 */
			Vector<
				Variant<
					String,
					Double,
					Object,
					Array,
					bool
				>
			> Values;
	
	
	};


	/**
	 *	Represents a JSON object.
	 */
	class Object {
	
	
		public:
		
		
			/**
			 *	A pointer to an unordered map
			 *	which contains the key value
			 *	pairs which make up the object.
			 *
			 *	May be null, in which case this
			 *	object is considered to be
			 *	synonymous with the JSON value
			 *	\em null.
			 */
			std::unique_ptr<
				std::unordered_map<
					String,
					Variant<
						String,
						Double,
						Object,
						Array,
						bool
					>
				>
			> Pairs;
			
			
			/**
			 *	Creates a new unordered map
			 *	pointer to by Pairs.
			 */
			void Construct ();
			
			
			/**
			 *	Determines whether or not this
			 *	object is null.
			 *
			 *	\return
			 *		\em true if this object is
			 *		null, \em false otherwise.
			 */
			bool IsNull () const noexcept;
	
	
	};
	
	
	/**
	 *	The type of a single JSON value.
	 */
	typedef Variant<String,Double,Object,Array,bool> Value;
	
	
	/**
	 *	\cond
	 */
	 
	 
	String Serialize (const String & str);
	
	
	String Serialize (Double dbl);
	
	
	String Serialize (std::nullptr_t ptr);
	
	
	String Serialize (const Object & obj);
	
	
	String Serialize (const Array & arr);
	
	
	String Serialize (bool bln);
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Serializes a JSON value to JSON.
	 *
	 *	\param [in] value
	 *		The value to serialize.
	 *
	 *	\return
	 *		The JSON representation of
	 *		\em value.
	 */
	String Serialize (const Value & value);
	
	
	/**
	 *	Parses JSON into a JSON value.
	 *
	 *	\param [in] json
	 *		The JSON to parse.
	 *	\param [in] max_depth
	 *		The maximum recursive depth
	 *		to which the parser will follow
	 *		the JSON structure before throwing
	 *		an exception.  Defaults to zero,
	 *		or unlimited depth.
	 *
	 *	\return
	 *		A single JSON value.
	 */
	Value Parse (const String & json, Word max_depth=0);
	
	
	/**
	 *	Creates a string representation of
	 *	a JSON value.
	 *
	 *	\param [in] value
	 *		The JSON value to represent.
	 *
	 *	\return
	 *		A formatted, human-readable string
	 *		which represents \em value.
	 */
	String ToString (const Value & value);


}

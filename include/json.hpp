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
					Double,
					String,
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
	
	
		private:
		
		
			inline void add_impl () const noexcept {	}
			
			
			template <typename T1, typename T2, typename... Args>
			void add_impl (T1 &&, T2 &&, Args &&...);
	
	
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
						Double,
						String,
						Object,
						Array,
						bool
					>
				>
			> Pairs;
			
			
			/**
			 *	Adds an arbitrary number of key/value
			 *	pairs to the object.
			 *
			 *	\param [in] args
			 *		An arbitrary number of arguments,
			 *		where the first is a key, the
			 *		second is a value, and so on.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			template <typename... Args>
			Object & Add (Args &&... args);
			
			
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
	typedef Variant<Double,String,Object,Array,bool> Value;
	
	
	/**
	 *	\cond
	 */
	 
	 
	template <typename T1, typename T2, typename... Args>
	void Object::add_impl (T1 && key, T2 && value, Args &&... args) {
	
		Pairs->emplace(
			String(std::forward<T1>(key)),
			Value(std::forward<T2>(value))
		);
		
		add_impl(std::forward<Args>(args)...);
	
	}
	 
	 
	template <typename... Args>
	Object & Object::Add (Args &&... args) {
	
		//	Create a new dictionary if
		//	necessary
		if (!Pairs) Construct();
		
		//	Call helper
		add_impl(std::forward<Args>(args)...);
		
		//	Return self reference
		return *this;
	
	}
	 
	 
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

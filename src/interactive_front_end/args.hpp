#pragma once


#include <rleahylib/rleahylib.hpp>
#include <hash.hpp>
#include <exception>
#include <unordered_map>


class InvalidArg : public std::exception {


	private:
	
	
		Word which;
		
		
	public:
	
	
		InvalidArg () = delete;
		InvalidArg (Word which) noexcept;
		
		
		Word Which () const noexcept;


};


class Args {


	private:
	
	
		std::unordered_map<
			String,
			Vector<String>
		> args;
		
		
		Args (std::unordered_map<String,Vector<String>> args) noexcept;
		
		
	public:
	
	
		static Args Parse (const Vector<const String> & args);
	
	
		Args () = delete;
		
		
		bool IsSet (const String & arg) const;
		const Vector<String> * Get (const String & arg) const;


};

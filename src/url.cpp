#include <url.hpp>


namespace MCPP {


	String URL::Encode (const String & encode) {
	
		//	Convert to UTF-8 representation
		Vector<Byte> utf8(UTF8().Encode(encode));
		
		//	Loop and reconstruct
		String encoded;
		for (const Byte b : utf8) {
		
			if (
				//	Outside ASCII range
				(b>=128) ||
				(
					//	Not A-Z
					(
						(b<'A') ||
						(b>'Z')
					) &&
					//	Not a-z
					(
						(b<'a') ||
						(b>'z')
					) &&
					//	Not 0-9
					(
						(b<'0') ||
						(b>'9')
					) &&
					//	Not -
					(b!='-') &&
					//	Not _
					(b!='_') &&
					//	Not .
					(b!='.') &&
					//	Not ~
					(b!='~')
				)
			) {
			
				//	Percent encode
				
				encoded << "%";
				
				String representation(b,16);
				
				if (representation.Count()==1) encoded << "0";
				
				encoded << representation;
			
			} else {
			
				//	Add as regular character
				encoded << GraphemeCluster(
					static_cast<ASCIIChar>(b)
				);
			
			}
		
		}
		
		return encoded;
	
	}
	
	
	String URL::Decode (const String & decode) {
	
		Vector<Byte> utf8(UTF8().Encode(decode));
		
		Vector<Byte> decoded;
		for (
			auto begin=utf8.begin(), end=utf8.end();
			begin!=end;
			++begin
		) {
		
			if (*begin=='%') {
			
				//	Save current position
				//	so we can roll back if
				//	parsing fails
				auto restore=begin;
				
				//	Extract the two characters
				//	that follow the % and designate
				//	the code unit
				String hex;
				for (Word i=0;i<2;++i) {
				
					//	Attempt to advance
					if (
						//	Fail because there's not
						//	enough data
						(++begin==end) ||
						//	Fail because this byte
						//	is not an ASCII character
						(*begin>=128)
					) {
					
						//	Fail
						begin=restore;
						
						goto literal;
					
					}
					
					hex << GraphemeCluster(
						static_cast<ASCIIChar>(*begin)
					);
				
				}
				
				//	Attempt to parse the two
				//	values into a byte
				Byte b;
				if (!hex.ToInteger(&b,16)) {
				
					//	Failed, restore and
					//	treat as literals
					begin=restore;
					
					goto literal;
				
				}
				
				//	Add byte
				decoded.Add(b);
			
			} else {
			
				literal:
			
				//	Copy byte verbatim
				decoded.Add(*begin);
				
			}
			
		}
		
		//	Decode and return
		return UTF8().Decode(decoded.begin(),decoded.end());
	
	}


}

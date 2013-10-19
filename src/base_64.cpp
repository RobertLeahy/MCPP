#include <base_64.hpp>
#include <utility>


namespace Base64 {


	//	Encoding table
	static const ASCIIChar encode []={
		'A','B','C','D','E','F','G','H',
		'I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X',
		'Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n',
		'o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3',
		'4','5','6','7','8','9','+','/'
	};
	static const ASCIIChar pad='=';
	
	
	String Encode (const Vector<Byte> & buffer) {
	
		auto begin=buffer.begin();
		auto end=buffer.end();
		
		//	This will be converted to
		//	a string and then returned
		Vector<CodePoint> cps;
		
		//	Iterate over entire input
		//	buffer
		while (begin!=end) {
		
			//	We need 24 bits for 4 base 64
			//	characters, they'll be stored
			//	in here
			UInt32 build=0;
			//	Keep track of the number of significant
			//	bytes for padding purposes
			Word significant=0;
			//	Extract 3 bytes (24 bits)
			for (Word i=0;i<3;++i) {
			
				//	Make room for the next byte
				build<<=BitsPerByte();
				
				//	Only add the next byte if
				//	there's a byte to extract
				if (begin!=end) {
				
					build|=*begin;
					
					//	Advance buffer pointer
					++begin;
					//	A byte from the buffer
					//	is significant
					++significant;
				
				}
			
			}
			
			Word printed=0;
			//	Print base 64 characters
			for (;printed<(significant+1);++printed) cps.Add(static_cast<CodePoint>(encode[(build>>(6*(3-printed)))&63]));
			//	Print padding (if necessary)
			for (;printed<4;++printed) cps.Add(static_cast<CodePoint>(pad));
		
		}
		
		return String(std::move(cps));
	
	}


}

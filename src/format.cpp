#include <format.hpp>


namespace MCPP {


	String FormatVersion (Word major, Word minor, Word patch) {
	
		String retr(major);
		
		if (!((minor==0) && (patch==0))) {
		
			retr << "." << String(minor);
			
			if (patch!=0) retr << "." << String(patch);
		
		}
		
		return retr;
	
	}


}

static size_t write_callback (char * ptr, size_t size, size_t nmemb, void * userdata) noexcept {

	try {
	
		Word safe_size=Word(SafeWord(size)*SafeWord(nmemb));
		
		return reinterpret_cast<HTTPRequest *>(userdata)->Write(
			reinterpret_cast<Byte *>(ptr),
			safe_size
		);
	
	} catch (...) {	}

	return 0;
	
}


static size_t header_callback (char * ptr, size_t size, size_t nmemb, void * userdata) noexcept {

	try {
	
		Word safe_size=Word(SafeWord(size)*SafeWord(nmemb));
		
		//	Decode header
		String header(UTF8().Decode(
			reinterpret_cast<Byte *>(ptr),
			reinterpret_cast<Byte *>(ptr)+safe_size
		));
		
		//	Parse it
		String key;
		String value;
		bool found=false;
		for (const auto & gc : header) {
		
			if (found) {
			
				value << gc;
			
			} else if (gc==':') {
			
				found=true;
			
			} else {
			
				key << gc;
			
			}
		
		}
		
		//	Trim
		key.Trim();
		value.Trim();
		
		//	Only proceed if this is a "real"
		//	header
		if (found) {
		
			//	Dispatch callback
			
			if (reinterpret_cast<HTTPRequest *>(userdata)->Header(key,value)) return safe_size;
			
			return 0;
		
		}
		
		//	Succeed
		return safe_size;
	
	} catch (...) {	}
	
	//	Fail
	return 0;

}


static size_t read_callback (void * ptr, size_t size, size_t nmemb, void * userdata) noexcept {

	try {
	
		Word safe_size=Word(SafeWord(size)*SafeWord(nmemb));
		
		return reinterpret_cast<HTTPRequest *>(userdata)->Read(
			reinterpret_cast<Byte *>(ptr),
			safe_size
		);
	
	} catch (...) {	}
	
	//	Fail
	return 0;

}

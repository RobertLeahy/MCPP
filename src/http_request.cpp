#include <http_handler.hpp>
#include <utility>


namespace MCPP {


	//static const Regex header_regex("(?<=charset\\=)(\\W*)");
	
	
	HTTPRequest::HTTPRequest (
		CURL * handle,
		Word max_bytes,
		const String & url,
		HTTPStatusStringDone status_string_done
	)	:	status_string_done(std::move(status_string_done)),
			max_bytes(max_bytes),
			handle(handle),
			url(UTF8().Encode(url))
	{
	
		this->url.Add(0);
	
	}
	
	
	HTTPRequest::~HTTPRequest () noexcept {
	
		curl_easy_cleanup(handle);
	
	}


	Word HTTPRequest::Write (const Byte * ptr, Word size) noexcept {
	
		try {
		
			//	Call user-supplied write function
			//	if it exists
			if (write) return write(ptr,size);
			
			//	Check to make sure we're not
			//	receiving more data than is
			//	permissible
			Word new_size=Word(SafeWord(size)+SafeWord(buffer.Count()));
			
			if (
				(max_bytes!=0) &&
				(new_size>max_bytes)
			) return 0;
			
			//	Add bytes to the buffer
			buffer.Add(ptr,ptr+size);
			
			//	Succeed
			return size;
		
		} catch (...) {	}
		
		//	Eat exceptions and return failure
		return 0;
	
	}
	
	
	Word HTTPRequest::Read (Byte * ptr, Word size) noexcept {
	
		try {
		
			return read ? read(ptr,size) : 0;
		
		} catch (...) {	}
		
		return 0;
	
	}
	
	
	void HTTPRequest::Done (Word status_code) noexcept {
	
		try {
		
			if (status_string_done) {
			
				if (status_code==0) {
				
					status_string_done(status_code,String());
				
				} else {
				
					try {
				
						if (encoding.IsNull()) encoding=SmartPointer<Encoding>::Make<UTF8>();
						
						String decoded(encoding->Decode(buffer.begin(),buffer.end()));
						
						try {
						
							status_string_done(status_code,std::move(decoded));
							
						} catch (...) {	}
						
					} catch (...) {
					
						status_string_done(status_code,String());
					
					}
				
				}
			
			} else if (status_done) {
			
				status_done(status_code);
			
			}
		
		} catch (...) {	}
	
	}
	
	
	bool HTTPRequest::Header (const String & key, const String & value) noexcept {
	
		if (header) {
		
			try {
			
				header(key,value);
			
			} catch (...) {
			
				//	Fail
				return false;
			
			}
		
		}
		
		//	TODO: Add encoding detection
		
		//	Suceed
		return true;
	
	}
	
	
	const Vector<Byte> & HTTPRequest::URL () const noexcept {
	
		return url;
	
	}


}

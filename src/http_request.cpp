#include <http_handler.hpp>
#include <cstring>
#include <utility>


namespace MCPP {


	//static const Regex header_regex("(?<=charset\\=)(\\W*)");
	
	
	HTTPRequest::HTTPRequest (
		CURL * handle,
		Word max_bytes,
		const String & url,
		HTTPStatusStringDone status_string_done
	)	:	status_string_done(std::move(status_string_done)),
			outgoing_pos(0),
			max_bytes(max_bytes),
			handle(handle),
			url(UTF8().Encode(url)),
			headers(nullptr)
	{
	
		this->url.Add(0);
	
	}
	
	
	HTTPRequest::HTTPRequest (
		CURL * handle,
		Word max_bytes,
		const String & url,
		const String & body,
		struct curl_slist * headers,
		HTTPStatusStringDone status_string_done
	)	:	status_string_done(std::move(status_string_done)),
			outgoing_pos(0),
			outgoing(UTF8().Encode(body)),
			max_bytes(max_bytes),
			handle(handle),
			url(UTF8().Encode(url)),
			headers(headers)
	{
	
		this->url.Add(0);
	
	}
	
	
	HTTPRequest::~HTTPRequest () noexcept {
	
		curl_easy_cleanup(handle);
		
		if (headers!=nullptr) curl_slist_free_all(headers);
	
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
	
		if (read) try {
		
			return read(ptr,size);
			
		} catch (...) {
		
			return 0;
		
		}
		
		Word count=outgoing.Count()-outgoing_pos;
		
		Word num=(count<size) ? count : size;
		
		std::memcpy(
			ptr,
			outgoing.begin()+outgoing_pos,
			num
		);
		
		outgoing_pos+=num;
		
		return num;
	
	}
	
	
	void HTTPRequest::Done (Word status_code) noexcept {
	
		try {
		
			if (status_string_done) {
			
				if (status_code==0) {
				
					status_string_done(status_code,String());
				
				} else {
				
					try {
					
						String decoded=encoding ? encoding->Decode(
							buffer.begin(),
							buffer.end()
						) : UTF8().Decode(
							buffer.begin(),
							buffer.end()
						);
						
						try {
						
							status_string_done(status_code,std::move(decoded));
							
						} catch (...) {	}
						
					} catch (...) {
					
						status_string_done(0,String());
					
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
	
	
	const Vector<Byte> & HTTPRequest::Body () const noexcept {
	
		return outgoing;
	
	}


}

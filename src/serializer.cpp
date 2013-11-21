#include <serializer.hpp>
#include <data_provider.hpp>
#include <server.hpp>


namespace MCPP {


	const char * SerializerError::InsufficientBytes="Insufficient bytes";
	const char * SerializerError::InvalidBoolean="Invalid boolean value";


	ByteBufferError::ByteBufferError (const char * what_str, Word where) noexcept : what_str(what_str), where(where) {	}
	
	
	const char * ByteBufferError::what () const noexcept {
	
		return what_str;
	
	}
	
	
	Word ByteBufferError::Where () const noexcept {
	
		return where;
	
	}


	ByteBuffer ByteBuffer::Load (const String & key) {
	
		ByteBuffer retr;
		
		auto buffer=Server::Get().Data().GetBinary(key);
		
		if (!buffer.IsNull()) retr.buffer=std::move(*buffer);
		
		return retr;
	
	}


	ByteBuffer::ByteBuffer () noexcept : loc(nullptr) {	}
	
	
	void ByteBuffer::Save (const String & key) {
	
		Server::Get().Data().SaveBinary(
			key,
			buffer.begin(),
			buffer.Count()
		);
	
	}
	
	
	Word ByteBuffer::Count () const noexcept {
	
		return buffer.Count();
	
	}


}

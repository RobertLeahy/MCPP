#include <compression.hpp>
#ifdef ENVIRONMENT_WINDOWS
#define ZLIB_WINAPI
#endif
#include <zlib.h>
#include <stdexcept>
#include <type_traits>


namespace MCPP {


	static const char * zlib_error="ZLib error";
	static const Word inflate_buffer_default=256;
	
	
	void Inflate (const Byte * begin, const Byte * end, Vector<Byte> * buffer) {
		
		//	Null check
		if (buffer==nullptr) throw std::out_of_range(NullPointerError);
		
		//	Preserve initial count for atomicity
		Word initial_count=buffer->Count();
		
		//	In event of error we roll
		//	buffer back
		try {
		
			//	Create and initialize a stream
			z_stream stream;
			stream.zalloc=Z_NULL;
			stream.zfree=Z_NULL;
			stream.opaque=Z_NULL;
			
			if (inflateInit2(&stream,32+MAX_WBITS)!=Z_OK) throw std::runtime_error(zlib_error);
			
			try {
			
				//	Wire input buffer into the stream
				auto avail=end-begin;
				stream.avail_in=static_cast<decltype(stream.avail_in)>(SafeInt<decltype(avail)>(avail));
				stream.next_in=const_cast<decltype(stream.next_in)>(
					reinterpret_cast<const std::remove_pointer<decltype(stream.next_in)>::type *>(
						begin
					)
				);
				
				//	Loop and decompress
				for (;;) {
				
					//	Wire output buffer into stream
					stream.next_out=reinterpret_cast<decltype(stream.next_out)>(buffer->end());
					stream.avail_out=static_cast<decltype(stream.avail_out)>(
						SafeWord(
							buffer->Capacity()-buffer->Count()
						)
					);
					
					//	Inflate
					int result=inflate(&stream,Z_FINISH);
					
					//	Update buffer count
					buffer->SetCount(buffer->Capacity()-stream.avail_out);
					
					//	Did we decompress everything?
					//	If so we're done.
					if (result==Z_STREAM_END) break;
					
					//	Was more buffer space required?
					if ((result==Z_BUF_ERROR) && (stream.avail_out==0)) {
					
						buffer->SetCapacity();
						
						continue;
					
					}
					
					//	Something went wrong, throw
					throw std::runtime_error(zlib_error);
				
				}
				
			} catch (...) {
			
				//	Make sure stream is
				//	cleaned up
				inflateEnd(&stream);
				
				throw;
			
			}
			
			inflateEnd(&stream);
		
		} catch (...) {
		
			//	Roll buffer back
			buffer->SetCount(initial_count);
			
			//	Propagate
			throw;
		
		}
	
	}


	Vector<Byte> Inflate (const Byte * begin, const Byte * end) {
		
		//	Create buffer
		Vector<Byte> buffer(inflate_buffer_default);
		
		//	Inflate
		Inflate(begin,end,&buffer);
		
		//	Return
		return buffer;
	
	}
	
	
	void Deflate (const Byte * begin, const Byte * end, Vector<Byte> * buffer, bool gzip) {
	
		//	Null check
		if (buffer==nullptr) throw std::out_of_range(NullPointerError);
		
		//	Preserve initial count for atomicity
		Word initial_count=buffer->Count();
		
		//	In event of error we roll
		//	buffer back
		try {
		
			//	Create and initialize a stream
			z_stream stream;
			stream.zalloc=Z_NULL;
			stream.zfree=Z_NULL;
			stream.opaque=Z_NULL;
			
			if (deflateInit2(
				&stream,
				Z_DEFAULT_COMPRESSION,
				Z_DEFLATED,
				MAX_WBITS+(gzip ? 16 : 0),
				8,
				Z_DEFAULT_STRATEGY
			)!=Z_OK) throw std::runtime_error(zlib_error);
			
			try {
			
				//	Convert the number of bytes to
				//	compress safely to the target
				//	integer type and store them
				//	in stream
				stream.avail_in=static_cast<decltype(stream.avail_in)>(
					SafeInt<decltype(end-begin)>(end-begin)
				);
				//	The ZLib compression/decompression
				//	functions do not modify the in buffer,
				//	therefore it should technically be
				//	const, but it's not so we'll do an
				//	ugly const_cast here...
				stream.next_in=const_cast<decltype(stream.next_in)>(
					reinterpret_cast<const std::remove_pointer<decltype(stream.next_in)>::type *>(
						begin
					)
				);
				
				//	Loop and compress
				for (;;) {
				
					//	Wire output buffer into stream
					stream.next_out=reinterpret_cast<decltype(stream.next_out)>(buffer->end());
					stream.avail_out=static_cast<decltype(stream.avail_out)>(SafeWord(buffer->Capacity()-buffer->Count()));
					
					//	Deflate
					int result=deflate(&stream,Z_FINISH);
					
					//	Update buffer count
					buffer->SetCount(buffer->Capacity()-stream.avail_out);
					
					//	Did we compress everything?
					//	If so we're done
					if (result==Z_STREAM_END) break;
					
					//	Does the buffer need to be bigger?
					if (
						(result==Z_OK) ||
						(
							(result==Z_BUF_ERROR) &&
							(stream.avail_out==0)
						)
					) {

						buffer->SetCapacity();
						
						continue;
						
					}
					
					//	Error
					throw std::runtime_error(zlib_error);
				
				}
				
			} catch (...) {
			
				//	Make sure the stream is
				//	cleaned up
				deflateEnd(&stream);
				
				throw;
			
			}
			
			deflateEnd(&stream);
		
		} catch (...) {
		
			//	Roll buffer back
			buffer->SetCount(initial_count);
			
			//	Propagate
			throw;
		
		}
	
	}
	
	
	Vector<Byte> Deflate (const Byte * begin, const Byte * end, bool gzip) {
	
		//	We want to try and do this in
		//	one go, so get the bound and
		//	initialize the output buffer
		//	to that size
		Word num=Word(SafeInt<decltype(end-begin)>(end-begin));
		Vector<Byte> buffer(DeflateBound(num,gzip));
		
		//	Compress
		Deflate(begin,end,&buffer,gzip);
		
		//	Return
		return buffer;
	
	}
	
	
	Word DeflateBound (Word num, bool gzip) {
	
		//	Safely convert the input number to
		//	the type that deflateBound expects
		typedef FunctionInformation<decltype(deflateBound)>::ArgumentType<1>::Type LengthType;
		
		LengthType z_num=LengthType(SafeWord(num));
	
		//	Prepare a dummy stream
		z_stream stream;
		stream.zalloc=Z_NULL;
		stream.zfree=Z_NULL;
		stream.opaque=Z_NULL;
		
		if (deflateInit2(
			&stream,
			Z_DEFAULT_COMPRESSION,
			Z_DEFLATED,
			MAX_WBITS+(gzip ? 16 : 0),
			8,
			Z_DEFAULT_STRATEGY
		)!=Z_OK) throw std::runtime_error(zlib_error);
		
		Word bound;
		try {
		
			//	Obtain the bound
			bound=Word(
				SafeInt<decltype(deflateBound(&stream,z_num))>(
					deflateBound(&stream,z_num)
				)
			);
			
		} catch (...) {
		
			deflateEnd(&stream);
			
			throw;
		
		}
		
		//	Free the stream
		deflateEnd(&stream);
		
		//	Return
		return bound;
	
	}


}

#include <chunk.hpp>
#include <utility>
#include <stdexcept>


namespace MCPP {


	static const char * size_error="Chunk has incorrect size";


	Chunk::Chunk (Vector<Byte> && data, bool skylight, bool add) noexcept : data(0) {
	
		//	Target size starts out as
		//	1 byte per block for block
		//	info, 0.5 bytes per block for
		//	metadata, 0.5 bytes per block
		//	for light, and 1 byte per
		//	XZ co-ordinate for biomes.
		Word target_size=(2*16*16*16)+(16*16);
		
		//	Add a half byte per block if
		//	the buffer supposedly has
		//	skylight info.
		if (skylight) target_size+=16*16*8;
		
		//	Add a half byte per block if
		//	the buffer supposedly has
		//	add info.
		if (add) target_size+=16*16*8;
		
		//	Check
		if (data.Length()!=target_size) throw std::out_of_range(size_error);
		
		//	Set
		this->data=std::move(data);
		this->skylight=skylight;
		this->add=add;
	
	}
	
	
	void Chunk::Read () noexcept {
	
		lock.Read();
	
	}
	
	
	void Chunk::CompleteRead () noexcept {
	
		lock.CompleteRead();
	
	}
	
	
	void Chunk::Write () noexcept {
	
		lock.Write();
	
	}
	
	
	void Chunk::CompleteWrite () noexcept {
	
		lock.CompleteWrite();
	
	}
	
	
	Byte * Chunk::begin () noexcept {
	
		return data.begin();
	
	}
	
	
	const Byte * Chunk::begin () const noexcept {
	
		return data.begin();
	
	}
	
	
	Byte * Chunk::end () noexcept {
	
		return data.end();
	
	}
	
	
	const Byte * Chunk::end () const noexcept {
	
		return data.end();
	
	}
	
	
	Byte & Chunk::Biome (Byte x, Byte z) noexcept {
	
		return data[
			//	Get position where biome array begins
			(data.Count()-256)+
			//	Add offset of this x/z combination
			(z*16)+x
		];
	
	}
	
	
	const Byte & Chunk::Biome (Byte x, Byte z) const noexcept {
	
		return data[
			//	Get position where biome array begins
			(data.Count()-256)+
			//	Add offset of this x/z combination
			(z*16)+x
		];
	
	}
	
	
	Word Chunk::Count () const noexcept {
	
		return data.Count();
	
	}
	
	
	bool Chunk::Add () const noexcept {
	
		return add;
	
	}


}

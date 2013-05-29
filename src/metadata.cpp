#include <metadata.hpp>


namespace MCPP {


	inline void Metadatum::destroy () noexcept {
	
		//	Cleanup string
		if (type==MetadatumType::String) string.~String();
		
		//	TODO: Add slot
	
	}


	Metadatum::~Metadatum () noexcept {
	
		destroy();
	
	}
	
	
	inline void Metadatum::copy_move_shared (const Metadatum & other) noexcept {
	
		type=other.type;
		key=other.key;
	
	}
	
	
	inline void Metadatum::copy (const Metadatum & other) {
	
		switch (other.type) {
		
			case MetadatumType::Byte:
			
				sbyte=other.sbyte;
				
				break;
				
			case MetadatumType::Short:
			
				int16=other.int16;
				
				break;
				
			case MetadatumType::Int:
			
				int32=other.int32;
				
				break;
				
			case MetadatumType::Float:
			
				single=other.single;
				
				break;
				
			case MetadatumType::String:
			
				new (&string) String (other.string);
				
				break;
				
			//	TODO: Add slot
			
			default:
			
				new (&coords) Coordinates (other.coords);
				
				break;
		
		}
	
	}
	
	
	inline void Metadatum::move (Metadatum && other) noexcept {
	
		switch (other.type) {
		
			case MetadatumType::String:
			
				new (&string) String(std::move(other.string));
				
				break;
				
			default:
			
				copy(other);
				
				break;
		
		}
	
	}
	
	
	Metadatum::Metadatum (const Metadatum & other) {
	
		copy(other);
		
		copy_move_shared(other);
	
	}
	
	
	Metadatum::Metadatum (Metadatum && other) noexcept {
	
		move(std::move(other));
		
		copy_move_shared(other);
	
	}
	
	
	Metadatum & Metadatum::operator = (const Metadatum & other) {
	
		destroy();
		
		copy(other);
		
		copy_move_shared(other);
		
		return *this;
	
	}
	
	
	Metadatum & Metadatum::operator = (Metadatum && other) noexcept {
	
		destroy();
		
		move(std::move(other));
		
		copy_move_shared(other);
		
		return *this;
	
	}
	
	
	Byte Metadatum::Key () const noexcept {
	
		return key;
	
	}
	
	
	MetadatumType Metadatum::Type () const noexcept {
	
		return type;
	
	}


}

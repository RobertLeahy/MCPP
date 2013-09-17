#include <world/world.hpp>


namespace MCPP {


	static inline Int32 adjust_coord (Double coord) {
	
		if (coord<0) {
		
			Double rem=coord-static_cast<Double>(
				static_cast<Int64>(
					coord
				)
			);
			
			if (rem!=0) coord-=1;
		
		}
		
		return static_cast<Int32>(coord);
	
	}


	ColumnID ColumnID::GetContaining (Double x, Double z, SByte dimension) noexcept {
	
		return BlockID{
			adjust_coord(x),
			0,
			adjust_coord(z),
			dimension
		}.GetContaining();
	
	}


	bool ColumnID::operator == (const ColumnID & other) const noexcept {
	
		return (
			(X==other.X) &&
			(Z==other.Z) &&
			(Dimension==other.Dimension)
		);
	
	}
	
	
	bool ColumnID::operator != (const ColumnID & other) const noexcept {
	
		return !(*this==other);
	
	}
	
	
	bool ColumnID::DoesContain (const BlockID & block) const noexcept {
	
		return block.IsContainedBy(*this);
	
	}
	
	
	Int32 ColumnID::GetStartX () const noexcept {
	
		return X*16;
	
	}
	
	
	Int32 ColumnID::GetStartZ () const noexcept {
	
		return Z*16;
	
	}
	
	
	Int32 ColumnID::GetEndX () const noexcept {
	
		return GetStartX()+15;
	
	}
	
	
	Int32 ColumnID::GetEndZ () const noexcept {
	
		return GetStartZ()+15;
	
	}


}

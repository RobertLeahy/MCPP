#include <world/world.hpp>


namespace MCPP {


	bool BlockID::operator == (const BlockID & other) const noexcept {
	
		return (
			(X==other.X) &&
			(Y==other.Y) &&
			(Z==other.Z) &&
			(Dimension==other.Dimension)
		);
	
	}
	
	
	bool BlockID::operator != (const BlockID & other) const noexcept {
	
		return !(*this==other);
	
	}
	
	
	ColumnID BlockID::GetContaining () const noexcept {
	
		ColumnID retr{
			X/16,
			Z/16,
			Dimension
		};
		
		if (
			(X<0) &&
			((X%16)!=0)
		) --retr.X;
		if (
			(Z<0) &&
			((Z%16)!=0)
		) --retr.Z;
		
		return retr;
	
	}
	
	
	Word BlockID::GetOffset () const noexcept {
	
		Int32 x=X%16;
		if ((X<0) && (x!=0)) x=16-(x*-1);
		Int32 z=Z%16;
		if ((Z<0) && (z!=0)) z=16-(z*-1);
		
		return static_cast<Word>(x)+(static_cast<Word>(z)*16)+(static_cast<Word>(Y)*16*16);
	
	}
	
	
	bool BlockID::IsContainedBy (const ColumnID & column) const noexcept {
	
		if (column.Dimension!=Dimension) return false;
	
		Int32 x=X/16;
		if (X<0) --x;
		
		if (column.X!=x) return false;
		
		Int32 z=Z/16;
		if (Z<0) --z;
		
		return column.Z==z;
	
	}


}

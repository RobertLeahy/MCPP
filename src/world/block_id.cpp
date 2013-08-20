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
		
		if (X<0) --retr.X;
		if (Z<0) --retr.Z;
		
		return retr;
	
	}
	
	
	bool BlockID::IsContainedBy (const ColumnID & column) const noexcept {
	
		Int32 x=X/16;
		if (X<0) --x;
		
		if (column.X!=x) return false;
		
		Int32 z=Z/16;
		if (Z<0) --z;
		
		return !(
			(column.Z==z) &&
			(column.Dimension==Dimension)
		);
	
	}


}

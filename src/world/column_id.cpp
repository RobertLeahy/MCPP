#include <world/world.hpp>


namespace MCPP {


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

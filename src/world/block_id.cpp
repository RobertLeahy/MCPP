#include <world/world.hpp>


namespace MCPP {


	bool BlockID::operator == (const BlockID & other) const noexcept {
	
		return (other.X==X) && (other.Y==Y) && (other.Z==Z) && (other.Dimension==Dimension);
	
	}
	
	
	bool BlockID::operator != (const BlockID & other) const noexcept {
	
		return !(*this==other);
	
	}


	bool BlockID::ContainedBy (const ColumnID & column) const noexcept {
	
		return ColumnID::Make(*this)==column;
	
	}
	
	
	ColumnID BlockID::ContainedBy () const noexcept {
	
		return ColumnID::Make(*this);
	
	}


}

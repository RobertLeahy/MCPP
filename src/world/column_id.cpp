#include <world/world.hpp>


namespace MCPP {


	ColumnID ColumnID::Make (BlockID block) noexcept {
	
		return ColumnID{
			block.X/16,
			block.Z/16,
			block.Dimension
		};
	
	}
	
	
	bool ColumnID::operator == (const ColumnID & other) const noexcept {
	
		return (other.X==X) && (other.Z==Z) && (other.Dimension==Dimension);
	
	}
	
	
	bool ColumnID::operator != (const ColumnID & other) const noexcept {
	
		return !(other==*this);
	
	}
	
	
	bool ColumnID::Contains (const BlockID & block) const noexcept {
	
		return Make(block)==*this;
	
	}


}

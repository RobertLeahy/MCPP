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
	
	
	static inline Word column_coord (Int32 coord) noexcept {
	
		if (coord==0) return 0;
		
		Word mod=static_cast<Word>(coord%16);
		
		return (coord<0) ? 16-mod : mod;
	
	}
	
	
	Word BlockID::Offset () const noexcept {
	
		return static_cast<Word>(
			(Y*16*16)+
			(column_coord(Z)*16)+
			column_coord(X)
		);
	
	}


}

#include <world/world.hpp>


namespace MCPP {


	WorldLockRequest::WorldLockRequest () noexcept : world(true) {	}
	
	
	WorldLockRequest::WorldLockRequest (BlockID block) : world(false), blocks(1) {
	
		blocks.Add(block);
	
	}
	
	
	WorldLockRequest::WorldLockRequest (ColumnID column) : world(false), columns(1) {
	
		columns.Add(column);
	
	}
	
	
	WorldLockRequest::WorldLockRequest (Vector<BlockID> blocks) noexcept : world(false), blocks(std::move(blocks)) {	}
	
	
	WorldLockRequest::WorldLockRequest (Vector<ColumnID> columns) noexcept : world(false), columns(std::move(columns)) {	}
	
	
	bool WorldLockRequest::IsEmpty () const noexcept {
	
		return !world && (blocks.Count()==0) && (columns.Count()==0);
	
	}
	
	
	bool WorldLockRequest::DoesContendWith (const WorldLockRequest & other) const noexcept {
	
		//	If either request requests
		//	the world lock, the only way
		//	the locks can not contend
		//	is if the other is empty
		
		if (world) return !other.IsEmpty();
		
		if (other.world) return !IsEmpty();
		
		//	If either lock requests a lock
		//	on the same column, they contend
		for (const auto & a : columns)
		for (const auto & b : other.columns)
		if (a==b) return true;
		
		//	If either lock requests a lock
		//	on the same block, they contend
		for (const auto & a : blocks)
		for (const auto & b : other.blocks)
		if (a==b) return true;
		
		//	If either lock requests a lock
		//	on a column which contains a block
		//	on which the other lock requests
		//	a lock, they contend
		
		for (const auto & a : blocks) {
		
			auto b=a.GetContaining();
			
			for (const auto & c : other.columns)
			if (b==c) return true;
		
		}
		
		for (const auto & a : other.blocks) {
		
			auto b=a.GetContaining();
			
			for (const auto & c : columns)
			if (b==c) return true;
		
		}
		
		//	Everything has been checked, the
		//	locks do not contend!
		return false;
	
	}
	
	
	WorldLockRequest & WorldLockRequest::Add (BlockID block) {
	
		blocks.Add(block);
		
		return *this;
	
	}
	
	
	WorldLockRequest & WorldLockRequest::Add (ColumnID column) {
	
		columns.Add(column);
		
		return *this;
	
	}


}

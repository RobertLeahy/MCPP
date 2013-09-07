#include <world/world.hpp>


namespace MCPP {


	void WorldContainer::generate (ColumnContainer & column) {
	
		auto id=column.ID();
	
		//	Get the appropriate world generator
		const WorldGenerator & generator=get_generator(id.Dimension);
		
		//	Calculate the start and end
		//	X and Z coordinates so they
		//	don't have to be recomputed
		//	later
		Int32 start_x=id.GetStartX();
		Int32 end_x=id.GetEndX();
		Int32 start_z=id.GetStartZ();
		Int32 end_z=id.GetEndZ();
		
		//	This block will be passed through
		//	to the generator
		BlockID block{
			0,
			0,
			0,
			id.Dimension
		};
		
		//	Keeps track of the current position
		//	in the multi-dimensional arrays that
		//	we're populating
		Word offset;
		
		//	Iterate for each block in the column
		//	and set it using the block generator
		for (
			offset=0,block.Y=0;
			;
			++block.Y
		) {
		
			for (
				block.Z=start_z;
				block.Z<=end_z;
				++block.Z
			)
			for (
				block.X=start_x;
				block.X<=end_x;
				++block.X
			) {
			
				//	Invoke the generator
				column.Blocks[offset++]=generator(block);
			
			}
		
			//	Break condition (can't be
			//	checked in the for loop due
			//	to limited range of byte
			//	type)
			if (block.Y==255) break;
		
		}
		
		//	Iterate for each column and set it
		//	using the biome generator
		offset=0;
		for (
			Int32 x=start_x;
			x<=end_x;
			++x
		) for (
			Int32 z=start_z;
			z<=end_z;
			++z
		) column.Biomes[offset++]=generator(
			x,
			z,
			id.Dimension
		);
	
	}


}

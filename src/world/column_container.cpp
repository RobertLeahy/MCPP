#include <world/world.hpp>
#include <limits>
#include <cstring>


namespace MCPP {


	Vector<Byte> ColumnContainer::ToChunkData (const ColumnID & id) const {
	
		//	First we need to convert the sane
		//	in memory format to the insane
		//	Mojang format
		
		Word size=(
			//	One byte per block
			//	for block type
			(16*16*16*16)+
			//	Half a byte per block
			//	for metadata
			((16*16*16*16)/2)+
			//	Half a byte per block
			//	for light
			((16*16*16*16)/2)+
			//	One byte per column for
			//	biome information
			(16*16)
		);
		
		//	Do we have to send "add"?
		bool add=false;
		for (Word i=0;i<(16*16*16*16);++i) {
		
			if (Blocks[i].GetType()>std::numeric_limits<Byte>::max()) {
			
				add=true;
				
				break;
			
			}
		
		}
		
		//	If we have to send add it's
		//	one half byte per block
		if (add) size+=(16*16*16*16)/2;
		
		//	If we have to send skylight it's
		//	one half byte per block
		bool skylight=HasSkylight(id.Dimension);
		if (skylight) size+=(16*16*16*16)/2;
		
		//	Allocate enough space to hold
		//	the column in Mojang format
		Vector<Byte> column(size);
		column.SetCount(size);
		
		Word offset=0;
		
		//	Loop for block types
		for (const auto & b : Blocks) column[offset++]=static_cast<Byte>(b.GetType());
		
		//	Loop for block metadata
		bool even=true;
		for (const auto & b : Blocks) {
		
			if (even) {
			
				column[offset]=b.GetMetadata()<<4;
				even=false;
				
			} else {
			
				column[offset++]|=b.GetMetadata();
				even=true;
			
			}
			
		}
		
		//	Loop for light
		even=true;
		for (const auto & b : Blocks) {
		
			if (even) {
			
				column[offset]=b.GetLight()<<4;
				even=false;
			
			} else {
			
				column[offset++]|=b.GetLight();
				even=true;
			
			}
		
		}
		
		//	Loop for skylight if applicable
		if (skylight) {
		
			even=true;
			
			for (const auto & b : Blocks) {
			
				if (even) {
				
					column[offset]=b.GetSkylight()<<4;
					even=false;
				
				} else {
				
					column[offset++]|=b.GetSkylight();
					even=true;
				
				}
			
			}
			
		}
		
		//	Loop for "add" if applicable
		if (add) {
		
			even=true;
			
			for (const auto & b : Blocks) {
			
				auto type=b.GetType();
				Byte val=(type>std::numeric_limits<Byte>::max()) ? static_cast<Byte>(type>>8) : 0;
			
				if (even) {
				
					column[offset]=val<<4;
					even=false;
				
				} else {
				
					column[offset++]|=val;
					even=true;
				
				}
			
			}
		
		}
		
		//	Copy biomes
		memcpy(
			&column[offset],
			Biomes,
			sizeof(Biomes)
		);
		
		//	Prepare the final buffer
		size=(
			//	X-coordinate of this chunk
			sizeof(Int32)+
			//	Z-coordinate of this chunk
			sizeof(Int32)+
			//	Group-up continuous
			sizeof(bool)+
			//	Primary bit mask
			sizeof(UInt16)+
			//	"Add" bit mask
			sizeof(UInt16)+
			//	Size of compressed data
			sizeof(Int32)+
			//	Largest possible size for
			//	compressed column data
			DeflateBound(
				column.Count()
			)
		);
		
		//	Allocate sufficient memory
		Vector<Byte> buffer(size);
		
		PacketHelper<Int32>::ToBytes(
			id.X,
			buffer
		);
		PacketHelper<Int32>::ToBytes(
			id.Z,
			buffer
		);
		PacketHelper<bool>::ToBytes(
			true,
			buffer
		);
		PacketHelper<UInt16>::ToBytes(
			std::numeric_limits<UInt16>::max(),
			buffer
		);
		PacketHelper<UInt16>::ToBytes(
			std::numeric_limits<UInt16>::max(),
			buffer
		);
		
		//	Push end point of buffer past
		//	where the size will go
		//	(as we do not yet know the size)
		Word size_loc=buffer.Count();
		buffer.SetCount(size_loc+sizeof(Int32));
		
		//	Compress
		Deflate(
			column.begin(),
			column.end(),
			&buffer
		);
		
		//	Get compressed size
		SafeWord compressed_size(buffer.Count()-size_loc-sizeof(Int32));
		
		Word final_size=buffer.Count();
		
		//	Insert compressed size
		buffer.SetCount(size_loc);
		PacketHelper<Int32>::ToBytes(
			Int32(compressed_size),
			buffer
		);
		
		buffer.SetCount(final_size);
		
		return buffer;
	
	}


}

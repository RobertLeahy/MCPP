/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <functional>
#include <unordered_map>


namespace RLeahyLib {


	/**
	 *	Encapsulates all the information
	 *	which may uniquely identify a
	 *	chunk.
	 */
	class ChunkID {
	
	
		public:
		
		
			/**
			 *	The X-coordinate of this chunk.
			 *
			 *	This coordinate is in chunks, not
			 *	blocks.  Multiply by 16 to get
			 *	the block coordinate.
			 */
			Int32 X;
			/**
			 *	The Y-coordinate of this chunk.
			 *
			 *	This coordinate is in chunks, not
			 *	blocks.  Multiply by 16 to get the
			 *	block coordinate.
			 */
			Byte Y;
			/**
			 *	The Z-coordinate of this chunk.
			 *
			 *	This coordinate is in chunks, not
			 *	blocks.  Multiply by 16 to get the
			 *	block coordinate.
			 */
			Int32 Z;
			/**
			 *	The dimension that this chunk is
			 *	in.
			 */
			SByte Dimension;
	
	
	};
	
	
}


/**
 *	\cond
 */


namespace std {


	template <>
	class hash<ChunkID> {
	
	
		public:
		
		
			size_t operator () (const ChunkID & input) const noexcept {
			
				return	static_cast<UInt32>(input.X)+
						input.Y+
						static_cast<UInt32>(input.Z)+
						static_cast<Byte>(input.Dimension);
			
			}
	
	
	};


};


/**
 *	\endcond
 */
 
 
namespace MCPP {


	/**
	 *	\cond
	 */
	 
	 
	class ChunkContainer {
	
	
		public:
		
		
			//	The chunk itself
			Nullable<Chunk> Storage;
			//	List of clients who
			//	have or want this
			//	chunk
			std::unordered_map<
				const Client *,
				SmartPointer<Client>
			> Clients;
			//	Lock
			Mutex Lock;
	
	
	};
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	Provides extensible facilities
	 *	for generating the Minecraft
	 *	world.
	 */
	class WorldGenerator {
	
	
		public:
		
		
			/**
			 *	Gets a fully decorated chunk at
			 *	a given X- and Z-coordinate.
			 *
			 *	This chunk may be either generated
			 *	or loaded 
			 *
			 *	\param [in] x
			 *		The X-coordinate of the chunk
			 *		to fetch.
			 *	\param [in] y
			 *		The Y-coordinate of the chunk
			 *		to fetch.
			 */
			Chunk GetChunk (Int32 x, Int32 z);
	
	
	};
	
	
	/**
	 *	A container which encapsulates the
	 *	Minecraft world.
	 */
	class World {
	
	
		private:
		
		
			//	Chunk storage
			std::unordered_map<
				const ChunkID,
				ChunkContainer
			> chunks;
			//	Player mapping
			//
			//	Tells us which players
			//	have which chunks loaded.
			std::unordered_map<
				const Client *,
				Vector<ChunkID>
			> map;
			//	Synchronizes access
			RWLock lock;
		
		
		public:
		
		
			/**
			 *	Adds a chunk to a client.
			 *
			 *	The chunk will be loaded or
			 *	generated as necessary and
			 *	then send to the client.
			 *
			 *	\param [in] client
			 *		The client to add the
			 *		chunk to.
			 *	\param [in] coord
			 *		The chunk to add to
			 *		\em client.
			 */
			void AddChunk (SmartPointer<Client> client, ChunkID coord);
	
	
	};


}

/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <functional>
#include <unordered_map>
#include <utility>


namespace MCPP {


	/**
	 *	Represents a single block in the
	 *	Minecraft world.
	 */
	class Block {
	
	
		private:
		
		
			UInt32 flags;
			UInt16 type;
			//	Skylight is the high 4 bits,
			//	metadata is the low 4 bits
			Byte skylightmetadata;
			Byte light;
	
	
		public:
		
		
			/**
			 *	Creates a new air block.
			 */
			inline Block () noexcept : flags(0), type(0), skylightmetadata(0), light(0) {	}
			
			
			/**
			 *	Creates a new block of the specified
			 *	type.
			 *
			 *	\param [in] type
			 *		The type of block to create.
			 */
			inline Block (UInt16 type) noexcept : flags(0), type(type), skylightmetadata(0), light(0) {	}
		
		
			/**
			 *	Tests to see if a certain flag is set
			 *	on this block.
			 *
			 *	\param [in] num
			 *		The zero-relative index of the flag
			 *		to test.
			 *
			 *	\return
			 *		\em true if the flag-in-question is
			 *		set, \em false otherwise.
			 */
			inline bool TestFlag (Word num) const noexcept {
			
				if (num>31) return false;
				
				return (flags&(static_cast<UInt32>(1)<<static_cast<UInt32>(num)))!=0;
			
			}
			
			
			/**
			 *	Sets a certain flag on this block.
			 *
			 *	\param [in] num
			 *		The zero-relative index of the flag
			 *		to set.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			inline Block & SetFlag (Word num) noexcept {
			
				if (num<=31) flags|=static_cast<UInt32>(1)<<static_cast<UInt32>(num);
				
				return *this;
			
			}
			
			
			/**
			 *	Clears a certain flag on this block.
			 *
			 *	\param [in] num
			 *		The zero-relative index of the flag
			 *		to clear.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			inline Block & UnsetFlag (Word num) noexcept {
			
				if (num<=31) flags&=~(static_cast<UInt32>(1)<<static_cast<UInt32>(num));
				
				return *this;
			
			}
			
			
			/**
			 *	Retrieves this block's light value.
			 *
			 *	\return
			 *		This block's light value.
			 */
			inline Byte GetLight () const noexcept {
			
				return light;
			
			}
			
			
			/**
			 *	Sets this block's light value.
			 *
			 *	\param [in] light
			 *		The light value to associate with
			 *		this block.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			inline Block & SetLight (Byte light) noexcept {
			
				if (light<=15) this->light=light;
				
				return *this;
			
			}
			
			
			/**
			 *	Retrieves this block's metadata.
			 *
			 *	\return
			 *		This block's metadata.
			 */
			inline Byte GetMetadata () const noexcept {
			
				return skylightmetadata&15;
			
			}
			
			
			/**
			 *	Sets this block's metadata.
			 *
			 *	\param [in] metadata
			 *		The metadata to associate with
			 *		this block.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			inline Block & SetMetadata (Byte metadata) noexcept {
			
				if (metadata<=15) skylightmetadata=(skylightmetadata&240)|metadata;
				
				return *this;
			
			}
			
			
			/**
			 *	Retrieves this block's skylight value.
			 *
			 *	\return
			 *		This block's skylight value.
			 */
			inline Byte GetSkylight () const noexcept {
			
				return skylightmetadata>>4;
			
			}
			
			
			/**
			 *	Sets this block's skylight value.
			 *
			 *	\param [in] skylight
			 *		The skylight value to associate
			 *		with this block.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			inline Block & SetSkylight (Byte skylight) noexcept {
			
				if (skylight<=15) skylightmetadata=(skylightmetadata&15)|(skylight<<4);
				
				return *this;
			
			}
			
			
			/**
			 *	Retrieves the type of this block.
			 *
			 *	\return
			 *		The type of this block.
			 */
			inline UInt16 GetType () const noexcept {
			
				return type;
			
			}
			
			
			/**
			 *	Sets this block's type.
			 *
			 *	\param [in] type
			 *		The ype of this block.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			inline Block & SetType (UInt16 type) noexcept {
			
				if (type<=4095) this->type=type;
				
				return *this;
			
			}
			
	
	};
	
	
	/**
	 *	\cond
	 */
	 
	 
	class BlockID;
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Uniquely identifies a column in the
	 *	Minecraft world.
	 *
	 *	Column coordinates are measured in
	 *	columns, not in blocks.  The column
	 *	0,0 is defined as the column which
	 *	contains the block 0,0.
	 */
	class ColumnID {
	
	
		public:
		
		
			/**
			 *	The x-coordinate of the
			 *	column-in-question.
			 */
			Int32 X;
			/**
			 *	The z-coordinate of the
			 *	column-in-question.
			 */
			Int32 Z;
			/**
			 *	The dimension in which the
			 *	referenced column resides.
			 */
			SByte Dimension;
			
			
			bool operator == (const ColumnID & other) const noexcept;
			bool operator != (const ColumnID & other) const noexcept;
			
			
			/**
			 *	Checks to see if this column
			 *	contains a particular block.
			 *
			 *	\param [in] block
			 *		The block to check.
			 *
			 *	\return
			 *		\em true if this column
			 *		contains \em block,
			 *		\em false otherwise.
			 */
			bool DoesContain (const BlockID & block) const noexcept;
			
			
			/**
			 *	Retrieves the lowest block
			 *	x-coordinate which is still
			 *	contained within this column.
			 *
			 *	\return
			 *		An x-coordinate measured
			 *		in blocks.
			 */
			Int32 GetStartX () const noexcept;
			/**
			 *	Retrieves the lowest block
			 *	z-coordinate which is still
			 *	contained within this column.
			 *
			 *	\return
			 *		A z-coordinate measured
			 *		in blocks.
			 */
			Int32 GetStartZ () const noexcept;
			/**
			 *	Retrieves the highest block
			 *	x-coordinate which is still
			 *	contained within this column.
			 *
			 *	\return
			 *		An x-coordinate measured
			 *		in blocks.
			 */
			Int32 GetEndX () const noexcept;
			/**
			 *	Retrieves the highest block
			 *	z-coordinate which is still
			 *	contained within this column.
			 *
			 *	\return
			 *		A z-coordinate measured
			 *		in blocks.
			 */
			Int32 GetEndZ () const noexcept;
	
	
	};
	
	
	/**
	 *	Uniquely identifies a block in the
	 *	Minecraft world.
	 */
	class BlockID {
	
	
		public:
		
		
			/**
			 *	The x-coordinate of the
			 *	block-in-question.
			 */
			Int32 X;
			/**
			 *	The y-coordinate of the
			 *	block-in-question.
			 */
			Byte Y;
			/**
			 *	The z-coordinate of the
			 *	block-in-question.
			 */
			Int32 Z;
			/**
			 *	The dimension in which the
			 *	referenced block resides.
			 */
			SByte Dimension;
			
			
			bool operator == (const BlockID & other) const noexcept;
			bool operator != (const BlockID & other) const noexcept;
			
			
			/**
			 *	Retrieves the ID of the column
			 *	which contains this block.
			 *
			 *	\return
			 *		The ID which uniquely identifies
			 *		the column which contains this
			 *		block.
			 */
			ColumnID GetContaining () const noexcept;
			
			
			/**
			 *	Checks to see if a certain column
			 *	contains this block.
			 *
			 *	\param [in] column
			 *		The ID of the column to check.
			 *
			 *	\return
			 *		\em true if this block is contained
			 *		by \em column, \em false otherwise.
			 */
			bool IsContainedBy (const ColumnID & column) const noexcept;
	
	
	};
	
	
	/**
	 *	Checks to see if a given dimension
	 *	causes skylight values to be sent
	 *	to the client.
	 *
	 *	\param [in] dimension
	 *		The dimension of interest.
	 *
	 *	\return
	 *		\em true if \em dimension causes
	 *		skylight values to be sent to
	 *		the client, \em false otherwise.
	 */
	bool HasSkylight (SByte dimension) noexcept;
	/**
	 *	Sets the internal value used by
	 *	HasSkylight for a certain dimension.
	 *
	 *	Not thread safe.
	 *
	 *	\param [in] dimension
	 *		The dimension of interest.
	 *	\param [in] has_skylight
	 *		\em true if skylight values should
	 *		be sent to the client for
	 *		\em dimension, \em false otherwise.
	 */
	void SetHasSkylight (SByte dimension, bool has_skylight) noexcept;
	
	
	/**
	 *	\cond
	 */
	
	
	class ColumnContainer {
	
		
		public:
		
		
			Block Blocks [16*16*16*16];
			Byte Biomes [16*16];
			
			
			//	Retrieves a 0x33 packet which represents
			//	the contained column
			Vector<Byte> ToChunkData (const ColumnID &) const;
		
	
	};
	
	
	/**
	 *	Represents a request for exclusive
	 *	access to certain elements of the
	 *	world.
	 */
	class WorldLockRequest {
	
	
		private:
		
		
			bool world;
			Vector<BlockID> blocks;
			Vector<ColumnID> columns;
			
			
		public:
		
		
			/**
			 *	Creates a new world lock
			 *	request which requests
			 *	exclusive access to the
			 *	entire world.
			 */
			WorldLockRequest () noexcept;
			/**
			 *	Creates a new world lock
			 *	request which requests
			 *	exclusive access to a certain
			 *	block.
			 *
			 *	\param [in] block
			 *		The block to which exclusive
			 *		access is desired.
			 */
			WorldLockRequest (BlockID block);
			/**
			 *	Creates a new world lock
			 *	request which requests
			 *	exclusive access to a certain
			 *	column.
			 *
			 *	\param [in] column
			 *		The column to which exclusive
			 *		access is desired.
			 */
			WorldLockRequest (ColumnID column);
			/**
			 *	Creates a new world lock request
			 *	which requests exclusive access
			 *	to certain blocks.
			 *
			 *	\param [in] blocks
			 *		The blocks to which exclusive
			 *		access is desired.
			 */
			WorldLockRequest (Vector<BlockID> blocks) noexcept;
			/**
			 *	Creates a new world lock request
			 *	which requests exclusive access to
			 *	certain columns.
			 *
			 *	\param [in] columns
			 *		The columns to which exclusive
			 *		access is desired.
			 */
			WorldLockRequest (Vector<ColumnID> columns) noexcept;
			
			
			/**
			 *	Determines whether or not this
			 *	lock request is empty, i.e.\ whether
			 *	or not it does not request a lock
			 *	on anything.
			 *
			 *	\return
			 *		\em true if this lock does not
			 *		request exclusive access to any
			 *		elements of the world, \em false
			 *		otherwise.
			 */
			bool IsEmpty () const noexcept;
			/**
			 *	Determines whether or not this lock
			 *	request contends with some other
			 *	lock request, i.e.\ whether or not
			 *	both of them can be held simultaneously.
			 *
			 *	\return
			 *		\em false if the locks contend,
			 *		i.e.\ may not be held simultaneously.
			 *		\em true otherwise.
			 */
			bool DoesContendWith (const WorldLockRequest & other) const noexcept;
			
			
			/**
			 *	Adds a block to the world lock
			 *	request.
			 *
			 *	\param [in] block
			 *		The block to add.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			WorldLockRequest & Add (BlockID block);
			/**
			 *	Adds a column to the world lock
			 *	request.
			 *
			 *	\param [in] column
			 *		The column to add.
			 *
			 *	\return
			 *		A reference to this object.
			 */
			WorldLockRequest & Add (ColumnID column);
	
	
	};
	
	
	/**
	 *	\cond
	 */
	
	
	class WorldLockInfo {
	
	
		private:
		
		
			Mutex lock;
			CondVar wait;
			bool sync;
			bool wake;
			std::function<void (const void *)> callback;
	
	
		public:
		
		
			WorldLockRequest Request;
			
			
			WorldLockInfo () = delete;
			WorldLockInfo (WorldLockRequest) noexcept;
			WorldLockInfo (std::function<void (const void *)>, WorldLockRequest) noexcept;
			
			
			void Asynchronous (std::function<void (const void *)>) noexcept;
			void Synchronous () noexcept;
			void Wait () noexcept;
			void Wake (ThreadPool &);
	
	
	};
	
	
	/**
	 *	\endcond
	 */
	 
	 
	class WorldLock {
	
	
		private:
		
		
			ThreadPool & pool;
		
		
			Mutex lock;
			Vector<SmartPointer<WorldLockInfo>> held;
			Vector<SmartPointer<WorldLockInfo>> pending;
			
			
			inline void acquire (SmartPointer<WorldLockInfo>);
			inline void release (const void *);
			
			
		public:
		
		
			WorldLock () = delete;
			WorldLock (ThreadPool &) noexcept;
			
			
			template <typename T, typename... Args>
			void Enqueue (WorldLockRequest request, T && callback, Args &&... args) {
			
				acquire(
					SmartPointer<WorldLockInfo>::Make(
						std::move(request),
						std::bind(
							std::forward<T>(callback),
							std::placeholders::_1,
							std::forward<Args>(args)...
						)
					)
				);
			
			}
			const void * Acquire (WorldLockRequest request);
			void Release (const void * handle);
	
	
	};
	
	
	/**
	 *	Provides an interface through which a
	 *	world generator may be accessed.
	 *
	 *	A "world generator" is a pair of a
	 *	block generator, which places individual
	 *	blocks when given a block ID, and a
	 *	biome generator, which returns a biome
	 *	value when given an X,Z coordinate
	 *	pair.
	 */
	class WorldGenerator {
	
	
		public:
		
		
			/**
			 *	The type of a block generator callback.
			 */
			typedef std::function<Block (const BlockID &)> Generator;
			/**
			 *	The type of a biome generator callback.
			 */
			typedef std::function<Byte (Int32, Int32, SByte)> BiomeGenerator;
	
	
		private:
		
		
			Generator generator;
			BiomeGenerator biome_generator;
		
		
		public:
		
		
			/**
			 *	Creates a new WorldGenerator.
			 *
			 *	\param [in] generator
			 *		The block generator.
			 *	\param [in] biome_generator
			 *		The biome generator.
			 */
			WorldGenerator (Generator generator, BiomeGenerator biome_generator) noexcept;
			
			
			WorldGenerator () = default;
			WorldGenerator (WorldGenerator &&) = default;
			WorldGenerator (const WorldGenerator &) = default;
			WorldGenerator & operator = (WorldGenerator &&) = default;
			WorldGenerator & operator = (const WorldGenerator &) = default;
			
			
			/**
			 *	Determines whether this object
			 *	contains a valid block
			 *	generator/biome generator pair.
			 *
			 *	\return
			 *		\em true if this object contains
			 *		two invocable targets, \em false
			 *		otherwise.
			 */
			operator bool () const noexcept;
			/**
			 *	Invokes the block generator.
			 *
			 *	\param [in] id
			 *		The ID of the block which shall
			 *		be generated and returned.
			 *
			 *	\return
			 *		The block given by \em id.
			 */
			Block operator () (const BlockID & id) const;
			/**
			 *	Invokes the biome generator.
			 *
			 *	\param [in] x
			 *		The x-coordinate of the
			 *		column-in-question.
			 *	\param [in] z
			 *		The z-coordinate of the
			 *		column-in-question.
			 *	\param [in] dimension
			 *		The dimension in which the
			 *		column-in-question resides.
			 *
			 *	\return
			 *		A byte indicating the biome
			 *		of the column specified by
			 *		\em x, \em z.
			 */
			Byte operator () (Int32 x, Int32 z, SByte dimension) const;
	
	
	};
	
	
	/**
	 *	Contains and manages the Minecraft world
	 *	as a collection of columns.
	 */
	class WorldContainer {
	
	
		private:
		
		
			//	SETTINGS
			
			//	The world type
			String type;
		
		
			//	Contains loaded world generators
			std::unordered_map<
				Tuple<
					String,
					SByte
				>,
				WorldGenerator
			> generators;
			std::unordered_map<
				SByte,
				WorldGenerator
			> default_generators;
		
		
			//	PRIVATE METHODS
			
			//	Generates a column
			void generate (ColumnContainer &, const ColumnID &) const;
			const WorldGenerator & get_generator (SByte) const;
		
		
		public:
		
		
			void Add (WorldGenerator generator, SByte dimension);
			void Add (WorldGenerator generator, String type, SByte dimension);
	
	
	};
	 

}

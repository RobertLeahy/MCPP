/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <client.hpp>
#include <event.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <thread_pool.hpp>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <new>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <type_traits>


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
	 
	 
	static_assert(
		sizeof(Block)==(sizeof(UInt32)+sizeof(UInt16)+(sizeof(Byte)*2)),
		"Block layout incorrect"
	);
	 
	 
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
			 *	Gets the column which contains a
			 *	certain point.
			 *
			 *	\param [in] x
			 *		The x coordinate of the point.
			 *	\param [in] z
			 *		The z coordinate of the point.
			 *	\param [in] dimension
			 *		The dimension of the point.
			 *
			 *	\return
			 *		The ID of the column which contains
			 *		the point-in-question.
			 */
			static ColumnID GetContaining (Double x, Double z, SByte dimension) noexcept;
		
		
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
	
	
}


/**
 *	\cond
 */


namespace std {


	template <>
	struct hash<MCPP::ColumnID> {
	
	
		public:
		
		
			size_t operator () (const MCPP::ColumnID & subject) const noexcept {
			
				union {
					UInt32 u_x;
					Int32 x;
				};
				
				x=subject.X;
				
				union {
					UInt32 u_z;
					Int32 z;
				};
				
				z=subject.Z;
				
				union {
					Byte u_d;
					SByte d;
				};
				
				d=subject.Dimension;
				
				size_t retr=23;
				retr*=31;
				retr+=u_x;
				retr*=31;
				retr+=u_z;
				retr*=31;
				retr+=u_d;
				
				return retr;
			
			}
	
	
	};


}


/**
 *	\endcond
 */


namespace MCPP {
	
	
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
			 *	Retrieves the block's offset within
			 *	its containing column.
			 *
			 *	\return
			 *		The rank of this block within
			 *		the contiguous memory of its
			 *		column.
			 */
			Word GetOffset () const noexcept;
			
			
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
	
	
}


/**
 *	\cond
 */
 
 
namespace std {


	template <>
	struct hash<MCPP::BlockID> {
	
	
		public:
		
		
			size_t operator () (const MCPP::BlockID & subject) const noexcept {
			
				union {
					UInt32 u_x;
					Int32 x;
				};
				
				x=subject.X;
				
				union {
					UInt32 u_z;
					Int32 z;
				};
				
				z=subject.Z;
				
				union {
					Byte u_d;
					SByte d;
				};
				
				d=subject.Dimension;
				
				size_t retr=23;
				retr*=31;
				retr+=u_x;
				retr*=31;
				retr+=subject.Y;
				retr*=31;
				retr+=u_z;
				retr*=31;
				retr+=u_d;
				
				return retr;
			
			}
	
	
	};


}


/**
 *	\endcond
 */
 
 
namespace MCPP {


	/**
	 *	An enumeration of all the biomes
	 *	supported by vanilla Minecraft.
	 */
	enum class Biome : Byte {
	
		Ocean=0,
		Plains=1,
		Desert=2,
		ExtremeHills=3,
		Forest=4,
		Taiga=5,
		Swampland=6,
		River=7,
		Hell=8,
		Sky=9,
		FrozenOcean=10,
		FrozenRiver=11,
		IcePlains=12,
		IceMountains=13,
		MushroomIsland=14,
		MushroomIslandShore=15,
		Beach=16,
		DesertHills=17,
		ForestHills=18,
		TaigaHills=19,
		ExtremeHillsEdge=20,
		Jungle=21,
		JungleHills=22
	
	};
	
	
	/**
	 *	Checks to see if a byte represents a
	 *	valid biome.
	 *
	 *	\param [in] biome
	 *		The byte to check.
	 *
	 *	\return
	 *		\em true if \em biome represents a
	 *		valid biome and may be safely cast
	 *		to the Biome enumeration type.
	 */
	inline bool IsValidBiome (Byte biome) noexcept {
	
		return biome<=22;
	
	}
	
	
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
	 
	 
	enum class ColumnState : Word {
	
		Loading=0,
		Generating=1,
		Generated=2,
		Populating=3,
		Populated=4
	
	};
	
	
	class ColumnContainer {
	
		
		public:
		
		
			typedef Packets::Play::Clientbound::ChunkData PacketType;
		
		
			ColumnContainer () = delete;
			//	Creates a ColumnContainer with a given id
			//	and in the loading state
			ColumnContainer (ColumnID) noexcept;
		
		
			//	The layout of the proceeding
			//	three fields is identical to
			//	the layout in which they will
			//	be saved to the backing store,
			//	this removes the need for any
			//	sort of copying
			Block Blocks [16*16*16*16];
			Biome Biomes [16*16];
			bool Populated;
			
			
			//	The size of the memory region
			//	within each column container
			//	which should be saved and
			//	loaded
			static constexpr Word Size=sizeof(Populated)+sizeof(Blocks)+sizeof(Biomes);
			
			
			//	Retrieves the ID of this column
			ColumnID ID () const noexcept;
			//	Retrieves a 0x33 packet which represents
			//	the contained column
			PacketType ToChunkData () const;
			//	Retrieves a 0x33 packet which unloads
			//	the contained column from a client
			PacketType GetUnload () const;
			//	Checks the column's state.
			//
			//	If the column is in the required state, or
			//	a state more advanced than that state,
			//	returns true immediately.
			//
			//	If the column is not in the required state,
			//	but is being processed, blocks until the
			//	required amount of processing is done, and
			//	then returns true.
			//
			//	If the column is not in the required state,
			//	and is not being processed, returns false
			//	at once -- the caller MUST do the required
			//	processing on the column.
			bool WaitUntil (ColumnState) noexcept;
			//	Sets the columns state.
			//
			//	Wakes all pending threads and fires all
			//	appropriate asynchronous callbacks using
			//	the provided thread pool.
			//
			//	If the dirty flag is set to false, the
			//	column will not consider itself to be
			//	changed by this state change (for example
			//	when loading a column its logical state
			//	does not change).
			//
			//	Returns true if processing is finished,
			//	false if processing should continue.
			bool SetState (ColumnState, bool, ThreadPool &) noexcept;
			//	Checks the column's state.
			//
			//	If the column is in the required state, or
			//	a state more advanced than that state, calls
			//	the provided callback at once.
			//
			//	If the column is not in the required state
			//	the callback is enqueued and will be called
			//	when the column transitions into the appropriate
			//	state.
			//
			//	If the column is not being processed, false
			//	is returned which means that the caller MUST
			//	do the required processing on the column.
			bool InvokeWhen (ColumnState, std::function<void ()>);
			//	Checks the column's state.
			//
			//	If the column will enter the requisite state
			//	at some point in the future, returns true,
			//	otherwise returns false and the caller should
			//	perform that required processing immediately.
			bool Check (ColumnState) noexcept;
			//	Retrieves the column's current state
			ColumnState GetState () const noexcept;
			//	Sends the column to all currently
			//	added clients
			//
			//	Should be called once the column becomes
			//	populated
			void Send ();
			//	Adds a player to this column.
			void AddPlayer (SmartPointer<Client>);
			//	Removes a player from this column.
			//
			//	Boolean indicates whether or not
			//	column should be unloaded on the
			//	client.
			void RemovePlayer (SmartPointer<Client>, bool);
			//	Expresses "interest" in the column.
			//
			//	So longer as interest has been expressed
			//	in a column it cannot be unloaded
			void Interested () noexcept;
			//	Ends "interest" in the column
			void EndInterest () noexcept;
			//	Determines whether the column can be unloaded
			//	at this time.
			//
			//	Only columns with no associated players and
			//	no interest can be unloaded.
			//
			//	Not thread safe.
			bool CanUnload () const noexcept;
			//	Sets a block within this column, sending
			//	the appropriate packet
			void SetBlock (BlockID, Block);
			//	Gets a block within this column
			Block GetBlock (BlockID) const noexcept;
			//	Acquires the column's internal lock
			void Acquire () const noexcept;
			//	Release the column's internal lock
			void Release () const noexcept;
			//	Determines whether or not the column
			//	is "dirty".
			//
			//	Not thread safe
			bool Dirty () const noexcept;
			//	Clears the "dirty" flag.
			void Clean () noexcept;
			//	Gets a string which represents
			//	the co-ordinates of this column
			String ToString () const;
			//	Gets a pointer through which this
			//	column may be loaded or saved
			void * Get () noexcept;
			
			
		private:
		
		
			//	The column's ID -- its X and Z
			//	coordinates and the dimension in
			//	which it resides
			ColumnID id;
			//	The column's current state
			//
			//	If this is not equal to target
			//	that means that processing is
			//	in progress
			std::atomic<Word> curr;
			//	Lock -- guards all non-public members
			//	except curr (which is atomic)
			mutable Mutex lock;
			//	Condition variable -- used to wait
			//	for changes in state (woken each
			//	time the state is set)
			CondVar wait;
			//	The column's current target state,
			//	if this is equal to curr it means
			//	that the column is not being processed
			ColumnState target;
			//	Asynchronous callbacks waiting on a
			//	change of state
			Vector<Tuple<ColumnState,std::function<void ()>>> pending;
			//	All clients who have or want this
			//	column
			std::unordered_set<SmartPointer<Client>> clients;
			//	The "interest" count.
			//
			//	So long as there is "interest" in
			//	a column, it will not be unloaded
			std::atomic<Word> interest;
			//	Whether column data has been sent
			//	to clients or not
			bool sent;
			//	Whether this column has been modified
			//	since it was last saved
			bool dirty;
		
	
	};
	
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Winvalid-offsetof"
	static_assert(
		(
			offsetof(
				ColumnContainer,
				Blocks
			)==0
		) &&
		(
			offsetof(
				ColumnContainer,
				Biomes
			)==(16*16*16*16*sizeof(Block))
		) &&
		(
			offsetof(
				ColumnContainer,
				Populated
			)==(
				(16*16*16*16*sizeof(Block))+(16*16*sizeof(Byte))
			)
		) &&
		(sizeof(bool)==sizeof(Byte)),
		"ColumnContainer layout incorrect"
	);
	#pragma GCC diagnostic pop
	
	
	/**
	 *	\endcond
	 */
	 
	 
	/**
	 *	The different strategies that
	 *	may be used to write to the
	 *	world.
	 */
	enum class BlockWriteStrategy {

		/**
		 *	The world's state is guaranteed
		 *	not to change after the handle
		 *	is acquired.  Other threads may
		 *	read from the world, but may not
		 *	write to it.
		 */
		Transactional,
		/**
		 *	Other threads may write to and
		 *	read from the world at any time,
		 *	but all operations are guaranteed
		 *	to be thread safe, however no
		 *	operations are guaranteed to be
		 *	atomic.
		 */
		Dirty

	};
	
	
	/**
	 *	The different strategies that a
	 *	WorldHandle may use for dealing with
	 *	columns that have not been loaded,
	 *	generated, populated, et cetera.
	 */
	enum class BlockAccessStrategy {
	
		/**
		 *	The WorldHandle shall load
		 *	columns that are not present,
		 *	but shall fail if loaded columns
		 *	are not populated.
		 */
		Load,
		/**
		 *	The WorldHandle shall load
		 *	columns that are not present,
		 *	but shall fail if loaded columns
		 *	are not generated.
		 *
		 *	In this mode the WorldHandle will
		 *	read blocks from columns that have
		 *	not been populated.
		 */
		LoadGenerated,
		/**
		 *	The WorldHandle shall load and
		 *	generate columns that are not
		 *	present.
		 *
		 *	In this mode the WorldHandle will
		 *	read blocks from columns that have
		 *	not been populated.
		 */
		Generate,
		/**
		 *	The WorldHandle shall not load
		 *	or generate columns that are not
		 *	present or generated.
		 *
		 *	In this mode the WorldHandle will
		 *	read blocks from columns that have
		 *	not been populated.
		 */
		Generated,
		/**
		 *	The WorldHandle shall load,
		 *	generate, and populate columns
		 *	that are not present.
		 */
		Populate,
		/**
		 *	The WorldHandle shall not load,
		 *	generate, or populate columns
		 *	that are not present, generated,
		 *	or populated.
		 */
		Populated
	
	};
	
	
	/**
	 *	\cond
	 */
	 
	 
	class World;
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	A handle which allows blocks to be
	 *	written or read according to a certain
	 *	strategy.
	 */
	class WorldHandle {
	
	
		friend class World;
	
	
		private:
		
		
			//	The strategy that this handle
			//	uses to write to the world
			BlockWriteStrategy write;
			//	The strategy that this handle
			//	uses to access the world
			BlockAccessStrategy access;
			//	A pointer to the world object
			//	that this handle reads from and
			//	writes to.  If it is null it
			//	means that this object has been
			//	moved
			mutable World * world;
			//	True if this handle holds the
			//	world lock, and write operations
			//	may proceed seamlessly
			mutable bool locked;
			//	A pointer to the last column
			//	that this handle accessed, for
			//	caching reasons
			mutable ColumnContainer * cache;
			//	If greater than zero, this
			//	handle is being used for
			//	population.
			mutable Word populate;
			
			
			WorldHandle (World *, BlockWriteStrategy, BlockAccessStrategy);
			void BeginPopulate () const noexcept;
			void EndPopulate () const noexcept;
			
			
			inline void destroy () noexcept;
			inline ColumnContainer * get_column_impl (ColumnID) const;
			inline ColumnContainer * get_column (ColumnID, bool) const;
			inline bool set_impl (ColumnContainer *, BlockID, Block, bool) const;
			
			
		public:
		
		
			WorldHandle () = delete;
			WorldHandle (const WorldHandle &) = delete;
			WorldHandle & operator = (const WorldHandle &) = delete;
			
			
			WorldHandle (WorldHandle &&) noexcept;
			WorldHandle & operator = (WorldHandle &&) noexcept;
			
			
			~WorldHandle () noexcept;
			
			
			/**
			 *	Attempts to set a block.
			 *
			 *	\param [in] id
			 *		The location at which to set
			 *		the block.
			 *	\param [in] block
			 *		The block to set at the location
			 *		specified by \em id.
			 *	\param [in] force
			 *		If \em true events shall not have
			 *		the opportunity to block the setting
			 *		of the block.  Defaults to \em true.
			 *
			 *	\return
			 *		\em true if the block was set, \em false
			 *		otherwise.
			 */
			bool Set (BlockID id, Block block, bool force=true) const;
			/**
			 *	Attempts to retrieve a block.
			 *
			 *	\exception std::runtime_error
			 *		Thrown if the requested block
			 *		could not be retrieved.
			 *
			 *	\param [in] id
			 *		The location of the block to
			 *		retrieve.
			 *
			 *	\return
			 *		The block at the location specified
			 *		by \em id.
			 */
			Block Get (BlockID id) const;
			/**
			 *	Attempts to retrieve a block.
			 *
			 *	\param [in] id
			 *		The location of the block to
			 *		retrieve.
			 *	\param [in] no_throw
			 *		An object of type \em std::nothrow_t
			 *		which disambiguates this overload.
			 *
			 *	\return
			 *		The block at the location specified
			 *		by \em id if it could be retrieved,
			 *		\em null otherwise.
			 */
			Nullable<Block> Get (BlockID id, std::nothrow_t no_throw) const;
			
			
			/**
			 *	Determines whether this handle currently
			 *	has exclusive access to the world.
			 *
			 *	If the return value of this function is
			 *	\em false, and no other handles are held
			 *	by the current thread, attempting to
			 *	acquire a transactional handle, or attempting
			 *	to write through a different handle is
			 *	guaranteed not to deadlock.
			 *
			 *	\return
			 *		\em true if this world handle currently
			 *		holds an exclusive lock on the world,
			 *		\em false otherwise.
			 */
			bool Exclusive () const noexcept;
	
	
	};
	
	
	/**
	 *	Provides an interface through which a
	 *	world generator may be accessed.
	 */
	class WorldGenerator {
	
	
		public:
		
		
			/**
			 *	Acquires a generated column from
			 *	the generator.
			 *
			 *	\param [in] column
			 *		A reference to the column to
			 *		generate.
			 */
			virtual void operator () (ColumnContainer & column) const = 0;
	
	
	};
	
	
	/**
	 *	Contains all information relevant to a
	 *	column being populated.
	 */
	class PopulateEvent {
	
	
		public:
		
		
			/**
			 *	The column being populated.
			 */
			ColumnID ID;
			/**
			 *	A handle through which the world
			 *	may be accessed.
			 */
			const WorldHandle & Handle;
	
	
	};
	
	
	/**
	 *	Provides an interface through which a populator
	 *	may be accessed.
	 */
	class Populator {
	
	
		public:
		
		
			/**
			 *	Populates a column.
			 *
			 *	\param [in] event
			 *		The event object which contains
			 *		information about the column to
			 *		be populated.
			 */
			virtual void operator () (const PopulateEvent & event) const = 0;
	
	
	};
	
	
	/**
	 *	A snapshot of information about the
	 *	world at a single point in time.
	 */
	class WorldInfo {
	
	
		public:
		
		
			/**
			 *	The number of times maintenance has
			 *	been performed on the world.
			 */
			Word Maintenances;
			/**
			 *	The number of nanoseconds that have
			 *	been spent maintaing the world.
			 */
			UInt64 Maintaining;
			
			
			/**
			 *	The number of times a column has
			 *	been loaded from the backing store.
			 */
			Word Loaded;
			/**
			 *	The number of nanoseconds that have
			 *	been spent loading columns from the
			 *	backing store.
			 */
			UInt64 Loading;
			
			
			/**
			 *	The number of columns which have been
			 *	unloaded.
			 */
			Word Unloaded;
			
			
			/**
			 *	The number of times a column has been
			 *	saved to the backing store.
			 */
			Word Saved;
			/**
			 *	The amount of time that has been spent
			 *	saving columns to the backing store.
			 */
			UInt64 Saving;
			
			
			/**
			 *	The number of times a column has been
			 *	generated.
			 */
			Word Generated;
			/**
			 *	The amount of time spent generating
			 *	columns.
			 */
			UInt64 Generating;
			
			
			/**
			 *	The number of times a column has been
			 *	populated.
			 */
			Word Populated;
			/**
			 *	The amount of time spent populating
			 *	columns.
			 */
			UInt64 Populating;
			
			
			/**
			 *	The number of currently loaded
			 *	columns.
			 */
			Word Count;
			/**
			 *	The number of bytes of memory being
			 *	used to hold raw column data.
			 */
			Word Size;
	
	
	};
	
	
	/**
	 *	Represents an attempt to change one block
	 *	to another block.
	 */
	class BlockSetEvent {
	
	
		public:
		
		
			/**
			 *	The WorldHandle object which is
			 *	being used to set this block.
			 *
			 *	May be used to atomically modify
			 *	or read from the world if
			 *	required.
			 */
			const WorldHandle & Handle;
			/**
			 *	The ID of the block being set.
			 */
			BlockID ID;
			/**
			 *	The block that current exists
			 *	at the location given by \em ID.
			 */
			Block From;
			/**
			 *	The block that will exist at the
			 *	location given by \em ID after
			 *	this event completes.
			 */
			Block To;
	
	
	};
	
	
	/**
	 *	Contains and manages the Minecraft world
	 *	as a collection of columns.
	 */
	class World : public Module {
	
	
		friend class WorldHandle;
	
	
		private:
		
		
			//	The key that all components shall
			//	use to determine whether or not they
			//	perform verbose logging
			static const String verbose;
		
		
			//	SETTINGS
			
			//	The world type
			String type;
			//	The world seed
			UInt64 seed;
			//	How often (in milliseconds)
			//	maintenance should be performed
			Word maintenance_interval;
			
			
			//	STATISTICS
			
			//	Number of times maintenance has
			//	been performed
			std::atomic<Word> maintenances;
			//	Number of nanoseconds that have been
			//	spent in maintenance
			std::atomic<UInt64> maintenance_time;
			//	Number of columns that have been
			//	unloaded
			std::atomic<Word> unloaded;
			//	Number of columns that have been
			//	loaded
			std::atomic<Word> loaded;
			//	Number of nanoseconds spent loading
			//	columns
			std::atomic<UInt64> load_time;
			//	Number of times a column has been
			//	saved
			std::atomic<Word> saved;
			//	Number of nanoseconds spent saving
			//	columns
			std::atomic<UInt64> save_time;
			//	Number of times a column has been
			//	generated
			std::atomic<Word> generated;
			//	Number of nanoseconds spent generating
			//	columns
			std::atomic<UInt64> generate_time;
			//	Number of times a column has been
			//	populated
			std::atomic<Word> populated;
			//	Number of nanoseconds spent populating
			//	columns
			std::atomic<UInt64> populate_time;
		
		
			//	Contains loaded world generators
			std::unordered_map<
				Tuple<
					String,
					SByte
				>,
				const WorldGenerator *
			> generators;
			std::unordered_map<
				SByte,
				const WorldGenerator *
			> default_generators;
			
			
			//	Contains loaded world populators
			std::unordered_map<
				SByte,
				Vector<Tuple<const Populator *,Word>>
			> populators;
			
			
			//	Contains the world
			std::unordered_map<
				ColumnID,
				std::unique_ptr<ColumnContainer>
			> world;
			mutable Mutex lock;
			
			
			//	World lock
			Mutex wlock;
			
			
			//	Only one thread is allowed to
			//	perform maintenance on the world
			//	at any one time, to avoid race
			//	conditions while saving
			Mutex maintenance_lock;
			
			
			//	Maps clients to the columns associated
			//	with them
			std::unordered_map<
				SmartPointer<Client>,
				std::unordered_set<ColumnID>
			> clients;
			Mutex clients_lock;
		
		
			//	PRIVATE METHODS
			
			//	WORLD PROCESSING
			
			//	Generates a column
			void generate (ColumnContainer &);
			//	Processes a column up until
			//	a certain satisfactory point
			void process (ColumnContainer &, const WorldHandle * handle=nullptr);
			//	Loads a column from the backing
			//	store (or attempts to).
			//
			//	Returns the state the column is
			//	in after being loaded.
			ColumnState load (ColumnContainer &);
			//	Populates a column
			void populate (ColumnContainer &, const WorldHandle *);
			//	Does maintenance work -- scans and
			//	saves all columns, unloads columns
			//	that are inactive.
			//
			//	Should be invoked periodically.
			void maintenance ();
			//	Saves a column
			//
			//	The maintenance lock must be held
			//	before calling this function
			//
			//	Returns whether or not the column
			//	was actually saved
			bool save (ColumnContainer &);
			
			//	GET/SET
			
			//	Fetches a generator for a given
			//	dimension
			const WorldGenerator & get_generator (SByte) const;
			//	Sets the world's seed to a given string,
			//	or to a randomly-generated number if the
			//	string is null
			void set_seed (Nullable<String>);
			//	Retrieves a column, creating it if it
			//	doesn't exist
			ColumnContainer * get_column (ColumnID, bool create=true);
			
			//	EVENT HANDLING
			
			//	Determines whether a given block can
			//	replace another block at a given set
			//	of coordinates
			bool can_set (const BlockSetEvent &) noexcept;
			//	Fires the event for a given block replacing
			//	another block at a given set of coordinates
			void on_set (const BlockSetEvent &);
			//	Initializes all event arrays be default
			//	constructing them
			void init_events () noexcept;
			//	Destroys all event arrays
			void destroy_events () noexcept;
			//	Cleans up all event arrays, default
			//	constructing each of their objects
			//	so that no module code is loaded
			//	into them
			void cleanup_events () noexcept;
			
			//	MISC

			//	Retrieves the key that will be associated
			//	with a given column in the backing
			//	store
			String key (ColumnID) const;
			String key (const ColumnContainer &) const;
		
		
		public:
		
		
			/**
			 *	Retrieves a reference to a valid
			 *	instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance
			 *		of this class.
			 */
			static World & Get () noexcept;
		
		
			//	EVENTS
		
		
			Event<void (const BlockSetEvent &)> OnSet;
			Event<void (const BlockSetEvent &)> OnReplace [4096];
			Event<void (const BlockSetEvent &)> OnPlace [4096];
			
			
			Event<bool (const BlockSetEvent &)> CanSet;
			Event<bool (const BlockSetEvent &)> CanReplace [4096];
			Event<bool (const BlockSetEvent &)> CanPlace [4096];
			
			
			/**
			 *	\cond
			 */
		
		
			World () noexcept;
			~World () noexcept;
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	Adds a WorldGenerator to be used in the
			 *	generation of columns for a certain dimension.
			 *
			 *	This overload specifies a default generator.
			 *	Default generators are only used where a
			 *	generator for the chosen world type cannot
			 *	be found.
			 *
			 *	\param [in] generator
			 *		A pointer to the WorldGenerator.  The
			 *		caller is responsible for this pointer's
			 *		lifetime and ensuring that it remains
			 *		valid until the server is shutdown.
			 *	\param [in] dimension
			 *		The dimension that should use \em generator
			 *		as a default.
			 */
			void Add (const WorldGenerator * generator, SByte dimension);
			/**
			 *	Adds a WorldGenerator to be used in the
			 *	generation of columns for a certain dimension.
			 *
			 *	This overload specifies a generator for a
			 *	certain world type and dimension combination.
			 *	The generator shall only be invoked when
			 *	generating columns in the specified dimension
			 *	when the world is set to the specified type.
			 *
			 *	\param [in] generator
			 *		A pointer to the WorldGenerator.  The
			 *		caller is responsible for this pointer's
			 *		lifetime and ensuring that it remains
			 *		valid until the server is shutdown.
			 *	\param [in] type
			 *		The world type that \em generator may
			 *		supply columns for.
			 *	\param [in] dimension
			 *		The dimension that \em generator may
			 *		supply chunks for.
			 */
			void Add (const WorldGenerator * generator, String type, SByte dimension);
			
			
			/**
			 *	Adds a Populator to be used in the population
			 *	of columns for a certain dimension.
			 *
			 *	\param [in] populator
			 *		A pointer to the Populator.  The caller
			 *		is responsible for this pointer's lifetime
			 *		and ensuring that it remains valid until
			 *		the server is shutdown.
			 *	\param [in] dimension
			 *		The dimension in which \em populator may
			 *		populate columns.
			 *	\param [in] priority
			 *		The priority of the populator.  This is the
			 *		relative order in which it shall be invoked
			 *		when populating columns for \em dimension.
			 */
			void Add (const Populator * populator, SByte dimension, Word priority);
			
			
			/**
			 *	Adds a client to a particular column.
			 *
			 *	The column will be loaded, populated, and/or
			 *	generated as necessary, after which the
			 *	column will be sent to the client-in-question.
			 *
			 *	The column will not be unloaded until this
			 *	client is removed from it.
			 *
			 *	\param [in] client
			 *		The client to add.
			 *	\param [in] id
			 *		The column to which to add \em client.
			 *	\param [in] async
			 *		If \em true the function will return at once
			 *		if the column specified by \em id is already
			 *		undergoing processing by another thread.  The
			 *		column will be sent to \em client at some
			 *		indeterminate point in the future.  If \em false
			 *		the method will block in the aforementioned situation
			 *		until the column has been prepared by the
			 *		other thread and sent.  Defaults to \em false.
			 */
			void Add (SmartPointer<Client> client, ColumnID id, bool async=false);
			/**
			 *	Removes a particular client from a particular column.
			 *
			 *	If that client was never added to that particular column,
			 *	or has already been removed from it, nothing happens.
			 *
			 *	\param [in] client
			 *		The client to remove.
			 *	\param [in] id
			 *		The column from which to remove \em client.
			 *	\param [in] force
			 *		If \em true \em client will be removed from the column
			 *		specified by \em id without sending the necessary packet
			 *		to \em client to unload the column on the client side.
			 *		Defaults to \em false.
			 */
			void Remove (SmartPointer<Client> client, ColumnID id, bool force=false);
			/**
			 *	Removes all columns from a particular client.
			 *
			 *	If the client never had any columns, or all columns have
			 *	been removed from the client, nothing happens.
			 *
			 *	\param [in] client
			 *		The client from which all columns shall be
			 *		removed.
			 *	\param [in] force
			 *		If \em true columns removed from \em client shall not
			 *		result in a packet being sent to \em client to unload
			 *		those columns.  Defaults to \em false.
			 */
			void Remove (SmartPointer<Client> client, bool force=false);
			
			
			/**
			 *	Retrieves the world's seed.
			 *
			 *	\return
			 *		The world's seed.
			 */
			UInt64 Seed () const noexcept;
			/**
			 *	Retrieves a random number generator
			 *	seeded by the world's seed.
			 *
			 *	\tparam T
			 *		The type of random number generator
			 *		to create and seed.  Defaults to
			 *		std::mt19937.
			 *
			 *	\return
			 *		A seeded random number generator of
			 *		type \em T.
			 */
			template <typename T=std::mt19937>
			T GetRandom () const noexcept(
				std::is_nothrow_constructible<
					std::seed_seq,
					std::initializer_list<Word>
				>::value &&
				std::is_nothrow_constructible<
					T,
					std::seed_seq
				>::value
			) {
			
				std::seed_seq seq({seed});
				
				return T(seq);
			
			}
			/**
			 *	Retrieves a random number generator
			 *	seeded by the world's seed and a
			 *	column ID.
			 *
			 *	\tparam T
			 *		The type of random number generator
			 *		to create and seed.  Defaults to
			 *		std::mt19937.
			 *
			 *	\param [in] id
			 *		A column ID to use to seed the random
			 *		number generator.
			 *
			 *	\return
			 *		A seeded random number generator of
			 *		type \em T.
			 */
			template <typename T=std::mt19937>
			T GetRandom (ColumnID id) const noexcept(
				std::is_nothrow_constructible<
					std::seed_seq,
					std::initializer_list<Word>
				>::value &&
				std::is_nothrow_constructible<
					T,
					std::seed_seq
				>::value
			) {
			
				Word seed=23;
				seed*=31;
				seed+=this->seed;
				
				union {
					UInt32 out;
					Int32 in;
				};
				
				in=id.X;
				seed*=31;
				seed+=out;
				
				in=id.Z;
				seed*=31;
				seed+=out;
				
				union {
					Byte out_b;
					SByte in_b;
				};
				
				in_b=id.Dimension;
				seed*=31;
				seed+=out_b;
				
				std::seed_seq seq({seed});
				
				return T(seq);
			
			}
			/**
			 *	Retrieves the current level
			 *	type.
			 *
			 *	\return
			 *		The string which represents
			 *		the current level type.
			 */
			const String & Type () const noexcept;
			
			
			/**
			 *	Retrieves information about the world.
			 *
			 *	\return
			 *		A structure populated with information
			 *		about the world.
			 */
			WorldInfo GetInfo () const noexcept;
			
			
			/**
			 *	Expresses interest in a column, loading
			 *	it into memory and preventing it from being
			 *	unloaded for the duration of the interest.
			 *
			 *	\param [in] id
			 *		The column in which to express interest.
			 *	\param [in] prepare
			 *		If \em true the column will be generated
			 *		and populated, otherwise the column will
			 *		simply be placed into memory.
			 */
			void Interested (ColumnID id, bool prepare=true);
			/**
			 *	Ends interest in a column, allowing the
			 *	column to once again be unloaded if appropriate.
			 *
			 *	\param [in] id
			 *		The column in which to end interest.
			 */
			void EndInterest (ColumnID id) noexcept;
			
			
			/**
			 *	Begins writing to/reading from the world
			 *	by creating a handle which allows the
			 *	world to be written/read according to
			 *	certain strategies.
			 *
			 *	\param [in] write
			 *		The strategy that shall be used to
			 *		write to the world.  Defaults to
			 *		\em BlockWriteStrategy::Dirty.
			 *	\param [in] access
			 *		The strategy that shall be used to
			 *		access the world.  Defaults to
			 *		\em BlockAccessStrategy::Populate.
			 *
			 *	\return
			 *		A handle through which the world may
			 *		be read and written.
			 */
			WorldHandle Begin (BlockWriteStrategy write=BlockWriteStrategy::Dirty, BlockAccessStrategy access=BlockAccessStrategy::Populate);
	
	
	};
	 

}

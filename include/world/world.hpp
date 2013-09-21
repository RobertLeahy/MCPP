/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <functional>
#include <unordered_map>
#include <utility>
#include <cstddef>


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
	struct hash<ColumnID> {
	
	
		public:
		
		
			size_t operator () (const ColumnID & subject) const noexcept {
			
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
	struct hash<BlockID> {
	
	
		public:
		
		
			size_t operator () (const BlockID & subject) const noexcept {
			
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
			Vector<Byte> ToChunkData () const;
			//	Retrieves a 0x33 packet which unloads
			//	the contained column from a client
			Vector<Byte> GetUnload () const;
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
	 
	 
	class WorldLock {
	
	
		private:
		
		
			//	The type of a lock request
			typedef Tuple<
				//	A callback for asynchronous lock
				//	requests, an empty std::function
				//	otherwise
				std::function<void (bool)>,
				//	Whether this lock request is for
				//	an exclusive lock
				bool,
				//	A boolean which synchronous lock
				//	requests will wait on
				std::atomic<bool> *,
				//	The thread ID of synchronous lock
				//	requests
				decltype(Thread::ID())
			> wl_tuple;
		
		
			//	Locks currently held
			std::unordered_map<
				//	ID of the thread holding
				//	the lock
				decltype(Thread::ID()),
				//	Recursion count
				Word
			> locks;
			//	Number of locks (necessary since
			//	asynchronous locks will not always
			//	be in the hash map
			Word num;
			//	Whether the currently-held lock
			//	(if there's just one) is exclusive
			//	or not
			bool exclusive;
			//	List of pending lock requests
			Vector<wl_tuple> pending;
			//	The lock that synchronizes access
			//	to this structure
			Mutex lock;
			//	The condvar on which non-exclusive
			//	requests wait
			CondVar wait;
			//	The condvar on which exclusive requests
			//	wait
			CondVar ex_wait;
			//	The thread pool that shall be used for
			//	asynchronous callbacks
			ThreadPool & pool;
			//	The panic callback which shall be invoked
			//	if something goes wrong with the thread
			//	pool
			std::function<void ()> panic;
			
			
			inline void acquire_impl(const std::function<void (bool)> &, decltype(Thread::ID()), bool);
			void acquire (std::function<void (bool)>, bool);
			inline bool can_acquire (const wl_tuple &) noexcept;
			void release ();
			inline void release_impl () {
			
				try {
				
					Release();
					
				} catch (...) {
				
					//	The lock may be in an inconsistent state!
					try {	if (panic) panic();	} catch (...) {	}
					
					//	We don't know what went wrong in Release,
					//	so we can't attempt recovery, just rethrow
					
					throw;
				
				}
			
			}
			template <typename T, typename... Args>
			void acquire_async_impl (bool ex, T && callback, Args &&... args) {
			
				auto bound=std::bind(
					std::forward<T>(callback),
					std::forward<Args>(args)...
				);
				
				//	If you try and bind bound
				//	expressions into a new bound
				//	expression weird stuff happens,
				//	so I just make this garbage
				//	intermediary object.
				//
				//	If we had capture-by-move
				//	this wouldn't be an issue,
				//	but we'll have to wait for C++14
				//	for that...
				Tuple<decltype(bound)> wrapped(std::move(bound));
				
				acquire(
					std::bind(
						[this] (decltype(wrapped) wrapped, bool set) mutable {
						
							if (set) {
							
								auto id=Thread::ID();
							
								lock.Execute([&] () mutable {
								
									try {
									
										locks.emplace(
											id,
											1
										);
									
									} catch (...) {
									
										//	The lock may be in an inconsistent
										//	state!
										try {	if (panic) panic();	} catch (...) {	}
										
										//	Attempt recovery as best we can
										--num;
										exclusive=false;
										
										throw;
									
									}
								
								});
								
							}
							
							try {
							
								wrapped.template Item<0>()();
								
							} catch (...) {
							
								release_impl();
								
								throw;
							
							}
							
							release_impl();
						
						},
						std::move(wrapped),
						std::placeholders::_1
					),
					ex
				);
			
			}
			
			
		public:
			
			
			WorldLock () = delete;
			WorldLock (
				ThreadPool & pool,
				std::function<void ()> panic
			) noexcept;
			
			
			void Acquire ();
			void AcquireExclusive ();
			template <typename T, typename... Args>
			void Acquire (T && callback, Args &&... args) {
			
				acquire_async_impl(
					false,
					std::forward<T>(callback),
					std::forward<Args>(args)...
				);
			
			}
			template <typename T, typename... Args>
			void AcquireExclusive (T && callback, Args &&... args) {
			
				acquire_async_impl(
					true,
					std::forward<T>(callback),
					std::forward<Args>(args)...
				);
			
			}
			void Release ();
			bool Transfer () noexcept;
			void Resume ();
	
	
	};
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Provides an interface through which a
	 *	world generator may be accessed.
	 *
	 *	A "world generator" encapsulates the
	 *	concept of a block generator, which accepts
	 *	a set of input coordinates and yields a block,
	 *	and a "biome generator", which takes a set
	 *	of input coordinates and yields a biome.
	 */
	class WorldGenerator {
	
	
		public:
		
		
			/**
			 *	When overriden in a derived class,
			 *	invokes the block generator.
			 *
			 *	\param [in] id
			 *		The ID of the block which shall
			 *		be generated and returned.
			 *
			 *	\return
			 *		The block given by \em id.
			 */
			virtual Block operator () (const BlockID & id) const = 0;
			/**
			 *	When overriden in a derived class,
			 *	invokes the biome generator.
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
			virtual Byte operator () (Int32 x, Int32 z, SByte dimension) const = 0;
	
	
	};
	
	
	/**
	 *	Contains and manages the Minecraft world
	 *	as a collection of columns.
	 */
	class WorldContainer : public Module {
	
	
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
				Vector<std::function<void (ColumnID)>>
			> populators;
			
			
			//	Contains the world
			std::unordered_map<
				ColumnID,
				SmartPointer<ColumnContainer>
			> world;
			Mutex lock;
			
			
			//	World lock
			Nullable<WorldLock> wlock;
			
			
			//	Which threads are currently
			//	populating?
			std::unordered_set<
				decltype(Thread::ID())
			> populating;
			mutable RWLock populating_lock;
			
			
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
			void process (ColumnContainer &);
			//	Loads a column from the backing
			//	store (or attempts to).
			//
			//	Returns the state the column is
			//	in after being loaded.
			ColumnState load (ColumnContainer &);
			//	Populates a column
			void populate (ColumnContainer &);
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
			SmartPointer<ColumnContainer> get_column (ColumnID);
			
			//	EVENT HANDLING
			
			//	Determines whether a given block can
			//	replace another block at a given set
			//	of coordinates
			bool can_set (BlockID, Block, Block) noexcept;
			//	Fires the event for a given block replacing
			//	another block at a given set of coordinates
			void on_set (BlockID, Block, Block);
			//	Initializes all event arrays be default
			//	constructing them
			void init_events () noexcept;
			//	Destroys all event arrays
			void destroy_events () noexcept;
			
			//	IS POPULATING?
			
			bool is_populating () const noexcept;
			inline void start_populating ();
			inline void stop_populating () noexcept;
			
			//	MISC

			//	Retrieves the key that will be associated
			//	with a given column in the backing
			//	store
			String key (ColumnID) const;
			String key (const ColumnContainer &) const;
		
		
		public:
		
		
			//	EVENTS
		
		
			Event<void (BlockID, Block, Block)> OnSet;
			Event<void (BlockID, Block, Block)> OnReplace [4096];
			Event<void (BlockID, Block, Block)> OnPlace [4096];
			
			
			Event<bool (BlockID, Block, Block)> CanSet;
			Event<bool (BlockID, Block, Block)> CanReplace [4096];
			Event<bool (BlockID, Block, Block)> CanPlace [4096];
			
			
			/**
			 *	\cond
			 */
		
		
			WorldContainer ();
			~WorldContainer () noexcept;
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
			 *	Begins a transaction against the world.
			 *
			 *	Does not return until the lock may be
			 *	acquired.
			 *
			 *	For the duration of the transaction the
			 *	thread of execution which called this
			 *	function is the only thread of execution
			 *	which may perform writes to the world.
			 *
			 *	Note that a thread of execution is not
			 *	necessarily tied to a specific thread.
			 *	The transaction lock follows the thread
			 *	of execution through asynchronous callbacks
			 *	enqueued by whichever thread currently
			 *	holds the lock.
			 */
			void Begin ();
			/**
			 *	Asynchronously begins a transaction against
			 *	the world.
			 *
			 *	When the asynchronous callback provided ends,
			 *	the transaction implicitly ends.
			 *
			 *	For the duration of the transaction the
			 *	thread of execution which called this function
			 *	is the only thread of execution which
			 *	may perform writes to the world.
			 *
			 *	\tparam T
			 *		The type of callback to be invoked once
			 *		the transaction begins.
			 *	\tparam Args
			 *		The types of the arguments to be
			 *		forwarded through to the callback of
			 *		type \em T.
			 *
			 *	\param [in] callback
			 *		The callback to be invoked once the lock
			 *		is acquired.
			 *	\param [in] args
			 *		The arguments which shall be forwarded
			 *		to \em callback.
			 */
			template <typename T, typename... Args>
			void Begin (T && callback, Args &&... args) {
			
				//	Acquire asynchronously
				wlock->AcquireExclusive(
					std::forward<T>(callback),
					std::forward<Args>(callback)...
				);
			
			}
			/**
			 *	Explicitly ends a transaction.
			 *
			 *	All synchronously-acquired transactions
			 *	must be ended this way, unless they are
			 *	passed off to an asynchronous callback,
			 *	in which case the transaction will
			 *	implicitly end once the callback which
			 *	was invoked last returns.
			 */
			void End ();
			
			
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
			 *	Synchronously retrieves a block.
			 *
			 *	\param [in] id
			 *		The coordinates of the block which
			 *		shall be retrieved.
			 *
			 *	\return
			 *		The block at the coordinates
			 *		given by \em id.
			 */
			Block GetBlock (BlockID id);
			/**
			 *	Asynchronously retrieves a block.
			 *
			 *	\tparam T
			 *		The type of callback to be invoked once
			 *		the block-in-question is retrieved.
			 *	\tparam Args
			 *		The types of the arguments to be
			 *		forwarded through to the callback of
			 *		type \em T.
			 *
			 *	\param [in] id
			 *		The coordinates of the block which
			 *		shall be retrieved.
			 *	\param [in] callback
			 *		The callback which shall be invoked
			 *		when the block is retrieved.  Its
			 *		first argument shall be \em id, and
			 *		its second argument shall be the
			 *		block which was retrieved.
			 *	\param [in] args
			 *		The arguments to forward through to
			 *		\em callback.
			 */
			template <typename T, typename... Args>
			void GetBlock (BlockID id, T && callback, Args &&... args) {
			
				auto bound=std::bind(
					std::forward<T>(callback),
					std::placeholders::_1,
					std::placeholders::_2,
					std::forward<Args>(args)...
				);
				
				//	Work around limitations of binding
				//	bound functions
				Tuple<decltype(bound)> wrapper(std::move(bound));
				
				//	Get the column the desired block resides
				//	in
				auto column=get_column(id.GetContaining());
				
				//	If this thread holds a lock,
				//	we need to move it, unless
				//	this thread is populating, in
				//	which case it needs to keep
				//	its lock
				bool resume=wlock->Transfer();
				
				try {
				
					//	Enqueue
					if (!column->InvokeWhen(
						is_populating() ? ColumnState::Generated : ColumnState::Populated,
						std::bind(
							[this,resume] (BlockID id, decltype(column) column, decltype(wrapper) wrapper) mutable {
								
								if (resume) wlock->Resume();
								
								try {
								
									wrapper.template Item<0>()(
										id,
										column->GetBlock(id)
									);
								
								} catch (...) {	}
								
								//	Don't leak interest
								column->EndInterest();
								//	Don't leak lock
								if (resume) try {
								
									wlock->Release();
									
								} catch (...) {	}
								
							},
							id,
							column,
							std::move(wrapper)
						)
					)) process(*column);
				
				} catch (...) {
				
					column->EndInterest();
					
					//	Make sure the lock gets
					//	restored
					if (resume) try {
					
						wlock->Resume();
						
					//	Nothing really we can do
					//	about this...
					} catch (...) {	}
					
					throw;
				
				}
				
				//	We don't release interest here,
				//	it's done inside the callback
			
			}
			
			
			/**
			 *	Synchronously sets a block.
			 *
			 *	\param [in] id
			 *		The coordinates of the block to set.
			 *	\param [in] block
			 *		The block to set at the coordinates given
			 *		by \em id.
			 *	\param [in] force
			 *		Whether the setting of the block shall be
			 *		forced.  If setting the block is forced,
			 *		event handlers shall not have the opportunity
			 *		to stop the setting of the block, and shall
			 *		only be informed after it has been set that
			 *		it has been.  Defaults to \em true.
			 *
			 *	\return
			 *		\em true if setting the block succeeded,
			 *		\em false otherwise.  If \em force is
			 *		\em true, \em true is always returned.
			 */
			bool SetBlock (BlockID id, Block block, bool force=true);
			/**
			 *	Asynchronously sets a block.
			 *
			 *	\tparam T
			 *		The type of callback which shall be
			 *		invoked when the block has been set.
			 *	\tparam Args
			 *		The type of arguments which shall be
			 *		forwarded through to the callback of
			 *		type \em T.
			 *
			 *	\param [in] id
			 *		The coordinates of the block to set.
			 *	\param [in] block
			 *		The block to set at the coordinates given
			 *		by \em id.
			 *	\param [in] force
			 *		Whether the setting of the block shall be
			 *		forced.  If setting the block is forced,
			 *		event handlers shall not have the opportunity
			 *		to stop the setting of the block, and shall
			 *		only be informed after it has been set that
			 *		it has been.  Defaults to \em true.
			 *	\param [in] callback
			 *		The callback which shall be invoked once the
			 *		block has been set (or once setting the block
			 *		has failed).  Shall be passed \em true if the
			 *		operation succeeds, \em false otherwise, \em id,
			 *		and \em block.
			 *	\param [in] args
			 *		Arguments which shall be forwarded through to
			 *		\em callback.
			 */
			template <typename T, typename... Args>
			void SetBlock (BlockID id, Block block, bool force, T && callback, Args &&... args) {
			
				auto bound=std::bind(
					std::forward<T>(callback),
					std::placeholders::_1,
					id,
					block,
					std::forward<Args>(args)...
				);
				
				//	Work around limitations of binding
				//	bound functions
				Tuple<decltype(bound)> wrapper(std::move(bound));
				
				//	Get the column the desired block resides
				//	in
				auto column=get_column(id.GetContaining());
				
				try {
				
					//	Acquire lock asynchronously
					wlock->Acquire(
						[this,id,block,force,column] (decltype(wrapper) wrapper) mutable {
						
							//	Lock acquired
							
							//	The lock will have to be
							//	transferred through
							wlock->Transfer();
							
							try {
							
								//	Enqueue, process, or dispatch
								//	immediately, as applicable
								if (!column->InvokeWhen(
									ColumnState::Generated,
									std::bind(
										[this,id,block,force,column] (decltype(wrapper) wrapper) mutable {
										
											//	Resume the lock
											wlock->Resume();
											
											bool success=false;
											try {
											
												try {
												
													auto old=column->GetBlock(id);
												
													//	Can we set the block?
													if (force || can_set(id,old,block)) {
													
														//	YES
														
														success=true;
														
														column->SetBlock(id,block);
														
														//	Only fire events for columns
														//	that are populating or populated
														auto state=column->GetState();
														if (
															(state==ColumnState::Populating) ||
															(state==ColumnState::Populated)
														) on_set(
															id,
															old,
															block
														);
													
													}
													
												} catch (...) {
												
													//	Don't leak the lock
													try {
													
														wlock->Release();
													
													} catch (...) {	}
													
													throw;
												
												}
												
												wlock->Release();
												
											} catch (...) {
											
												column->EndInterest();
												
												throw;
											
											}
											
											column->EndInterest();
											
											//	Invoke callback
											try {
											
												wrapper.template Item<0>()(success);
											
											} catch (...) {	}
										
										},
										std::move(wrapper)
									)
								)) process(*column);
							
							} catch (...) {
							
								column->EndInterest();
								
								//	Make sure to resume the lock
								//	so it can properly be
								//	released
								try {
								
									wlock->Resume();
									
								//	Nothing really we can do about
								//	this...
								} catch (...) {	}
								
								throw;
							
							}
						
						},
						std::move(wrapper)
					);
				
				} catch (...) {
				
					column->EndInterest();
					
					throw;
				
				}
				
				//	We don't release interest here, it's done
				//	within the asynchronous callback
			
			}
	
	
	};
	
	
	/**
	 *	The single valid instance of WorldContainer.
	 */
	extern Nullable<WorldContainer> World;
	 

}

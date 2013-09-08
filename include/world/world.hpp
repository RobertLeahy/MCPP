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
			Byte Biomes [16*16];
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
			void RemovePlayer (SmartPointer<Client>);
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
	
	
	template <typename T>
	class bind_wrapper {
	
	
		public:
		
		
			T bound;
			
			
			template <typename... Args>
			void operator () (Args &&... args) const noexcept(noexcept(bound(std::forward<Args>(args)...))) {
			
				bound(std::forward<Args>(args)...);
			
			}
	
	
	};
	 
	 
	class WorldLock {
	
	
		private:
		
		
			//	ID of the thread that currently
			//	holds the lock
			Nullable<decltype(Thread::ID())> id;
			//	Recursion count, if zero
			//	the lock is not held
			Word count;
			//	Guards against unsynchronized
			//	access
			Mutex lock;
			//	Synchronous calls wait here
			CondVar wait;
			//	Asynchronous calls are stored
			//	here.  The lock decides whether
			//	to wake the condvar or dispatch
			//	an asynchronous callback by
			//	checking for empty callbacks,
			//	which denote that a synchronous
			//	call was next in line
			Vector<std::function<void (bool)>> list;
			//	The thread pool that shall be
			//	used for asynchronous callbacks
			ThreadPool & pool;
			//	The panic callback
			std::function<void ()> panic;
			
			
			void acquire (std::function<void (bool)>);
			
			
		public:
			
			
			WorldLock () = delete;
			WorldLock (
				ThreadPool & pool,
				std::function<void ()> panic
			) noexcept;
			
			
			void Acquire ();
			template <typename T, typename... Args>
			void Acquire (T && callback, Args &&... args) {
			
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
				bind_wrapper<decltype(bound)> wrapped{std::move(bound)};
				
				acquire(
					std::bind(
						[this] (decltype(wrapped) callback, bool set_thread) mutable {
						
							if (set_thread) {
							
								lock.Acquire();
								id.Construct(Thread::ID());
								lock.Release();
							
							}
							
							try {
							
								callback();
							
							} catch (...) {	}
							
							try {
							
								Release();
							
							} catch (...) {	}
						
						},
						std::move(wrapped),
						std::placeholders::_1
					)
				);
			
			}
			void Release ();
			bool Transfer () noexcept;
			void Resume () noexcept;
	
	
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
		
		
			WorldContainer ();
			~WorldContainer () noexcept;
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
		
		
			void Add (const WorldGenerator * generator, SByte dimension);
			void Add (const WorldGenerator * generator, String type, SByte dimension);
			
			
			void Begin ();
			template <typename T, typename... Args>
			void Begin (T && callback, Args &&... args) {
			
				//	Acquire asynchronously
				wlock->Acquire(
					std::forward<T>(callback),
					std::forward<Args>(callback)...
				);
			
			}
			void End ();
			
			
			Block GetBlock (BlockID id);
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
				bind_wrapper<decltype(bound)> wrapper{std::move(bound)};
				
				//	Get the column the desired block resides
				//	in
				auto column=get_column(id.GetContaining());
				
				//	If this thread holds a lock,
				//	we need to move it, unless
				//	this thread is populating, in
				//	which case it needs to keep
				//	its lock
				bool resume=!is_populating() && wlock->Transfer();
				
				try {
				
					if (!(
						//	Enqueue
						column->InvokeWhen(
							ColumnState::Populated,
							std::bind(
								[this,resume] (BlockID id, decltype(column) column, decltype(wrapper) callback) mutable {
									
									if (resume) wlock->Resume();
									
									try {
									
										callback(
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
						) ||
						//	If we're populating, we'll
						//	end up at ColumnState::Populated
						//	anyway, so don't process
						is_populating()
					)) process(*column);
				
				} catch (...) {
				
					column->EndInterest();
					
					//	Make sure the lock gets
					//	restored
					if (resume) wlock->Resume();
					
					throw;
				
				}
				
				//	We don't release interest here,
				//	it's done inside the callback
			
			}
			
			
			bool SetBlock (BlockID id, Block block);
	
	
	};
	
	
	/**
	 *	The single valid instance of WorldContainer.
	 */
	extern Nullable<WorldContainer> World;
	 

}

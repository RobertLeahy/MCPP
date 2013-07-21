/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <atomic>


namespace MCPP {


	/**
	 *	\cond
	 */
	 
	 
	class ColumnID;
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	Encapsulates all the information which may
	 *	uniquely identify a block.
	 */
	class BlockID {
	
	
		public:
		
		
			/**
			 *	The X coordinate of this block.
			 */
			Int32 X;
			/**
			 *	The Y coordinate of this block.
			 */
			Int32 Y;
			/**
			 *	The Z coordinate of this block.
			 */
			Int32 Z;
			/**
			 *	The dimension of this block.
			 */
			SByte Dimension;
			
			
			/**
			 *	Checks whether this block is contained
			 *	by a given column.
			 *
			 *	\param [in] column
			 *		The column to check.
			 *
			 *	\return
			 *		\em true if \em column contains this
			 *		block, \em false otherwise.
			 */
			bool ContainedBy (const ColumnID & column) const noexcept;
			/**
			 *	Fetches the column that contains this block.
			 *
			 *	\return
			 *		The column that contains this block.
			 */
			ColumnID ContainedBy () const noexcept;
			/**
			 *	Retrieves the offset of this block within
			 *	the column that contains it.
			 *
			 *	\return
			 *		The offset of this block within the
			 *		column that contains it.
			 */
			Word Offset () const noexcept;
			
			
			/**
			 *	Checks whether this block and another
			 *	block are identical.
			 *
			 *	\param [in] other
			 *		The block to compare with.
			 *
			 *	\return
			 *		\em true if this block and \em other
			 *		refer to the same block, \em false
			 *		otherwise.
			 */
			bool operator == (const BlockID & other) const noexcept;
			/**
			 *	Checks whether this block and another block
			 *	are not identical.
			 *
			 *	\param [in] other
			 *		The block to compare with.
			 *
			 *	\return
			 *		\em true if this block and \em other
			 *		do not refer to the same block, \em false
			 *		otherwise.
			 */
			bool operator != (const BlockID & other) const noexcept;
	
	
	};


	/**
	 *	Encapsulates all the information
	 *	which may uniquely identify a
	 *	column.
	 */
	class ColumnID {
	
	
		public:
		
		
			/**
			 *	Finds the column that a given block
			 *	is in.
			 *
			 *	\param [in] block
			 *		The block-in-question.
			 *
			 *	\return
			 *		The ID of the column which encloses
			 *		\em block.
			 */
			static ColumnID Make (BlockID block) noexcept;
		
		
			/**
			 *	The X-coordinate of this chunk.
			 *
			 *	This coordinate is in chunks, not
			 *	blocks.  Multiply by 16 to get
			 *	the block coordinate.
			 */
			Int32 X;
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
			
			
			/**
			 *	Checks whether this column contains a
			 *	given block.
			 *
			 *	\param [in] block
			 *		The block to check.
			 *
			 *	\return
			 *		\em true if this column contains
			 *		\em block, \em false otherwise.
			 */
			bool Contains (const BlockID & block) const noexcept;
			
			
			/**
			 *	Checks whether this column and another
			 *	column are identical.
			 *
			 *	\param [in] other
			 *		The column to compare with.
			 *
			 *	\return
			 *		\em true if \em other and this object
			 *		refer to the same column, \em false
			 *		otherwise.
			 */
			bool operator == (const ColumnID & other) const noexcept;
			/**
			 *	Checks whether this column and another column
			 *	are not identical.
			 *
			 *	\param [in] other
			 *		The column to compare with.
			 *
			 *	\return
			 *		\em true if \em other and this object
			 *		refer to different columns, \em false
			 *		otherwise.
			 */
			bool operator != (const ColumnID & other) const noexcept;
	
	
	};
	
	
}


/**
 *	\cond
 */


namespace std {


	template <>
	class hash<ColumnID> {
	
	
		public:
		
		
			size_t operator () (const ColumnID & input) const noexcept {
			
				return	static_cast<UInt32>(input.X)+
						static_cast<UInt32>(input.Z)+
						static_cast<Byte>(input.Dimension);
			
			}
	
	
	};
	
	
	template <>
	class hash<BlockID> {
	
	
		public:
		
		
			size_t operator () (const BlockID & block) const noexcept {
			
				return	static_cast<UInt32>(block.X)+
						static_cast<UInt32>(block.Y)+
						static_cast<UInt32>(block.Z)+
						static_cast<UInt32>(block.Dimension);
			
			}
	
	
	};


}


/**
 *	\endcond
 */
 
 
namespace MCPP {


	/**
	 *	Encapsulates a single block in the
	 *	Minecraft world.
	 */
	class Block {
	
	
		public:
		
		
			/**
			 *	The block type associated with this
			 *	block.
			 *
			 *	All bits higher than the 12th bit
			 *	will be ignored, meaning that this
			 *	field has an effective maximum of
			 *	4095.
			 */
			UInt16 Type;
			/**
			 *	The metadata associated with this
			 *	block.
			 *
			 *	All bits higher than the 4th bit
			 *	will be ignored, meaning that this
			 *	field has an effective maximum of
			 *	15.
			 */
			Byte Metadata;
			/**
			 *	The light value associated with this
			 *	block.
			 *
			 *	All bits higher than the 4th bit
			 *	will be ignored, meaning that this
			 *	field has an effective maximum of
			 *	15.
			 */
			Byte Light;
			/**
			 *	The skylight value associated with this
			 *	block.
			 *
			 *	All bits higher than the 4th bit
			 *	will be ignored, meaning that this
			 *	field has an effective maximum of
			 *	15.
			 */
			Byte Skylight;
	
	
	};


	class ColumnContainer {
	
	
		public:
		
		
			ColumnContainer ();
			ColumnContainer (const ColumnContainer &) = delete;
			ColumnContainer (ColumnContainer &&) = delete;
			ColumnContainer & operator = (const ColumnContainer &) = delete;
			ColumnContainer & operator = (ColumnContainer &&) = delete;
		
		
			//	The raw column data
			Column Storage;
			
			//	A lock which guards the structure
			//	against unsynchronized access
			Mutex Lock;
			
			//	Whether the column has been generated
			//	or not
			bool Generated;
			
			//	Whether the column is being processed
			//	(i.e. generated or populated) currently
			bool Processing;
			
			//	A condition variable on which threads
			//	may wait for the column's state to
			//	change.
			CondVar Wait;
			
			//	The interest count.  So long as the
			//	interest count is not zero the chunk
			//	shall not be unloaded.
			std::atomic<Word> Interested;

			//	The set of clients which have or are
			//	interested in this column
			std::unordered_set<SmartPointer<Client>> Clients;
			
			//	True if the column has been modified,
			//	false otherwise.
			bool Dirty;
	
	
	};
	
	
	/**
	 *	The various states that a column
	 *	may be in.
	 */
	enum class ColumnState {
	
		Unloaded,	/**<	The column is not loaded/generated.	*/
		Generating,	/**<	The column is currently loading or generating.	*/
		Generated,	/**<	The column has been generated, but is not populated.	*/
		Populating,	/**<	The column has been generated and is being populated.	*/
		Populated	/**<	The column has been generated and populated.	*/
	
	};


	/**
	 *	Contains the Minecraft world and is
	 *	responsible for loading, unloading,
	 *	and sending columns and other block-related
	 *	packets.
	 */
	class WorldContainer : public Module {
	
	
		public:
		
		
			/**
			 *	The type of callback which shall be
			 *	invoked to obtain unpopulated columns.
			 */
			typedef std::function<void (ColumnID, Column &)> Provider;
			/**
			 *	The type of callback which shall be
			 *	invoked to populate a column.
			 */
			typedef std::function<void (ColumnID)> Populator;
			
			
		private:
		
		
			//
			//	PROVIDERS
			//
		
		
			//	Maps dimensions to maps which
			//	map the world type string to
			//	their column providers
			std::unordered_map<
				SByte,
				std::unordered_map<
					String,
					Provider
				>
			> providers;
			//	Maps dimensions to a default
			//	provider for that dimension,
			//	which shall be used if there
			//	is no provider for the given
			//	world type
			std::unordered_map<
				SByte,
				Provider
			> default_providers;
			
			
			//
			//	POPULATORS
			//
			
			
			//	A list of providers.  The tuple
			//	stores their priority, which is
			//	used as they are being added to
			//	determine the order in which they
			//	shall be called.
			Vector<
				Tuple<
					Word,
					Populator
				>
			> populators;
			
			
			//
			//	COLUMN STORAGE/MANAGEMENT
			//
			
			
			//	Stores the world by mapping
			//	ColumnIDs to ColumnContainers
			//
			//	ColumnContainers are stored
			//	in a SmartPointer for two reasons:
			//
			//	1.	Because they are not copyable
			//		or movable.
			//	2.	Because this reduces the amount
			//		of time that threads must spend
			//		with the world lock acquired.
			std::unordered_map<
				ColumnID,
				SmartPointer<ColumnContainer>
			> world;
			//	Guards the world against unsynchronized
			//	modification
			Mutex world_lock;
			//	Maps clients to columns that they
			//	have
			std::unordered_map<
				SmartPointer<Client>,
				std::unordered_set<ColumnID>
			> client_map;
			
			
			//
			//	MISC.
			//
			
			
			std::function<void ()> save_callback;
			
			
			//
			//	SETTINGS
			//
			
			
			//	The world type to be used in generation
			String world_type;
			//	The amount of time the container will wait
			//	between checking columns for unloading
			Word unload_interval;
			
			
			//
			//	STATISTICS
			//
			
			
			//	Number of chunks generated
			std::atomic<Word> generated;
			//	Number of chunks loaded
			std::atomic<Word> loaded;
			//	Number of chunks populated
			std::atomic<Word> populated;
			//	Amount of time spent generating
			std::atomic<UInt64> generate_time;
			//	Amount of time spent loading
			std::atomic<UInt64> load_time;
			//	Amount of time spent populating
			std::atomic<UInt64> populate_time;
			
			
			//
			//	PRIVATE METHODS
			//
			
			
			//	Retrieves a column container.
			//
			//	If the requested column is not available, it
			//	will be loaded or generated.
			//
			//	The interest count on the column shall be
			//	incremented.
			//
			//	If population is requested, the column shall
			//	be populated before this function returns.
			SmartPointer<ColumnContainer> load (ColumnID, bool);
			inline bool set_block (BlockID, Block, SmartPointer<ColumnContainer>);
			
			
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			WorldContainer ();
			virtual Word Priority () const noexcept override;
			virtual const String & Name () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	An event invoked whenever an attempt to set
			 *	a block is made.
			 *
			 *	Only invoked if the target column has been
			 *	populated.
			 *
			 *	<B>Parameters:</B>
			 *
			 *	1.	The ID of the block which is being set.
			 *	2.	The details which are trying to be set
			 *		to the block given by the first parameter.
			 *
			 *	Returning \em false will block the attempt
			 *	to set the block, returning \em false will
			 *	allow it.
			 */
			Event<bool (BlockID, Block)> OnSet;
		
		
			/**
			 *	Gets the current state of a given column.
			 *
			 *	\param [in] id
			 *		The column whose state sholl be determined.
			 *	\param [in] acquire
			 *		\em true if interest in the column-in-question
			 *		should be expressed, \em false otherwise.
			 *		Defaults to \em false.  Interest shall never
			 *		be expressed in an unloaded column.
			 *
			 *	\return
			 *		The current state of the column specified by
			 *		\em id.
			 */
			ColumnState GetColumnState (ColumnID id, bool acquire=false) noexcept;
			/**
			 *	Ends interest in a given column.
			 *
			 *	If the column is not loaded, nothing happens.
			 *
			 *	If interest is ended in a column on which
			 *	interest was never acquired, the result is
			 *	undefined behaviour.
			 *
			 *	\param [in] id
			 *		The column in which the coller is no
			 *		longer interested.
			 */
			void EndInterest (ColumnID id) noexcept;
			/**
			 *	Sets the block at the position indicated
			 *	by \em id to be \em block.
			 *
			 *	\param [in] id
			 *		The position of the block to set.
			 *	\param [in] block
			 *		The block to set the block at \em id
			 *		to be equal to.
			 *	\param [in] release
			 *		If \em true, shall release interest in
			 *		the column which contains \em id.
			 *		Defaults to \em false.
			 *
			 *	\return
			 *		\em true if the block was successfully
			 *		set, \em false if the attempt to set it
			 *		was disallowed.  If the attempt to set
			 *		the block is disallowed, and \em release
			 *		was \em true, interest is still released.
			 */
			bool SetBlock (BlockID id, Block block, bool release=false);
			/**
			 *	Gets the block at the position indicated by
			 *	\em id.
			 *
			 *	\param [in] id
			 *		The position of the block to retrieve.
			 *	\param [in] acquire
			 *		If \em true, shall acquire interest in
			 *		the column which contains \em id.
			 *		Defaults to \em false.
			 *
			 *	\return
			 *		The block at the position given by
			 *		\em id.
			 */
			Block GetBlock (BlockID id, bool acquire=false);
			/**
			 *	Adds a specific client to a specific column,
			 *	which will be sent to that client.
			 *
			 *	The column may be added at once, generated or
			 *	loaded synchronously and then added, or
			 *	added at some point in the future.  This is
			 *	up to the implementation.
			 *
			 *	If the client is already added to the specified
			 *	column, nothing happens.
			 *
			 *	\param [in] id
			 *		The column to add the client to.
			 *	\param [in] client
			 *		The client to add to the column specified
			 *		by \em id.
			 */
			void Add (ColumnID id, SmartPointer<Client> client);
			/**
			 *	Removes a specific client from a specific
			 *	column.
			 *
			 *	The client will be sent a packet instructing them
			 *	to unload the column.
			 *
			 *	If the client is not added to the specified column,
			 *	nothing happens.
			 *
			 *	\param [in] id
			 *		The column to remove the client from.
			 *	\param [in] client
			 *		The client to remove from the column specified
			 *		by \em id.
			 */
			void Remove (ColumnID id, SmartPointer<Client> client);
			/**
			 *	Adds a provider for a specific dimension to the
			 *	world.
			 *
			 *	This is not thread safe and should not be called
			 *	after the module loading process.
			 *
			 *	\param [in] dimension
			 *		The dimension this provider can provide
			 *		columns for.
			 *	\param [in] world_type
			 *		The world type this provider can provide
			 *		columns for.
			 *	\param [in] provider
			 *		The provider.
			 *	\param [in] is_default
			 *		Whether this provider should overwrite the
			 *		default provider (which will be used if no
			 *		provider for a world type/dimension
			 *		combination can be found).  Defaults to
			 *		\em false.
			 */
			void Add (SByte dimension, String world_type, Provider provider, bool is_default=false);
			/**
			 *	Adds a populator.
			 *
			 *	During the population process, populators will be
			 *	called from the lowest \em priority to the highest, with
			 *	populators with the same \em priority called in
			 *	an indeterminate order.
			 *
			 *	This is not thread safe and should not be called
			 *	after the module loading process.
			 *
			 *	\param [in] priority
			 *		The priority of this populator.
			 *	\param [in] populator
			 *		The populator.
			 */
			void Add (Word priority, Populator populator);
	
	
	};
	
	
	/**
	 *	The Minecraft world.
	 */
	extern Nullable<WorldContainer> World;


}

/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <functional>
#include <utility>
#include <type_traits>


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
	 *	\cond
	 */


	class WorldLockInfo {
	
	
		public:
		
		
			WorldLockInfo () noexcept;
		
		
			//	The specific blocks that this lock
			//	desires exclusive access to
			std::unordered_set<BlockID> Blocks;
			//	The specific columns that this lock
			//	desires exclusive access to
			std::unordered_set<ColumnID> Columns;
			//	An optional barrier which blocking locks
			//	will wait on if they cannot acquire
			//	the lock immediately
			SmartPointer<Barrier> Wait;
			//	An optional callback which non-blocking
			//	locks will use to take whatever action
			//	they are acquiring the lock for
			std::function<void ()> Callback;
			//	Used to communicate ID in the case of
			//	sychronous lock acquisition, is set
			//	to nullptr otherwise.
			Word * ID;
			
	
	};
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	Allows per-block, per-column, and exclusive
	 *	locks to be acquired on the world.
	 */
	class WorldLock {
	
	
		private:
		
		
			//	Incrementing IDs for tasks
			Word id;
			
			
			//	Tasks or threads waiting to acquire
			//	a lock
			Vector<WorldLockInfo> waiting;
			//	Map from task ID to running task
			std::unordered_map<Word,WorldLockInfo> running;
			
			
			//	Whether exclusive lock is held/desired
			bool exclusive;
			//	Set of locked columns
			std::unordered_set<ColumnID> columns;
			//	Set of locked blocks
			std::unordered_set<BlockID> blocks;
			//	Set of columns that contain locked
			//	blocks
			std::unordered_set<ColumnID> enclosing_columns;
			
			
			//	Lock which guards data structures
			//	from unsynchronized access
			Mutex lock;
			
			
			//	Thread pool for asynchronous
			//	callbacks
			ThreadPool & pool;
			
			
			//	Checks to see if a given lock can be
			//	immediately acquired.
			//
			//	Requires that the mutex be held.
			inline bool check (const WorldLockInfo &);
			//	Synchronously acquires a given lock.
			inline Word acquire (WorldLockInfo);
			//	Asynchronously acquires a given lock.
			void acquire (WorldLockInfo, std::function<void ()>);
			
			
		public:
		
		
			WorldLock () = delete;
			WorldLock (const WorldLock &) = delete;
			WorldLock (WorldLock &&) = delete;
			WorldLock & operator = (const WorldLock &) = delete;
			WorldLock & operator = (WorldLock &&) = delete;
		
		
			/**
			 *	Creates a new WorldLock object.
			 *
			 *	\param [in] pool
			 *		The ThreadPool that the WorldLock
			 *		should use to invoke asynchronous
			 *		callbacks.
			 */
			WorldLock (ThreadPool & pool) noexcept;
		
		
			/**
			 *	Synchronously acquires a lock
			 *	on a certain block.
			 *
			 *	\param [in] block
			 *		The block on which a lock
			 *		is to be acquired.
			 *
			 *	\return
			 *		An ID which shall be used
			 *		to release the lock.
			 */
			Word Acquire (BlockID block);
			/**
			 *	Synchronously acquires a lock
			 *	on a number of blocks.
			 *
			 *	\param [in] blocks
			 *		A hash set which contains
			 *		all the blocks on which a
			 *		lock shall be acquired.
			 *
			 *	\return
			 *		An ID which shall be used
			 *		to release the lock.
			 */
			Word Acquire (std::unordered_set<BlockID> blocks);
			/**
			 *	Synchronously acquires a lock on
			 *	the entire world.
			 *
			 *	\return
			 *		An ID which shall be used to
			 *		release the lock.
			 */
			Word Acquire ();
			/**
			 *	Synchronously acquires a lock on a
			 *	certain column.
			 *
			 *	\param [in] column
			 *		The column on which a lock is to be
			 *		acquired.
			 *
			 *	\return
			 *		An ID which shall be used to release
			 *		the lock.
			 */
			Word Acquire (ColumnID column);
			/**
			 *	Synchronously acquires a lock on a
			 *	number of columns.
			 *
			 *	\param [in] columns
			 *		A hash set which contains all
			 *		the columns on which a lock
			 *		shall be acquired.
			 *
			 *	\return
			 *		An ID which shall be used to
			 *		release the lock.
			 */
			Word Acquire (std::unordered_set<ColumnID> columns);
			/**
			 *	Releases an acquired lock.
			 *
			 *	\param [in] task_id
			 *		The ID returned when the
			 *		lock was acquired.
			 */
			void Release (Word task_id);
			
			
			/**
			 *	Enqueues an asynchronous task to be
			 *	executed when an exclusive lock may
			 *	be acquired on a certain block.
			 *
			 *	\tparam T
			 *		The type of the callback to be
			 *		asynchronously executed.
			 *	\tparam Args
			 *		The types of the parameters to be
			 *		forwarded to the callback.
			 *
			 *	\param [in] block
			 *		The block on which a lock is to be
			 *		acquired.
			 *	\param [in] callback
			 *		The callback to be executed once the
			 *		lock is acquired.
			 *	\param [in] args
			 *		The arguments to forward to \em callback.
			 */
			template <typename T, typename... Args>
			void Enqueue (BlockID block, T && callback, Args &&... args) {
			
				WorldLockInfo info;
				info.Blocks.insert(block);
			
				acquire(
					std::move(info),
					std::bind(
						std::forward<T>(callback),
						std::forward<Args>(args)...
					)
				);
			
			}
			/**
			 *	Enqueues an asynchronous task to be
			 *	executed when an exclusive lock may
			 *	be acquired on a certain column.
			 *
			 *	\tparam T
			 *		The type of the callback to be
			 *		asynchronously executed.
			 *	\tparam Args
			 *		The types of the parameters to be
			 *		forwarded to the callback.
			 *
			 *	\param [in] column
			 *		The column on which a lock is to be
			 *		acquired.
			 *	\param [in] callback
			 *		The callback to be executed once the
			 *		lock is acquired.
			 *	\param [in] args
			 *		The arguments to forward to \em callback.
			 */
			template <typename T, typename... Args>
			void Enqueue (ColumnID column, T && callback, Args &&... args) {
			
				WorldLockInfo info;
				info.Columns.insert(column);
				
				acquire(
					std::move(info),
					std::bind(
						std::forward<T>(callback),
						std::forward<Args>(args)...
					)
				);
			
			}
			/**
			 *	Enqueues an asynchronous task to be
			 *	executed when an exclusive lock may
			 *	be acquired on a certain set of
			 *	columns.
			 *
			 *	\tparam T
			 *		The type of the callback to be
			 *		asynchronously executed.
			 *	\tparam Args
			 *		The types of the parameters to be
			 *		forwarded to the callback.
			 *
			 *	\param [in] columns
			 *		A hash set containing the columns on
			 *		which a lock is to be acquired.
			 *	\param [in] callback
			 *		The callback to be executed once the
			 *		lock is acquired.
			 *	\param [in] args
			 *		The arguments to forward to \em callback.
			 */
			template <typename T, typename... Args>
			void Enqueue (std::unordered_set<ColumnID> columns, T && callback, Args &&... args) {
			
				WorldLockInfo info;
				info.Columns=std::move(columns);
				
				acquire(
					std::move(info),
					std::bind(
						std::forward<T>(callback),
						std::forward<Args>(args)...
					)
				);
			
			}
			/**
			 *	Enqueues an asynchronous task to be
			 *	executed when an exclusive lock may
			 *	be acquired on a certain set of
			 *	blocks.
			 *
			 *	\tparam T
			 *		The type of the callback to be
			 *		asynchronously executed.
			 *	\tparam Args
			 *		The types of the parameters to be
			 *		forwarded to the callback.
			 *
			 *	\param [in] blocks
			 *		A hash set containing the blocks on
			 *		which a lock is to be acquired.
			 *	\param [in] callback
			 *		The callback to be executed once the
			 *		lock is acquired.
			 *	\param [in] args
			 *		The arguments to forward to \em callback.
			 */
			template <typename T, typename... Args>
			void Enqueue (std::unordered_set<BlockID> blocks, T && callback, Args &&... args) {
			
				WorldLockInfo info;
				info.Blocks=std::move(blocks);
				
				acquire(
					std::move(info),
					std::bind(
						std::forward<T>(callback),
						std::forward<Args>(args)...
					)
				);
			
			}
			/**
			 *	Enqueues an asynchronous task to be
			 *	executed when an exclusive lock may
			 *	be acquired on the entire world.
			 *
			 *	\tparam T
			 *		The type of the callback to be
			 *		asynchronously executed.
			 *	\tparam Args
			 *		The types of the parameters to be
			 *		forwarded to the callback.
			 *
			 *	\param [in] callback
			 *		The callback to be executed once the
			 *		lock is acquired.
			 *	\param [in] args
			 *		The arguments to forward to \em callback.
			 */
			template <typename T, typename... Args>
			typename std::enable_if<
				!(
					std::is_same<
						typename std::decay<T>::type,
						BlockID
					>::value ||
					std::is_same<
						typename std::decay<T>::type,
						ColumnID
					>::value ||
					std::is_same<
						typename std::decay<T>::type,
						std::unordered_set<ColumnID>
					>::value ||
					std::is_same<
						typename std::decay<T>::type,
						std::unordered_set<BlockID>
					>::value
				)
			>::type Enqueue (T && callback, Args &&... args) {
			
				acquire(
					WorldLockInfo(),
					std::bind(
						std::forward<T>(callback),
						std::forward<Args>(args)...
					)
				);
			
			}
	
	
	};


	class WorldTask {
	
	
		private:
		
		
			//	The columns that the task is
			//	waiting on
			std::unordered_set<ColumnID> columns;
			//	The function that is waiting to
			//	be invoked
			std::function <void ()> func;
			//	The blocks that the task needs
			//	a lock on
			Vector<Coordinates> coords;
	
	
		public:
		
		
			
	
	
	};


	class ColumnContainer {
	
	
		private:
		
		
			//	The column's data
			SmartPointer<Column> column;
			//	Column lock
			RWLock column_lock;
			//	List of players who have or
			//	want this column
			std::unordered_set<Client *> clients;
			//	Number of tasks either waiting on
			//	or actively using this column
			std::atomic<Word> tasks;
	
	
	};
	
	
	/**
	 *	Contains the Minecraft world.
	 */
	//extern Nullable<WorldContainer> World;


}

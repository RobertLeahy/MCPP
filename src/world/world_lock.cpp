#include <world/world.hpp>


namespace MCPP {


	inline bool WorldLock::check (const WorldLockInfo & info) {
	
		//	If the exclusive lock is desired,
		//	fail at once
		if (exclusive) return false;
		
		//	Whether the lock can be immediately
		//	acquired.
		bool can_acquire=true;
		
		//	Is this lock exclusive?
		if (
			(info.Blocks.size()==0) &&
			(info.Columns.size()==0)
		) {
		
			//	YES
			
			//	Can the exclusive lock be
			//	acquired?
			if (!(
				!exclusive &&
				(columns.size()==0) &&
				(blocks.size()==0)
			)) can_acquire=false;
			
			exclusive=true;
			
			return can_acquire;
		
		}
		
		//	Check all blocks in this lock
		for (const auto & block : info.Blocks) {
		
			//	Is a lock held on this block?
			//
			//	If so we cannot acquire the lock.
			if (blocks.find(block)==blocks.end()) blocks.insert(block);
			else can_acquire=false;
			
			//	Get the column which encloses this
			//	block
			ColumnID column=block.ContainedBy();
			
			//	Is there a lock on that column?
			//
			//	If so we cannot acquire the lock
			if (columns.find(column)!=columns.end()) can_acquire=false;
			
			//	Make sure we add this column to the set
			//	of enclosing columns
			enclosing_columns.insert(column);
		
		}
		
		//	Check all columns in this lock
		for (const auto & column : info.Columns) {
		
			//	Is a lock held on this column?
			//
			//	IF so we cannot acquire the lock.
			if (columns.find(column)==columns.end()) columns.insert(column);
			else can_acquire=false;
			
			//	Is a lock held on a block within this
			//	column?
			//
			//	If so we cannot acquire the lock
			if (enclosing_columns.find(column)!=enclosing_columns.end()) can_acquire=false;
		
		}
		
		return can_acquire;
	
	}
	
	
	inline Word WorldLock::acquire (WorldLockInfo info) {
	
		Word task_id;
		SmartPointer<Barrier> wait;
		
		lock.Acquire();
		
		try {
		
			if (check(info)) {
			
				//	We may proceed at once
				
				//	Get a unique ID
				while (running.find(id)!=running.end()) ++id;
				
				//	Add to map of running tasks
				running.emplace(
					id,
					std::move(info)
				);
				
				//	Prepare task ID for return
				task_id=id;
			
			} else {
			
				//	Prepare a barrier to wait on
				wait=SmartPointer<Barrier>::Make(2);
				
				info.Wait=wait;
				info.ID=&task_id;
				
				waiting.Add(std::move(info));
			
			}
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
		
		//	Wait if necessary
		if (!wait.IsNull()) wait->Enter();
		
		//	Return task ID that can be used
		//	to release this lock
		return task_id;
	
	}
	
	
	void WorldLock::acquire (WorldLockInfo info, std::function<void ()> callback) {
	
		//StdOut << "Acquire" << Newline;
	
		Word task_id;
		bool run;
		
		lock.Acquire();
		
		try {
		
			if ((run=check(info))) {
			
				//StdOut << "EXECUTE NOW!" << Newline;
			
				//	We may proceed at once
				
				//	Get a unique ID
				while (running.find(id)!=running.end()) ++id;
				
				//	Add to map of running tasks
				running.emplace(
					id,
					std::move(info)
				);
				
				//	Prepare task ID for call
				task_id=id;
			
			} else {
			
				//	Enqueue
				
				info.Callback=std::move(callback);
				
				waiting.Add(std::move(info));
			
			}
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
		
		//	Run if possible
		if (run) {
		
			pool.Enqueue(
				[this] (std::function<void ()> callback, Word task_id) {
				
					try {	callback();	} catch (...) {	}
					
					Release(task_id);
				
				},
				std::move(callback),
				task_id
			);
		
		}
	
	}
	
	
	Word WorldLock::Acquire (BlockID block) {
	
		WorldLockInfo info;
		info.Blocks.insert(block);
		
		return acquire(std::move(info));
	
	}
	
	
	Word WorldLock::Acquire (std::unordered_set<BlockID> blocks) {
	
		WorldLockInfo info;
		info.Blocks=std::move(blocks);
		
		return acquire(std::move(info));
	
	}
	
	
	Word WorldLock::Acquire () {
	
		return acquire(WorldLockInfo());
	
	}
	
	
	Word WorldLock::Acquire (ColumnID column) {
	
		WorldLockInfo info;
		info.Columns.insert(column);
		
		return acquire(std::move(info));
	
	}
	
	
	Word WorldLock::Acquire (std::unordered_set<ColumnID> columns) {
	
		WorldLockInfo info;
		info.Columns=std::move(columns);
		
		return acquire(std::move(info));
	
	}
	
	
	void WorldLock::Release (Word task_id) {
	
		lock.Acquire();
		
		try {
		
			//	Find task identified by task_id
			auto iter=running.find(task_id);
			
			//	If there is no such task, the
			//	task_id we were given is bogus,
			//	simply end
			if (iter==running.end()) {
			
				lock.Release();
				
				return;
			
			}
			
			//	Remove the running task
			running.erase(iter);
			
			//	Reset locks
			exclusive=false;
			columns.clear();
			blocks.clear();
			enclosing_columns.clear();
			
			//	Repopulate locks held by
			//	running tasks
			for (const auto & pair : running) check(pair.second);
			
			//	Repopulate and launch tasks as
			//	applicable
			
			//	Tasks we launched and which must
			//	be deleted from waiting
			Vector<Word> del;
			
			for (Word i=0;i<waiting.Count();++i) {
			
				//	Can task be run?
				if (check(waiting[i])) {
				
					//	YES
					
					//	Queue up for deletion
					del.Add(i);
					
					//	Generate a task ID
					while (running.find(id)!=running.end()) ++id;
					
					//	Synchronous?
					if (!waiting[i].Wait.IsNull()) {
					
						//	YES
						
						//	Write through so that the waiting
						//	thread has the task ID
						*waiting[i].ID=id;
						
						//	Extract the barrier
						auto wait=std::move(waiting[i].Wait);
						
						//	Add it to the list of running tasks
						running.emplace(
							id,
							std::move(waiting[i])
						);
						
						//	Release the waiting thread
						wait->Enter();
					
					//	Asynchronous?
					} else if (waiting[i].Callback) {
					
						//	YES
						
						//	Dispatch a thread
						pool.Enqueue(
							[this] (std::function<void ()> callback, Word task_id) {
							
								try {	callback();	} catch (...) {	}
								
								Release(task_id);
							
							},
							std::move(waiting[i].Callback),
							id
						);
						
						//	Add it to the list of running tasks
						running.emplace(
							id,
							std::move(waiting[i])
						);
					
					}
				
				}
			
			}
			
			//	Purge tasks that were run
			for (Word i=del.Count();(i--)>0;) waiting.Delete(del[i]);
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
	
	}
	
	
	WorldLock::WorldLock (ThreadPool & pool) noexcept : id(0), exclusive(false), pool(pool) {	}


}

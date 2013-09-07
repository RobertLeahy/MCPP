#include <world/world.hpp>


namespace MCPP {


	static const String maintenance_error("Error in world maintenance");
	static const String end_maintenance("Finished world maintenance, took {0}ns, saved {1}, unloaded {2}");
	static const String end_save("Saved column {0} - {1} bytes in {2}ns");
	static const String unload("Unloading column {0}");


	void WorldContainer::maintenance () {
	
		try {
		
			bool is_verbose=RunningServer->IsVerbose(verbose);
			Word this_saved=0;
			Word this_unloaded=0;
		
			//	Start maintenance cycle timer
			Timer timer(Timer::CreateAndStart());
	
			//	Get a list of all the loaded
			//	columns.
			//
			//	If we maintain the lock we'll
			//	hold all other threads up, but
			//	if we dispatch an asynchronous
			//	callback per worker we could wind
			//	up clogging the thread pool
			//	waiting for synchronous I/O.
			Vector<SmartPointer<ColumnContainer>> columns;
			lock.Execute([&] () {	for (auto & pair : world) columns.Add(pair.second);	});
			
			//	Loop and perform maintenance on
			//	each column
			for (auto & column : columns) {
			
				//	Get the key which will be associated
				//	with this column in the backing store
				String k(key(*column));
				
				//	Lock so the column is no longer
				//	modified
				column->Acquire();
				
				try {
				
					//	Save if necessary
					if (column->Dirty()) {
					
						Timer timer(Timer::CreateAndStart());
					
						RunningServer->Data().SaveBinary(
							k,
							column->Get(),
							column->Size()
						);
						
						column->CompleteSave();
						
						auto elapsed=timer.ElapsedNanoseconds();
						save_time+=elapsed;
						++saved;
						++this_saved;
						
						//	Log if applicable
						if (is_verbose) RunningServer->WriteLog(
							String::Format(
								end_save,
								column->ToString(),
								column->Size(),
								elapsed
							),
							Service::LogType::Debug
						);
					
					}
					
					//	See if we can unload this
					//	column
					//
					//	It is safe to acquire this lock
					//	because no other task tries to
					//	acquire column locks while holding
					//	the world lock, they merely increment
					//	the interest count (atomic) and move
					//	on
					lock.Execute([&] () {
					
						if (column->CanUnload()) {
						
							world.erase(column->ID());
							
							++unloaded;
							++this_unloaded;
							
							//	TODO: Fire event
							
							if (is_verbose) RunningServer->WriteLog(
								String::Format(
									unload,
									column->ToString()
								),
								Service::LogType::Debug
							);
						
						}
					
					});
				
				} catch (...) {
				
					column->Release();
					
					throw;
				
				}
				
				column->Release();
			
			}
			
			auto elapsed=timer.ElapsedNanoseconds();
			maintenance_time+=elapsed;
			++maintenances;
			
			//	Log if applicable
			if (is_verbose) RunningServer->WriteLog(
				String::Format(
					end_maintenance,
					elapsed,
					this_saved,
					this_unloaded
				),
				Service::LogType::Debug
			);
			
		//	If something goes wrong during maintenance
		//	we panic and kill the server.
		} catch (...) {
		
			RunningServer->WriteLog(
				maintenance_error,
				Service::LogType::Error
			);
			
			RunningServer->Panic();
			
			throw;
		
		}
	
	}


}

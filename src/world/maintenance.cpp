#include <world/world.hpp>


namespace MCPP {


	static const String maintenance_error("Error during world maintenance");
	static const String end_maintenance("Finished world maintenance, took {0}ns, saved {1}, unloaded {2}");
	static const String unload("Unloaded column {0}");


	void World::maintenance () {
	
		try {
		
			bool is_verbose=Server::Get().IsVerbose(verbose);
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
			lock.Execute([&] () {
			
				columns=Vector<SmartPointer<ColumnContainer>>(world.size());
			
				for (auto & pair : world) columns.Add(pair.second);
				
			});
			
			maintenance_lock.Execute([&] () {
			
				//	Loop and perform maintenance on
				//	each column
				for (auto & column : columns) {
				
					//	Save
					if (save(*column)) ++this_saved;
					
					//	See if we can unload
					
					bool unloaded=false;
					
					column->Acquire();
					
					if (column->CanUnload()) {
					
						//	We can unload
						
						unloaded=true;
						
						lock.Execute([&] () {	world.erase(column->ID());	});
						
					}
					
					column->Release();
					
					if (unloaded) {
						
						++unloaded;
						++this_unloaded;
						
						//	TODO: Fire event
						
						if (is_verbose) Server::Get().WriteLog(
							String::Format(
								unload,
								column->ToString()
							),
							Service::LogType::Debug
						);
						
					}
				
				}
				
			});
			
			auto elapsed=timer.ElapsedNanoseconds();
			maintenance_time+=elapsed;
			++maintenances;
			
			//	Log if applicable
			if (is_verbose) Server::Get().WriteLog(
				String::Format(
					end_maintenance,
					elapsed,
					this_saved,
					this_unloaded
				),
				Service::LogType::Debug
			);
			
			//	Prepare for the next maintenance
			//	cycle
			Server::Get().Pool().Enqueue(
				maintenance_interval,
				[this] () {	maintenance();	}
			);
			
		//	If something goes wrong during maintenance
		//	we panic and kill the server.
		} catch (...) {
		
			try {
		
				Server::Get().WriteLog(
					maintenance_error,
					Service::LogType::Error
				);
				
			//	We don't care if this fails,
			//	we're panicking anyway
			} catch (...) {	}
			
			Server::Get().Panic();
			
			throw;
		
		}
	
	}


}

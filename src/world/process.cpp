#include <world/world.hpp>


namespace MCPP {


	static const String end_load("Loaded {0} {1} - {2} bytes in {3}ns");
	static const String populated_str("populated");
	static const String generated_str("generated");
	static const String end_load_miss("Attempted to load {0} but it was not present - took {1}ns");
	static const String end_generate("Generated {0} - took {1}ns");
	static const String end_populate("Populated {0} - took {1}ns");
	static const String processing_error("Error while processing {0}");


	void World::process (ColumnContainer & column) {
	
		try {
	
			//	The state the column is currently in
			ColumnState curr=column.GetState();
			
			//	Are we logging debug information?
			bool is_verbose=Server::Get().IsVerbose(verbose);
			
			bool dirty;
			//	Process at least once -- this function
			//	would not be called if processing did
			//	not need to be performed
			do {
			
				dirty=true;
			
				//	Branch based on the column's current
				//	state
				switch (curr) {
				
				
					//	LOADING
					case ColumnState::Loading:{
					
						Timer timer(Timer::CreateAndStart());
						
						//	Loading does not change the column's
						//	logical state
						dirty=false;
						
						//	Load from backing store
						curr=load(column);
						
						//	Stats
						auto elapsed=timer.ElapsedNanoseconds();
						load_time+=elapsed;
						++loaded;
						
						//	Log if necessary
						if (is_verbose) Server::Get().WriteLog(
							(
								(curr==ColumnState::Generating)
									//	We missed on the load --
									//	nothing was loaded
									?	String::Format(
											end_load_miss,
											column.ToString(),
											elapsed
										)
									//	Load hit something -- we either
									//	loaded a populated or generated
									//	column
									:	String::Format(
											end_load,
											column.ToString(),
											(curr==ColumnState::Populated) ? populated_str : generated_str,
											ColumnContainer::Size,
											elapsed
										)
							),
							Service::LogType::Debug
						);
						
						//	We need to send the column to clients
						//	if it has become populated
						if (curr==ColumnState::Populated) goto populated;
						
					}break;
					
					
					//	GENERATING
					case ColumnState::Generating:{
					
						Timer timer(Timer::CreateAndStart());
						
						//	Generate column by invoking
						//	world generator
						generate(column);
						curr=ColumnState::Generated;
						
						//	Stats
						auto elapsed=timer.ElapsedNanoseconds();
						generate_time+=elapsed;
						++generated;
						
						//	Log if necessary
						if (is_verbose) Server::Get().WriteLog(
							String::Format(
								end_generate,
								column.ToString(),
								elapsed
							),
							Service::LogType::Debug
						);
						
					}break;
					
					
					//	GENERATED
					case ColumnState::Generated:
						//	No processing needed at this
						//	stage, just advance
						curr=ColumnState::Populating;
						//	This does not change the column's
						//	logical state
						dirty=false;
						break;
					
					
					//	POPULATING
					case ColumnState::Populating:{
					
						Timer timer(Timer::CreateAndStart());
						
						//	Populate column by invoking
						//	populators
						populate(column);
						curr=ColumnState::Populated;
						
						//	Stats
						auto elapsed=timer.ElapsedNanoseconds();
						populate_time+=elapsed;
						++populated;
						
						//	Log if necessary
						if (is_verbose) Server::Get().WriteLog(
							String::Format(
								end_populate,
								column.ToString(),
								elapsed
							),
							Service::LogType::Debug
						);
					
					//	This scope is a neat
					//	trick to avoid goto jumping
					//	over initializations
					}{
						
						//	If a column is loaded into
						//	the populated state, control
						//	is sent here so the column is
						//	sent to attached users
						populated:
						
						column.Send();
						
						//	TODO: Fire event
						
					}break;
					
					
					//	POPULATED
					case ColumnState::Populated:
						//	This shouldn't happen, but
						//	if it does, we're done, just
						//	return
						return;
						
				
				}
			
			} while (!column.SetState(
				curr,
				dirty,
				Server::Get().Pool()
			));
		
		//	Any error leaves the world
		//	in an inconsistent state and
		//	is therefore irrecoverable
		} catch (...) {
		
			try {
			
				Server::Get().WriteLog(
					String::Format(
						processing_error,
						column.ToString()
					),
					Service::LogType::Error
				);
			
			//	We're already panicking,
			//	can't do anything about this
			} catch (...) {	}
		
			Server::Get().Panic();
		
			throw;
		
		}
	
	}


}

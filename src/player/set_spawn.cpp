#include <player/player.hpp>


namespace MCPP {


	static const String spawn_key("spawn");
	static const String spawn_template("{0},{1},{2}");
	static const Regex spawn_parse("(?<=(?:,|^)\\s*)\\-?\\d+(?=\\s*(?:,|$))");
	static const Int32 spawn_x_default=0;
	static const Int32 spawn_y_default=256;
	static const Int32 spawn_z_default=0;
	static const String prepare_spawn_area("Preparing spawn area ({0} columns)");
	static const String prepare_spawn_area_progress("Preparing spawn area - {0}% ({1}/{2} columns)");
	static const String prepare_spawn_area_complete("Prepared spawn area ({0} columns), took {1}ns ({2}ns per column)");
	
	
	void Players::set_spawn (std::function<void ()> then) {
	
		auto setting=Server::Get().Data().GetSetting(spawn_key);
		
		if (setting.IsNull()) goto default_vals;
		
		{
		
			auto matches=spawn_parse.Matches(*setting);
				
			if (
				(matches.Count()==3) &&
				matches[0].Value().ToInteger(&spawn_x) &&
				matches[1].Value().ToInteger(&spawn_y) &&
				matches[2].Value().ToInteger(&spawn_z)
			) goto end;
			
		}
		
		default_vals:
		
		spawn_x=spawn_x_default;
		spawn_y=spawn_y_default;
		spawn_z=spawn_z_default;
		
		end:
		
		get_spawn(std::move(then));
	
	}


	void Players::set_spawn (Int32 x, Int32 y, Int32 z, bool initial) {
	
		//	Set the spawn to the new
		//	location
		spawn_lock.Write([&] () mutable {
		
			spawn_x=x;
			spawn_y=y;
			spawn_z=z;
		
		});
		
		
	
	}
	
	
	void Players::get_spawn (std::function<void ()> then) {
	
		spawn_lock.Read();
		Int32 x=spawn_x;
		Int32 z=spawn_z;
		SByte dimension=spawn_dimension;
		spawn_lock.CompleteRead();
	
		interested_lock.Acquire();
		while (interested_locked) interested_wait.Sleep(interested_lock);
		interested_locked=true;
		interested_lock.Release();
		
		try {
		
			//	Remove interest on all columns
			//	we were previously interested in
			for (auto & id : interested) World::Get().EndInterest(id);
			
			//	No longer holding interest on
			//	any columns
			interested.Clear();
			
			//	Prepare a list of all columns
			//	we need to load/express interest
			//	in
			
			for (
				Int32 curr_x=x-static_cast<Int32>(view_distance);
				curr_x<=(x+static_cast<Int32>(view_distance));
				++curr_x
			) for (
				Int32 curr_z=z-static_cast<Int32>(view_distance);
				curr_z<=(z+static_cast<Int32>(view_distance));
				++curr_z
			) interested.Add(ColumnID{curr_x,curr_z,dimension});
			
			//	Log
			Server::Get().WriteLog(
				String::Format(
					prepare_spawn_area,
					interested.Count()
				),
				Service::LogType::Information
			);
			
			//	Get setup for asynchronous callbacks
			interested_count=0;
			interest_timer=Timer::CreateAndStart();
			
			//	Start the asynchronous callbacks
			//	going
			for (
				Word i=0;
				(i<cm->Maximum()) &&
				(i<interested.Count());
				++i
			) cm->Enqueue([=] () mutable {	express_interest(i,std::move(then));	});
			
		} catch (...) {
		
			interested_lock.Acquire();
			interested_locked=false;
			interested_wait.WakeAll();
			interested_lock.Release();
			
			try {
			
				Server::Get().Panic();
			
			} catch (...) {	}
			
			throw;
		
		}
	
	}
	
	
	void Players::express_interest (
		Word i,
		std::function<void ()> then
	) {
	
		try {
		
			//	Express interest
			World::Get().Interested(
				interested[i]
			);
			
			bool done;
			interested_lock.Execute([&] () mutable {
			
				++interested_count;
				
				//	Log
				Server::Get().WriteLog(
					String::Format(
						prepare_spawn_area_progress,
						(static_cast<Double>(interested_count)/interested.Count())*100,
						interested_count,
						interested.Count()
					),
					Service::LogType::Information
				);
				
				//	Are we done?
				if (interested_count==interested.Count()) {
				
					//	YES
					
					done=true;
					
					auto elapsed=interest_timer.ElapsedNanoseconds();
					
					//	Log
					Server::Get().WriteLog(
						String::Format(
							prepare_spawn_area_complete,
							interested.Count(),
							elapsed,
							(interested.Count()==0) ? 0 : (elapsed/interested.Count())
						),
						Service::LogType::Information
					);
					
					//	Dispatch continuation (if applicable)
					if (then) Server::Get().Pool().Enqueue(then);
					
					//	End lock
					interested_locked=false;
					interested_wait.WakeAll();
				
				} else {
				
					//	NO
					
					done=false;
				
				}
			
			});
			
			//	If we're not done, and if we can
			//	dispatch another callback, do so
			if (!done) {
			
				Word index=i+cm->Maximum();
				
				if (index<interested.Count()) cm->Enqueue(
					[=] () {	express_interest(index,std::move(then));	}
				);
			
			}
		
		} catch (...) {
		
			try {
			
				Server::Get().Panic();
			
			} catch (...) {	}
			
			throw;
		
		}
	
	}


}

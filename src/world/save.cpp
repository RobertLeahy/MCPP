#include <world/world.hpp>
#include <server.hpp>
#include <cstring>


namespace MCPP {


	static const String save_failed("Failed saving {0} after {1}ns");
	static const String end_save("Saved column {0} - {1} bytes in {2}ns");


	bool World::save (ColumnContainer & column) {
	
		//	Start timer
		Timer timer(Timer::CreateAndStart());
	
		column.Acquire();
		
		//	Only save if necessary
		if (!column.Dirty()) {
		
			column.Release();
			
			return false;
		
		}
		
		//	We copy the column so that
		//	other threads do not have
		//	to wait for the backing
		//	store save operation
		Byte buffer [ColumnContainer::Size];
		memcpy(
			buffer,
			column.Get(),
			ColumnContainer::Size
		);
		
		//	Column is no longer dirty
		column.Clean();
		
		column.Release();
		
		auto & server=Server::Get();
		
		//	Perform save
		try {
		
			server.Data().SaveBinary(
				key(column),
				buffer,
				ColumnContainer::Size
			);
		
		} catch (...) {
		
			try {
		
				server.WriteLog(
					String::Format(
						save_failed,
						column.ToString(),
						timer.ElapsedNanoseconds()
					),
					Service::LogType::Error
				);
				
			//	We don't care whether this
			//	actually happens or not
			} catch (...) {	}
			
			//	PANIC
			server.Panic();
			
			throw;
		
		}
		
		auto elapsed=timer.ElapsedNanoseconds();
		save_time+=elapsed;
		++saved;
		
		//	Log if applicable
		if (server.IsVerbose(verbose)) server.WriteLog(
			String::Format(
				end_save,
				column.ToString(),
				ColumnContainer::Size,
				elapsed
			),
			Service::LogType::Debug
		);
		
		return true;
	
	}


}

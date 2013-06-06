static const String mods_dir="mods";
static const String mod_dir_set="Setting modules directory to {0}";
static const String could_not_open_dir="Could not open modules directory";
static const String attempt_to_load="Attempting to load {0}";
static const String failed_to_load="Failed to load {0}";
static const String loaded="Loading {0}";
static const String identified="{0} identifies as \"{1}\"";
static const String does_not_identify="{0} does not identify";


inline void Server::load_mods () {

	//	Determine mod directory path
	String mod_dir=Path::Combine(
		Path::GetPath(
			File::GetCurrentExecutableFileName()
		),
		mods_dir
	);
	
	WriteLog(
		String::Format(
			mod_dir_set,
			mod_dir
		),
		Service::LogType::Information
	);
	
	//	Attempt to iterate directory
	IterateDirectory iter(mod_dir);
	
	Nullable<FileIterator> begin;
	Nullable<FileIterator> end;
	try {
	
		begin.Construct(iter.begin());
		end.Construct(iter.end());
	
	} catch (...) {
	
		WriteLog(
			could_not_open_dir,
			Service::LogType::Warning
		);
		
		return;
	
	}
	
	//	Build listing
	for (;*begin!=*end;++(*begin)) {
	
		//	Get file name
		String filename=(*begin)->Name();
	
		//	Get extension to see if it
		//	purports to be a library
		String ext(Path::GetFileExtension(filename));
		
		if (
			#ifdef ENVIRONMENT_WINDOWS
			ext.ToLower()=="dll"
			#else
			ext=="so"
			#endif
		) {
		
			//	Attempt to load
			WriteLog(
				String::Format(
					attempt_to_load,
					filename
				),
				Service::LogType::Information
			);
			
			Library lib;
			
			try {
			
				lib=Library(
					Path::Combine(
						mod_dir,
						filename
					)
				);
			
			} catch (...) {
			
				//	Loading failed
			
				WriteLog(
					String::Format(
						failed_to_load,
						filename
					),
					Service::LogType::Warning
				);
				
				//	Try next file
				continue;
			
			}
			
			WriteLog(
				String::Format(
					loaded,
					filename
				),
				Service::LogType::Information
			);
			
			//	Try and get identifier from mod
			const char * module_name=nullptr;
			
			try {
			
				module_name=lib.GetAddress<const char * (*) ()>("ModuleName")();
			
			//	Eat exceptions -- a mod doesn't need a name
			} catch (...) {	}
			
			String mod_identifier;
			if (module_name==nullptr) {
			
				mod_identifier=filename;
				
				WriteLog(
					String::Format(
						does_not_identify,
						filename
					),
					Service::LogType::Information
				);
			
			} else {
			
				mod_identifier=module_name;
				
				WriteLog(
					String::Format(
						identified,
						filename,
						mod_identifier
					),
					Service::LogType::Information
				);
			
			}
			
			//	Need the module object
			
			
		}
	
	}

}

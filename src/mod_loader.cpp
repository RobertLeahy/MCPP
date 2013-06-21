#include <mod_loader.hpp>
#include <utility>


static const String could_not_open_dir="Could not open modules directory";
static const String attempt_to_load="Attempting to load {0}";
static const String failed_to_load="Failed to load {0}";
static const String loaded="Loading {0}";
static const String bad_format="{0} does not specify a load function";
static const String bad_ptr="{0} failed to generate module structure";
static const String identified="{0} identifies as \"{1}\"";


namespace MCPP {


	ModuleLoader::ModuleLoader (String dir, LogType log) : dir(std::move(dir)), log(std::move(log)) {	}
	
	
	inline void ModuleLoader::destroy () noexcept {
	
		unload();
		
		mods.Clear();
	
	}
	
	
	inline void ModuleLoader::unload () noexcept {
	
		//	Enumerate loaded modules
		for (auto & mod : mods) {
		
			//	Attempt to find the address
			//	of the void Unload () method
			//	and call it
			try {
			
				mod.Item<2>().GetAddress<void (*) ()>("Unload")();
			
			} catch (...) {	}
		
		}
	
	}
	
	
	ModuleLoader::~ModuleLoader () noexcept {
	
		destroy();
	
	}
	
	
	void ModuleLoader::Unload () noexcept {
	
		unload();
	
	}
	
	
	void ModuleLoader::Load () {
	
		//	If there are loaded modules,
		//	unload them
		if (mods.Count()!=0) destroy();
		
		//	Attempt to iterate mod
		//	directory
		IterateDirectory iter(dir);
		
		Nullable<FileIterator> begin;
		Nullable<FileIterator> end;
		try {
		
			begin.Construct(iter.begin());
			end.Construct(iter.end());
		
		//	Directory could not be
		//	opened
		} catch (...) {
		
			//	Log this
			log(
				could_not_open_dir,
				Service::LogType::Warning
			);
		
			//	End -- nothing else we
			//	can do
			return;
		
		}
		
		//	Iterate the directory to build
		//	a listing
		for (;*begin!=*end;++(*begin)) {
		
			//	Get the filename
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
			
				//	Attempt to load the module
				
				log(
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
							dir,
							filename
						)
					);
				
				} catch (...) {
				
					//	Loading failed
					
					log(
						String::Format(
							failed_to_load,
							filename
						),
						Service::LogType::Warning
					);
					
					//	Try the next file
					continue;
				
				}
				
				log(
					String::Format(
						loaded,
						filename
					),
					Service::LogType::Information
				);
				
				//	Try and get the Module pointer
				//	from the module
				Module * ptr;
				try {
				
					ptr=lib.GetAddress<Module * (*) ()>("Load")();
				
				} catch (...) {
				
					//	Couldn't get a pointer from
					//	it
					
					log(
						String::Format(
							bad_format,
							filename
						),
						Service::LogType::Warning
					);
					
					//	Try the next file
					continue;
				
				}
				
				//	Was no pointer returned?
				//
				//	That's an error condition
				if (ptr==nullptr) {
				
					log(
						String::Format(
							bad_ptr,
							filename
						),
						Service::LogType::Warning
					);
					
					//	Try the next file
					continue;
				
				}
				
				//	If anything goes wrong hereafter,
				//	we have a pointer that we're responsible
				//	for
				//
				//	Try and cache the address of the cleanup
				//	function in case we lose the library
				//	due to moving it (notably into the
				//	vector emplace insert).
				void (*cleanup) ()=nullptr;
				try {
				
					cleanup=lib.GetAddress<void (*) ()>("Unload");
				
				//	If we can't get it, that's fine, modules
				//	don't necessarily need cleanup code
				} catch (...) {	}
				
				try {
				
					//	Print out the module's name
					log(
						String::Format(
							identified,
							filename,
							ptr->Name()
						),
						Service::LogType::Information
					);
					
					//	Find where the module should go
					//	within the list of modules
					Word priority=ptr->Priority();
					Word insert=0;
					
					bool found=false;
					for (Word i=0;i<mods.Count();++i) {
					
						//	Check the priority of this element,
						//	compare it to this priority,
						//	and set this as the insertion point
						//	if it's greater or equal
						if (mods[i].Item<1>()->Priority()>=priority) {
						
							insert=i;
							
							found=true;
							
							break;
						
						}
					
					}
					
					//	If we didn't find an insertion point,
					//	insert at the end
					if (!found) insert=mods.Count();
					
					//	Create tuple in place
					mods.Emplace(
						insert,
						std::move(filename),
						ptr,
						std::move(lib)
					);
					
				} catch (...) {
				
					//	Try and dispose of the pointer
					try {
					
						if (cleanup!=nullptr) cleanup();
					
					} catch (...) {	}
					
					//	Re-throw, an exception shouldn't
					//	be thrown therefore it's fatal
					//	(probably memory not being allocated
					//	inside a string or vector, not good)
					throw;
				
				}
			
			}
		
		}
	
	}
	
	
	void ModuleLoader::Install () {
	
		//	Loop for each loaded module
		//	they're already in the correct
		//	order
		for (auto & mod : mods) {
		
			//	Attempt to install
			mod.Item<1>()->Install();
		
		}
	
	}


}

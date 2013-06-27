#include <mod_loader.hpp>
#include <utility>


#ifdef ENVIRONMENT_POSIX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <cstring>
#include <stdexcept>
#endif


static const String could_not_open_dir="Could not open modules directory";
static const String attempt_to_load="Attempting to load {0}";
static const String failed_to_load="Failed to load {0}";
static const String loaded="Loading {0}";
static const String bad_format="{0} does not specify a load function";
static const String bad_ptr="{0} failed to generate module structure";
static const String identified="{0} identifies as \"{1}\"";


namespace MCPP {


	ModuleLoader::ModuleLoader (String dir, LogType log) : dir(std::move(dir)), log(std::move(log)) {	}
	
	
	//	There's an issue with the module
	//	loading/management scheme vis-a-vis
	//	dlsym on Linux.
	//
	//	On Windows GetAddress only returns an
	//	address inside the target module,
	//	whereas on Linux dlsym will do a
	//	tree-like search through that module
	//	and all its dependencies.
	//
	//	This is an issue because modules can
	//	have sub-modules, which depend on that
	//	module.
	//
	//	Example:
	//
	//	We load foo.so, whidh loads bar.so.
	//
	//	bar.so depends on foo.so.
	//
	//	foo.so tries to lookup "Cleanup" in
	//	bar.so, but bar.so does not have a
	//	cleanup action, and therefore does
	//	not contain that symbol.
	//
	//	On Windows -- using GetAddress --
	//	this would throw, indicating the
	//	desired symbol was not present.
	//
	//	On Linux however, this will branch
	//	into foo.so, find "Cleanup" there,
	//	and return the address of that.
	//
	//	This is disastrous as this will lead
	//	to infinite recursion.
	//
	//	Therefore we have implement a workaround
	//	on Linux to check to make sure the
	//	address of the target symbol actually
	//	comes from the correct module file.
	//
	//	Credit goes to greymerk for finding/thinking
	//	of this workaround.
	template <typename T>
	T ModuleLoader::load (const String & symbol, const Library & lib, const String &
		#ifdef ENVIRONMENT_POSIX
		filename
		#endif
	) {
	
		#ifdef ENVIRONMENT_WINDOWS
		
		return lib.GetAddress<T>(symbol);
		
		#else
		
		union {
			T addr;
			void * addr_ptr;
		};
		
		//	Get the address, if that throws
		//	then we're free and clear
		addr=lib.GetAddress<T>(symbol);
		
		//	Otherwise we need to do a sanity
		//	check to enforce the Windows behaviour
		
		//	Get the raw OS handle from the
		//	wrapper
		void * handle;
		memcpy(&handle,&lib,sizeof(Library));
		
		//	Get information about the module
		//	containing addr
		Dl_info info;
		if (dladdr(
			addr_ptr,
			&info
		)==0) throw std::runtime_error(
			dlerror()
		);
		
		//	Check to see if that's this module
		if (
			UTF8().Decode(
				reinterpret_cast<const Byte *>(
					info.dli_fname
				),
				reinterpret_cast<const Byte *>(
					info.dli_fname+strlen(info.dli_fname)
				)
			)!=Path::Combine(
				dir,
				filename
			)
		) throw std::runtime_error(nullptr);	//	Dummy exception
		
		//	Okay we're good
		return addr;
		
		#endif
	
	}
	
	
	inline void ModuleLoader::destroy () noexcept {
	
		unload();
		
		//	Attempt to call the "Cleanup" method
		//	in each mod
		for (auto & mod : mods) {
		
			try {
			
				load<void (*) ()>(
					"Cleanup",
					mod.Item<2>(),
					mod.Item<0>()
				)();
			
			} catch (...) {	}
		
		}
		
		mods.Clear();
	
	}
	
	
	inline void ModuleLoader::unload () noexcept {
	
		//	Enumerate loaded modules
		for (auto & mod : mods) {
		
			//	Attempt to find the address
			//	of the void Unload () method
			//	and call it
			try {
			
				load<void (*) ()>(
					"Unload",
					mod.Item<2>(),
					mod.Item<0>()
				)();
			
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
				
					ptr=load<Module * (*) ()>(
						"Load",
						lib,
						filename
					)();
				
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
				void (*unload) ()=nullptr;
				void (*cleanup) ()=nullptr;
				try {
					
					unload=load<void (*) ()>(
						"Unload",
						lib,
						filename
					);
					
				} catch (...) {	}
				
				try {
				
					cleanup=load<void (*) ()>(
						"Cleanup",
						lib,
						filename
					);
				
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
					
						if (unload!=nullptr) unload();
					
					} catch (...) {	}
					
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

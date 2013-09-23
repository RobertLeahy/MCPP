#include <mod_loader.hpp>
#include <system_error>
#include <type_traits>
#include <utility>


#ifdef ENVIRONMENT_WINDOWS
#include <windows.h>
#else
//	Make sure we get dladdr
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <cstring>
#include <stdexcept>
#endif


namespace MCPP {


	static const String could_not_open_dir="Could not open modules directory";
	static const String attempt_to_load="Attempting to load {0}";
	static const String failed_to_load="Failed to load {0}";
	static const String loaded="Loaded {0}";
	static const String bad_format="{0} does not specify a load function";
	static const String bad_ptr="{0} failed to generate module structure";
	static const String identified="{0} identifies as \"{1}\"";


	#ifdef ENVIRONMENT_WINDOWS
	
	
	//	Adds the appropriate directory
	//	to the search path (on Windows)
	inline void ModuleLoader::begin_load () const {
	
		//	Get OS string
		auto os_str=dir.ToOSString();
		
		//	Set the DLL search directory
		if (!SetDllDirectoryW(
			reinterpret_cast<LPCWSTR>(
				static_cast<Byte *>(
					os_str
				)
			)
		)) throw std::system_error(
			std::error_code(
				GetLastError(),
				std::system_category()
			)
		);
	
	}
	
	
	//	Removes the added directory from
	//	the search path
	inline void ModuleLoader::end_load () const {
	
		//	Remove the DLL search directory
		if (!SetDllDirectoryW(nullptr)) throw std::system_error(
			std::error_code(
				GetLastError(),
				std::system_category()
			)
		);
	
	}
	
	
	//	Loads a particular DLL and returns a
	//	handle to it, or throws on failure
	static void * load (const String & filename) {
	
		//	Get OS string
		auto os_str=filename.ToOSString();
		
		//	Attempt to load the DLL
		void * retr=reinterpret_cast<void *>(
			LoadLibraryW(
				reinterpret_cast<LPCWSTR>(
					static_cast<Byte *>(
						os_str
					)
				)
			)
		);
		
		//	Throw on error
		if (retr==nullptr) throw std::system_error(
			std::error_code(
				GetLastError(),
				std::system_category()
			)
		);
		
		return retr;
	
	}
	
	
	//	Unloads a particular DLL
	static void unload (void * handle) {
	
		if (!FreeLibrary(
			reinterpret_cast<HMODULE>(
				handle
			)
		)) throw std::system_error(
			std::error_code(
				GetLastError(),
				std::system_category()
			)
		);
	
	}
	
	
	//	Gets the address of a particular
	//	function from a particular loaded
	//	library
	template <typename T>
	T get (void * handle, const String & func, const String &) {
	
		//	Make sure assumptions hold
		static_assert(
			std::is_pointer<T>::value &&
			(sizeof(T)==sizeof(FARPROC)),
			"Cannot GetProcAddress of that type"
		);
	
		//	GetProcAddress does not have a
		//	Unicode version
		auto os_str=func.ToCString();
		
		//	Union allows warning-less
		//	"casting" between pointer-to-function
		//	and pointer-to-object, which
		//	Windows necessarily allows but
		//	C++ does not
		union {
			T out;
			FARPROC in;
		};
		
		//	Get address
		if ((in=GetProcAddress(
			reinterpret_cast<HMODULE>(handle),
			static_cast<char *>(os_str)
		))==nullptr) throw std::system_error(
			std::error_code(
				GetLastError(),
				std::system_category()
			)
		);
		
		return out;
	
	}
	
	
	#else
	
	
	//	Does nothing
	inline void ModuleLoader::begin_load () const noexcept {	}
	//	Does nothing
	inline void ModuleLoader::end_load () const noexcept {	}
	
	
	//	Loads a particular SO and returns
	//	a handle to it, or throws on failure
	static void * load (const String & filename) {
	
		//	Get OS string for filename
		auto os_str=filename.ToOSString();
		
		//	Attempt to load SO
		void * retr=dlsym(
			reinterpret_cast<char *>(
				static_cast<Byte *>(
					os_str
				)
			),
			RTLD_GLOBAL|RTLD_LAZY
		);
		
		//	Throw on error
		if (retr==nullptr) throw std::runtime_error(dlerror());
	
	}
	
	
	//	Unloads a particular SO
	static void unload (void * handle) {
	
		if (dlclose(handle)!=0) throw std::runtime_error(dlerror());
	
	}
	
	
	//	Gets the address of a particular function
	//	from a particular loaded library
	template <typename T>
	T get (void * handle, const String & func, const String & filename) {
	
		//	Make sure assumptions hold
		static_assert(
			std::is_pointer<T>::value &&
			(sizeof(T)==sizeof(void *)),
			"Cannot dlsym that type"
		);
	
		//	Get OS name
		auto os_str=filename.ToOSString();
		
		//	Union allows warning-less
		//	"casting" between pointer-to-function
		//	and pointer-to-object, which
		//	POSIX mandates but C++ does
		//	not
		union {
			T out;
			void * in;
		};
		
		//	Attempt to load
		if ((in=dlsym(
			handle,
			reinterpret_cast<char *>(
				static_cast<Byte *>(
					os_str
				)
			)
		))==nullptr) throw std::runtime_error(dlerror());
		
		//	On POSIX attempting to find a symbol
		//	through a module handle does NOT
		//	constrain the search to that module
		//	(as it does on Windows).
		//
		//	Rather it constrains the search to
		//	that module AND ALL ITS DEPENDENCIES,
		//	which can create problems (as this
		//	module could define a symbol with an
		//	identical name).
		//
		//	Therefore we implement a workaround
		//	to check to make sure the address of
		//	the located symbol actually comes
		//	from the correct module file.
		//
		//	Credit goes to greymerk for finding/thinking
		//	of this workaround.
		Dl_info info;
		if (dladdr(
			in,
			&info
		)==0) throw std::runtime_error(dlerror());
		
		//	Check to see if that's this module
		if (
			Path::GetFileName(
				UTF8().Decode(
					reinterpret_cast<const Byte *>(
						info.dli_fname
					),
					reinterpret_cast<const Byte *>(
						info.dli_fname+strlen(info.dli_fname)
					)
				)
			)!=Path::GetFileName(
				filename
			)
		) throw std::runtime_error(nullptr);	//	Dummy exception
		
		return out;
	
	}
	
	
	#endif
	
	
	inline void ModuleLoader::destroy () noexcept {
	
		//	Call unload function of all
		//	loaded modules
		unload_impl();
		
		//	Attempt to call the "Cleanup" method
		//	in each module
		for (auto & mod : mods) {
		
			try {
			
				get<void (*) ()>(
					mod.Item<2>(),
					"Cleanup",
					mod.Item<0>()
				)();
			
			//	If cleanup function doesn't
			//	exist, that's fine
			} catch (...) {	}
		
		}
		
		//	Unload each module
		for (auto & mod : mods) {
		
			try {
			
				unload(mod.Item<2>());
			
			//	If a module fails to be
			//	unloaded, we can't really
			//	do anything about it
			} catch (...) {	}
		
		}
		
		//	Clear list of modules
		mods.Clear();
	
	}
	
	
	inline void ModuleLoader::unload_impl () noexcept {
	
		//	Attempt to call "Unload" method in
		//	each module
		for (auto & mod : mods) {
		
			try {
			
				get<void (*) ()>(
					mod.Item<2>(),
					"Unload",
					mod.Item<0>()
				)();
			
			//	Modules don't necessarily have to
			//	specify an unload method
			} catch (...) {	}
		
		}
	
	}
	
	
	ModuleLoader::ModuleLoader (String dir, LogType log) : dir(std::move(dir)), log(std::move(log)) {	}
	
	
	ModuleLoader::~ModuleLoader () noexcept {
	
		destroy();
	
	}
	
	
	void ModuleLoader::Unload () noexcept {
	
		unload_impl();
	
	}
	
	
	inline void ModuleLoader::load_impl (String filename) {
	
		//	Attempt to load the module
		
		log(
			String::Format(
				attempt_to_load,
				filename
			),
			Service::LogType::Information
		);
		
		void * handle;
		try {
		
			//	On Windows the search directory
			//	is set, on Linux it is not
			handle=load(
				#ifdef ENVIRONMENT_WINDOWS
				filename
				#else
				Path::Combine(dir,filename)
				#endif
			);
		
		} catch (...) {
		
			//	Load failed
			
			log(
				String::Format(
					failed_to_load,
					filename
				),
				Service::LogType::Warning
			);
			
			//	Abandon this module
			return;
		
		}
		
		try {
		
			//	Module successfully mapped into our
			//	address space, log this
			
			log(
				String::Format(
					loaded,
					filename
				),
				Service::LogType::Information
			);
			
			//	Attempt to get Module pointer from
			//	module
			Module * ptr;
			try {
			
				ptr=get<Module * (*) ()>(
					handle,
					"Load",
					filename
				)();
			
			} catch (...) {
			
				//	Couldn't get pointer
				
				log(
					String::Format(
						bad_format,
						filename
					),
					Service::LogType::Information
				);
				
				//	Abandon this module
				
				unload(handle);
				
				return;
			
			}
			//	Was a pointer returned?
			//
			//	If not, that's an error
			if (ptr==nullptr) {
			
				log(
					String::Format(
						bad_ptr,
						filename
					),
					Service::LogType::Warning
				);
				
				//	Abandon this module
				
				unload(handle);
				
				return;
			
			}
			
			//	We're now responsible for the lifetime
			//	of the Module pointer
			try {
			
				//	Print module's name
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
				Word i=0;
				for (;
					(i<mods.Count()) &&
					(mods[i].Item<1>()->Priority()<priority);
					++i
				);
				
				//	Insert
				mods.Emplace(
					i,
					std::move(filename),
					ptr,
					handle
				);
			
			} catch (...) {
			
				try {
				
					get<void (*) ()>(
						handle,
						"Unload",
						filename
					)();
				
				} catch (...) {	}
				
				try {
				
					get<void (*) ()>(
						handle,
						"Cleanup",
						filename
					)();
				
				} catch (...) {	}
				
				throw;
			
			}
			
		} catch (...) {
		
			//	Free module if an error occurs
			unload(handle);
			
			throw;
		
		}
	
	}
	
	
	void ModuleLoader::Load () {
	
		//	If there are loaded modules,
		//	unload them
		destroy();
		
		//	Attempt to iterate module
		//	directory
		IterateDirectory iter(dir);
		Nullable<FileIterator> begin;
		Nullable<FileIterator> end;
		try {
		
			begin.Construct(iter.begin());
			end.Construct(iter.end());
		
		} catch (...) {
		
			log(
				could_not_open_dir,
				Service::LogType::Information
			);
			
			//	Nothing we can do
			return;
		
		}
		
		//	Begin the loading process
		begin_load();
		
		try {
		
			//	Iterate the directory
			for (;*begin!=*end;++(*begin)) {
			
				String filename=(*begin)->Name();
				
				//	Get extension and check
				//	if the file extension is
				//	the library file extension
				//	on this platform
				String ext(Path::GetFileExtension(filename));
				if (
					#ifdef ENVIRONMENT_WINDOWS
					//	Filenames on Windows are not
					//	case sensitive
					ext.ToLower()=="dll"
					#else
					ext=="so"
					#endif
				) load_impl(std::move(filename));
			
			}
		
		} catch (...) {	
		
			end_load();
			
			throw;
		
		}
		
		end_load();
	
	}
	
	
	void ModuleLoader::Install () {
	
		//	Loop for each loaded module
		//	as they're already in the
		//	correct order
		for (auto & mod : mods) mod.Item<1>()->Install();
	
	}
	
	
	ModuleLoaderInfo ModuleLoader::GetInfo () const {
	
		Vector<ModuleInfo> modules;
		
		for (const auto & t : mods) modules.Add(
			ModuleInfo{
				t.Item<0>(),
				t.Item<1>()
			}
		);
		
		return ModuleLoaderInfo{
			dir,
			std::move(modules)
		};
	
	}
	
	
}

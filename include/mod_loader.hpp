/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <typedefs.hpp>


namespace MCPP {
	
	
	/**
	 *	This class provides dynamic module-loading
	 *	functionality.
	 *
	 *	When created it is given a directory which
	 *	it shall search for module files (DLLs on
	 *	Windows and SOs on Linux).
	 *
	 *	When instructed to load it shall search the
	 *	directory-in-question and load each module
	 *	file it finds.
	 *
	 *	During the loading process, as each module
	 *	is loaded, the loader shall obtain the address
	 *	of and invoke the function with the signature
	 *	<B>Module * Load ()</B> in each module file.
	 *
	 *	This function is expected to return a pointer
	 *	to a Module object, or \em nullptr on failure.
	 *
	 *	As each module is loaded, the module objects
	 *	shall be placed into a tuple, with the filename
	 *	the module was loaded from as the first element,
	 *	and the pointer to the Module object as the
	 *	second.
	 *
	 *	When instructed to install, the mod loader shall
	 *	enumerate the modules in ascending order of
	 *	priority and shall call the Module::Install
	 *	function, which shall instruct the module to
	 *	install itself.
	 *
	 *	When the module loader is instructed to unload
	 *	the loaded modules, or when its destructor
	 *	is called, it shall again enumerate the loaded
	 *	modules, find the address of the function
	 *	in each matching the signature
	 *	<B>void Unload ()</B> and invoke this, which
	 *	shall be expected to properly clean up the
	 *	Module object created and returned by
	 *	<B>Module * Load ()</B>.
	 */
	class ModuleLoader {
	
	
		private:
		
		
			//	Collection of loaded modules
			Vector<
				Tuple<
					//	Filename the module
					//	was loaded from
					String,
					//	Pointer to the module
					//	object
					Module *,
					//	Handle to the dynamically
					//	loaded module
					Library
				>
			> mods;
			//	Directory to scan
			String dir;
			//	Callback function to use for logging
			LogType log;
			
			
			inline void unload () noexcept;
			inline void destroy () noexcept;
			
			
		public:
		
		
			ModuleLoader () = delete;
			ModuleLoader (const ModuleLoader &) = delete;
			ModuleLoader & operator = (const ModuleLoader &) = delete;
			
			
			/**
			 *	Creates a ModuleLoader which will
			 *	load modules from the given directory.
			 *
			 *	\param [in] dir
			 *		The directory from which modules
			 *		shall be loaded.
			 *	\param [in] log
			 *		A callback which shall be used to
			 *		log events from within the module
			 *		loader.
			 */
			ModuleLoader (String dir, LogType log);
			
			
			/**
			 *	Destroys the ModuleLoader, giving all
			 *	loaded modules an opportunity to clean
			 *	up their resources.
			 */
			~ModuleLoader () noexcept;
			
			
			/**
			 *	Attempts to install loaded mods.
			 *
			 *	Mods which throw exceptions upon
			 *	an attempt to load them have these
			 *	exceptions propagated.
			 */
			void Install ();
			
			
			/**
			 *	Unloads all loaded modules.
			 */
			void Unload () noexcept;
			
			
			/**
			 *	Loads all modules in the target
			 *	directory.
			 *
			 *	If called when modules are loaded
			 *	will first call Unload internally.
			 */
			void Load ();
	
	
	};


}

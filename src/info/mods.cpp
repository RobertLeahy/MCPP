#include <rleahylib/rleahylib.hpp>
#include <info/info.hpp>
#include <server.hpp>
#include <mod.hpp>
#include <mod_loader.hpp>
#include <algorithm>


using namespace MCPP;


static const String name("Loaded Module Information");
static const Word priority=1;
static const String identifier("mods");
static const String help("Displays loaded modules.");
static const String loader_banner("MODULE LOADER:");
static const String num_modules("Loaded Modules: ");
static const String modules_dir("Modules Directory: ");
static const String mods_banner("LOADED MODULES:");
static const String info_separator(": ");
static const String file_label("File: ");
static const String priority_label("Priority: ");
static const String data_separator(", ");


class ModsInfo : public Module, public InformationProvider {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Install () override {
		
			Information::Get().Add(this);
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			auto info=Server::Get().Loader().GetInfo();
			
			std::sort(
				info.Modules.begin(),
				info.Modules.end(),
				[] (const ModuleInfo & a, const ModuleInfo & b) {	return a.Mod->Name()<b.Mod->Name();	}
			);
			
			message	<<	ChatStyle::Bold
					<<	loader_banner
					<<	Newline
					<<	num_modules
					<<	ChatFormat::Pop
					<<	info.Modules.Count()
					<<	Newline
					<<	ChatStyle::Bold
					<<	modules_dir
					<<	ChatFormat::Pop
					<<	info.Directory
					<<	Newline
					<<	ChatStyle::Bold
					<<	mods_banner
					<<	ChatFormat::Pop;
					
			for (const auto & module : info.Modules) {
			
				message	<<	Newline
						<<	ChatStyle::Bold
						<<	module.Mod->Name()
						<<	info_separator
						<<	ChatFormat::Pop
						<<	file_label
						<<	module.Filename
						<<	data_separator
						<<	priority_label
						<<	module.Mod->Priority();
			
			}
		
		}


};


static Nullable<ModsInfo> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

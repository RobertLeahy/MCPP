#include <info/info.hpp>
#include <chat/chat.hpp>
#include <world/world.hpp>
#include <rleahylib/rleahylib.hpp>


using namespace MCPP;


static const String name("World Information");
static const Word priority=1;
static const String identifier("world");
static const String help("Displays information about the world container.");
static const String world_banner("WORLD:");


static const String load_label("Loads: ");
static const String load_time_label("Loading Time: ");
static const String load_time_avg_label("Loading Time (Average): ");


static const String generate_label("Generates: ");
static const String generate_time_label("Generating Time: ");
static const String generate_time_avg_label("Generating Time (Average): ");


static const String populate_label("Populates: ");
static const String populate_time_label("Populating Time: ");
static const String populate_time_avg_label("Populating Time (Average): ");


static const String save_label("Saves: ");
static const String save_time_label("Saving Time: ");
static const String save_time_avg_label("Saving Time (Average): ");


static const String unload_label("Unloads: ");


static const String maintenance_label("Maintenance Cycles: ");
static const String maintenance_time_label("Maintenance Cycle Time: ");
static const String maintenance_time_avg_label("Maintenance Cycle Time (Average): ");


static const String count_label("Loaded Columns: ");
static const String memory_label("Memory Use: ");


static inline UInt64 avg (UInt64 t, Word n) noexcept {

	return (n==0) ? 0 : (t/n);

}


static const String ns_template("{0}ns");


static inline String ns (UInt64 n) {

	return String::Format(ns_template,n);

}


static const String mb("megabyte");
static const String mb_p("megabytes");
static const String kb("kilobyte");
static const String kb_p("kilobytes");
static const String gb("gigabyte");
static const String gb_p("gigabytes");
static const String b("byte");
static const String b_p("bytes");
static const String memory_format_template("{0} {1}");


static inline String memory_format (Word n) {

	Double d=n;
	const String * suffix;
	if (n<1024) {
	
		suffix=&((d>1) ? b_p : b);
	
	} else if (n<(1024*1024)) {
	
		d/=1024;
		suffix=&((d>1) ? kb_p : kb);
	
	} else if (n<(1024*1024*1024)) {
	
		d/=1024*1024;
		suffix=&((d>1) ? mb_p : mb);
	
	} else {
	
		d/=1024*1024*1024;
		suffix=&((d>1) ? gb_p : gb);
	
	}

	return String::Format(
		memory_format_template,
		d,
		*suffix
	);

}


class WorldInfoProvider : public Module, public InformationProvider {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			Information::Get().Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			//	Get information from world
			//	container
			auto info=World::Get().GetInfo();
			
			//	Create output
			message	<<	ChatStyle::Bold
					<<	world_banner
					<<	ChatFormat::Pop
					<<	Newline
					
					//	Loaded/Loading
					<<	ChatStyle::Bold
					<<	load_label
					<<	ChatFormat::Pop
					<<	info.Loaded
					<<	Newline
					<<	ChatStyle::Bold
					<<	load_time_label
					<<	ChatFormat::Pop
					<<	ns(info.Loading)
					<<	Newline
					<<	ChatStyle::Bold
					<<	load_time_avg_label
					<<	ChatFormat::Pop
					<<	ns(avg(
							info.Loading,
							info.Loaded
						))
					<<	Newline
						
					//	Generated/Generating
					<<	ChatStyle::Bold
					<<	generate_label
					<<	ChatFormat::Pop
					<<	info.Generated
					<<	Newline
					<<	ChatStyle::Bold
					<<	generate_time_label
					<<	ChatFormat::Pop
					<<	ns(info.Generating)
					<<	Newline
					<<	ChatStyle::Bold
					<<	generate_time_avg_label
					<<	ChatFormat::Pop
					<<	ns(avg(
							info.Generating,
							info.Generated
						))
					<<	Newline
						
					//	Populated/Populating
					<<	ChatStyle::Bold
					<<	populate_label
					<<	ChatFormat::Pop
					<<	info.Populated
					<<	Newline
					<<	ChatStyle::Bold
					<<	populate_time_label
					<<	ChatFormat::Pop
					<<	ns(info.Populating)
					<<	Newline
					<<	ChatStyle::Bold
					<<	populate_time_avg_label
					<<	ChatFormat::Pop
					<<	ns(avg(
							info.Populating,
							info.Populated
						))
					<<	Newline
					
					//	Saved/Saving
					<<	ChatStyle::Bold
					<<	save_label
					<<	ChatFormat::Pop
					<<	info.Saved
					<<	Newline
					<<	ChatStyle::Bold
					<<	save_time_label
					<<	ChatFormat::Pop
					<<	ns(info.Saving)
					<<	Newline
					<<	ChatStyle::Bold
					<<	save_time_avg_label
					<<	ChatFormat::Pop
					<<	ns(avg(
							info.Saving,
							info.Saved
						))
					<<	Newline
					
					//	Unloaded
					<<	ChatStyle::Bold
					<<	unload_label
					<<	ChatFormat::Pop
					<<	info.Unloaded
					<<	Newline
					
					//	Maintenances/Maintaining
					<<	ChatStyle::Bold
					<<	maintenance_label
					<<	ChatFormat::Pop
					<<	info.Maintenances
					<<	Newline
					<<	ChatStyle::Bold
					<<	maintenance_time_label
					<<	ChatFormat::Pop
					<<	ns(info.Maintaining)
					<<	Newline
					<<	ChatStyle::Bold
					<<	maintenance_time_avg_label
					<<	ChatFormat::Pop
					<<	ns(avg(
							info.Maintaining,
							info.Maintenances
						))
					<<	Newline	
					
					//	Column count
					<<	ChatStyle::Bold
					<<	count_label
					<<	ChatFormat::Pop
					<<	info.Count
					<<	Newline
					
					//	Memory footprint
					<<	ChatStyle::Bold
					<<	memory_label
					<<	ChatFormat::Pop
					<<	memory_format(info.Size);
					
		}


};


static Nullable<WorldInfoProvider> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

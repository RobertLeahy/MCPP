#include <info/info.hpp>


static const String name("MCPP Information");
static const Word priority=1;
static const String identifier("mcpp");
static const String help("Display information about mcpp.dll.");
static const String mcpp_banner("MINECRAFT++:");
static const String compiled_by_template("Compiled by {0} on {1}");


class MCPPInfo : public Module, public InformationProvider {


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
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
		
			message	<<	ChatStyle::Bold
					<<	mcpp_banner
					<<	ChatFormat::Pop
					<<	Newline
					<<	String::Format(
							compiled_by_template,
							Server::Get().CompiledWith(),
							Server::Get().BuildDate()
						);
		
		}


};


static Nullable<MCPPInfo> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

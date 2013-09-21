#include <info/info.hpp>
#include <op/op.hpp>
#include <algorithm>


static const Word priority=1;
static const String name("Server Operators Information");
static const String identifier("ops");
static const String help("Lists server operators.");
static const String ops_banner("SERVER OPERATORS:");


class OpsInfo : public Module, public InformationProvider {


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
		
			//	Get the list of server operators
			auto ops=Ops::Get().List();
			
			//	Sort it
			std::sort(
				ops.begin(),
				ops.end()
			);
			
			//	Generate output
			message	<< ChatStyle::Bold
					<< ops_banner
					<< ChatFormat::Pop;
					
			for (const auto & s : ops) message << Newline << s;
		
		}


};


static Nullable<OpsInfo> module;


extern "C" {


	Module * Load () {
	
		module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

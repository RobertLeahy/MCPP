#include <blacklist/blacklist.hpp>
#include <info/info.hpp>
#include <mod.hpp>
#include <algorithm>


using namespace MCPP;


static const Word priority=1;
static const String name("Blacklist Information Provider");
static const String help("Displays the blacklist.");
static const String identifier("blacklist");
static const String blacklist_banner("BLACKLIST:");


class BlacklistInfoProvider : public Module, public InformationProvider {


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
		
			auto info=Blacklist::Get().GetInfo();
			
			message	<<	ChatStyle::Bold
					<<	blacklist_banner
					<<	ChatFormat::Pop;
					
			std::sort(
				info.Ranges.begin(),
				info.Ranges.end()
			);
			
			for (auto & range : info.Ranges) message << Newline << String(range);
		
		}


};


INSTALL_MODULE(BlacklistInfoProvider)

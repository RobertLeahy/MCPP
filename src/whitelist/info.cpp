#include <chat/chat.hpp>
#include <info/info.hpp>
#include <whitelist/whitelist.hpp>
#include <mod.hpp>
#include <algorithm>
#include <utility>


using namespace MCPP;


static const String name("Whitelist Information");
static const Word priority=1;
static const String identifier("whitelist");
static const String help("Displays information about the whitelist.");
static const String whitelist_banner("WHITELIST:");
static const String enabled("Enabled: ");
static const String whitelist("Whitelist:");
static const String true_string("Yes");
static const String false_string("No");


class WhitelistInfo : public Module, public InformationProvider {


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
		
			auto & whitelist=Whitelist::Get();
		
			message	<<	ChatStyle::Bold
					<<	whitelist_banner
					<<	Newline
					<<	enabled
					<<	ChatFormat::Pop
					<<	(whitelist.Enabled() ? true_string : false_string)
					<<	Newline
					<<	ChatStyle::Bold
					<<	::whitelist
					<<	ChatFormat::Pop;
					
			auto info=whitelist.GetInfo();
			
			std::sort(info.begin(),info.end());
			
			for (auto & str : info) message << Newline << std::move(str);
		
		}


};


INSTALL_MODULE(WhitelistInfo)

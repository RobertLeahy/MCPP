#include <ban/ban.hpp>
#include <info/info.hpp>
#include <mod.hpp>
#include <algorithm>


using namespace MCPP;


static const String name("Bans Information");
static const String identifier("bans");
static const String help("Displays the banlist.");
static const Word priority=1;
static const String bans_banner("BANLIST:");
static const String parenthetical []={
	" (",
	")"
};
static const String by("by {0}");
static const String reason("with reason \"{0}\"");
static const String separator(" ");


class BanInfoProvider : public Module, public InformationProvider {


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
		
		
		virtual void Execute (ChatMessage & message) const {
		
			//	Get information
			auto info=Bans::Get().GetInfo();
			
			//	Sort
			std::sort(
				info.begin(),
				info.end(),
				[] (const BanInfo & a, const BanInfo & b) {	return a.Username<b.Username;	}
			);
			
			//	Output
			
			message	<<	ChatStyle::Bold
					<<	bans_banner
					<<	ChatFormat::Pop;
					
			for (auto & i : info) {
			
				message << Newline << i.Username;
				
				if (!(i.By.IsNull() && i.Reason.IsNull())) {
				
					message << parenthetical[0];
					
					if (!i.By.IsNull()) {
					
						message << String::Format(
							by,
							*i.By
						);
						
						if (!i.Reason.IsNull()) message << separator;
					
					}
					
					if (!i.Reason.IsNull()) message << String::Format(
						reason,
						*i.Reason
					);
					
					message << parenthetical[1];
				
				}
			
			}
		
		}


};


INSTALL_MODULE(BanInfoProvider)

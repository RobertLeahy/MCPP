#include <info/info.hpp>
#include <ban/ban.hpp>
#include <algorithm>


static const String name("Ban Information");
static const Word priority=1;
static const String identifier("bans");
static const String help("Lists all banned players and IPs.");
static const String bans_banner("BANS:");
static const String users_banner("Banned Users:");
static const String ips_banner("Banned IPs:");
static const String ranges_banner("Banned IP Ranges:");
static const String mask_template("{0} mask {1}");


class BanInfo : public Module, public InformationProvider {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			Information->Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			//	Get list
			auto banlist=Bans->List();
			
			//	Sort the lists
			std::sort(
				banlist.Users.begin(),
				banlist.Users.end()
			);
			auto ip_sorter=[] (IPAddress a, IPAddress b) {
			
				//	If IP addresses aren't the same type
				//	the IPv4 addresses come first
				if (a.IsV6()!=b.IsV6()) return !a.IsV6();
				
				//	If they're the same type just use
				//	numerical sorting
				if (a.IsV6()) return static_cast<UInt128>(a)<static_cast<UInt128>(b);
				
				return static_cast<UInt32>(a)<static_cast<UInt32>(b);
			
			};
			std::sort(
				banlist.IPs.begin(),
				banlist.IPs.end(),
				ip_sorter
			);
			std::sort(
				banlist.Ranges.begin(),
				banlist.Ranges.end(),
				[&] (const Tuple<IPAddress,IPAddressMask> & a, const Tuple<IPAddress,IPAddressMask> & b) {
				
					//	If the IPs aren't equal, sort based
					//	solely on the IPs themselves
					if (a.Item<0>()!=b.Item<0>()) return ip_sorter(
						a.Item<0>(),
						b.Item<0>()
					);
					
					//	Sort based on the masks with the
					//	lower (i.e. narrower) masks coming
					//	first
					if (a.Item<0>().IsV6()) return a.Item<1>().IPv6<b.Item<1>().IPv6;
					
					return a.Item<1>().IPv4<b.Item<1>().IPv6;
				
				}
			);
			
			//	Create output
			
			message	<< ChatStyle::Bold
					<< bans_banner
					<< ChatFormat::Pop;
					
			if (banlist.Users.Count()!=0) {
			
				message	<< ChatStyle::Bold
						<< Newline
						<< users_banner
						<< ChatFormat::Pop;
						
				for (const auto & s : banlist.Users) message << Newline << s;
			
			}
			
			if (banlist.IPs.Count()!=0) {
			
				message	<< ChatStyle::Bold
						<< Newline
						<< ips_banner
						<< ChatFormat::Pop;
						
				for (const auto & ip : banlist.IPs) message << Newline << String(ip);
			
			}
			
			if (banlist.Ranges.Count()!=0) {
			
				message	<< ChatStyle::Bold
						<< Newline
						<< ranges_banner
						<< ChatFormat::Pop;
						
				for (const auto & t : banlist.Ranges) {
				
					message << Newline << String::Format(
						mask_template,
						t.Item<0>(),
						t.Item<0>().IsV6() ? IPAddress(t.Item<1>().IPv6) : IPAddress(t.Item<1>().IPv4)
					);
					
				}
			
			}
		
		}


};


static Nullable<BanInfo> module;


extern "C" {

	
	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}
	

}

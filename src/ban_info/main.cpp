#include <common.hpp>
#include <ban/ban.hpp>
#include <op/op.hpp>
#include <chat/chat.hpp>
#include <utility>
#include <algorithm>


static const Word priority=2;
static const String name("Banlist Chat Information Provider");
static const Regex info_regex(
	"^\\/info\\s+(\\w+)$",
	RegexOptions().SetIgnoreCase()
);
static const String bans_arg("bans");
static const String info_banner("§e§l====INFORMATION====§r");
static const String not_an_op_error(
	"§c§l"	//	Bold and red
	"SERVER:"	//	Server error message
	"§r§c"	//	Not-bold and red
	" You must be an operator to issue that command"
);
static const String bans_banner("§lBANS:§r");
static const String users_banner("§lBanned Users:§r");
static const String ips_banner("§lBanned IPs:§r");
static const String ranges_banner("§lBanned IP Ranges:§r");
static const String mask_template("{0} mask {1}");


class BanInfo : public Module {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	Grab previous handler for
			//	chaining
			ChatHandler prev(std::move(Chat->Chat));
			
			//	Install ourselves
			Chat->Chat=[=] (SmartPointer<Client> client, const String & message) {
			
				//	Attempt to match against our
				//	regex
				auto match=info_regex.Match(message);
				
				//	If there's a match, proceed
				if (match.Success()) {
				
					//	The user has to be op to
					//	request info on the banlist
					if (!Ops->IsOp(client->GetUsername())) {
					
						Chat->Send(
							client,
							not_an_op_error
						);
						
						return;
					
					}
					
					//	See if it's the right arg
					if (match[1].Value().ToLower()==bans_arg) {
					
						//	Output info
						
						//	Get list
						auto banlist=Bans->List();
						
						//	Sort the various
						//	lists
						std::sort(banlist.Users.begin(),banlist.Users.end());
						//	Callback for sorting IP addresses
						auto ip_sorter=[] (IPAddress a, IPAddress b) {
						
							//	They're not the same type
							if (a.IsV6()!=b.IsV6()) {
							
								//	v6 addresses come last
								return !a.IsV6();
								
							
							}
							
							//	They're the same type
							if (a.IsV6()) {
							
								//	Use numerical sorting
								return static_cast<UInt128>(a)<static_cast<UInt128>(b);
							
							}
							
							//	Use numerical sorting
							return static_cast<UInt32>(a)<static_cast<UInt32>(b);
						
						};
						std::sort(banlist.IPs.begin(),banlist.IPs.end(),ip_sorter);
						std::sort(banlist.Ranges.begin(),banlist.Ranges.end(),[&] (const Tuple<IPAddress,IPAddressMask> & a, const Tuple<IPAddress,IPAddressMask> & b) {
						
							//	If the IPs aren't equal, sort
							//	based on the IPs
							if (a.Item<0>()!=b.Item<0>()) return ip_sorter(
								a.Item<0>(),
								b.Item<0>()
							);
							
							//	Sort based on the masks
							//	with the lower masks
							//	(i.e. narrower) coming
							//	first
							if (a.Item<0>().IsV6()) {
							
								return a.Item<1>().IPv6<b.Item<1>().IPv6;
							
							}
							
							return a.Item<1>().IPv4<b.Item<1>().IPv6;
						
						});
						
						//	Create output
						String output(info_banner);
						output << Newline << bans_banner;
						
						if (banlist.Users.Count()!=0) {
						
							output << Newline << users_banner;
							
							for (auto & s : banlist.Users) output << Newline << s;
						
						}
						
						if (banlist.IPs.Count()!=0) {
						
							output << Newline << ips_banner;
							
							for (auto & ip : banlist.IPs) output << Newline << String(ip);
						
						}
						
						if (banlist.Ranges.Count()!=0) {
						
							output << Newline << ranges_banner;
							
							for (auto & t : banlist.Ranges) {
							
								output << Newline << String::Format(
									mask_template,
									t.Item<0>(),
									t.Item<0>().IsV6() ? IPAddress(t.Item<1>().IPv6) : IPAddress(t.Item<1>().IPv4)
								);
							
							}
						
						}
						
						//	Send
						Chat->Send(
							client,
							ChatModule::Sanitize(
								output,
								false
							)
						);
						
						//	Stop processing this chat message
						return;
					
					}
				
				}

				//	Chain if applicable
				if (prev) {
				
					prev(
						std::move(client),
						message
					);
				
				}
			
			};
		
		}


};


Nullable<BanInfo> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

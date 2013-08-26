#include <ban/ban.hpp>
#include <utility>
#include <stdexcept>
#include <limits>


namespace MCPP {


	static const String name("Ban Support");
	static const Word priority=1;
	static const String user_key("ban");
	static const String ips_key("ban_ip");
	static const String ip_format_log("Banned IP \"{0}\" has an invalid format");
	static const String ip_ranges_key("ban_ip_range");
	static const Regex range_regex(
		"^(.+)\\/(\\d+)$",
		RegexOptions().SetRightToLeft().SetSingleline()
	);
	static const String overflow_log("Number of bits \"{0}\" in banned IP range specification \"{1}\" is too large");
	static const String ip_range_format_log("IP \"{0}\" in banned IP range specification \"{1}\" has an invalid format");
	static const String too_many_bits("Number of bits \"{0}\" in banned IP range specification \"{1}\" exceeds number of bits in IP");
	static const String banned_everyone("Number of bits in banned IP range specification \"{0}\" is zero, meaning everyone is banned");
	static const String banned_template("{0} is banned, disconnecting");
	static const String ban_msg("You are banned");
	static const String ban_reason("Banned");
	static const char * too_many_bits_except("Mask specifies more bits than in address");
	
	
	Word BanModule::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & BanModule::Name () const noexcept {
	
		return name;
	
	}
	
	
	//	Creates an IPAddressMask
	//	structure which corresponds
	//	to the given IP version and
	//	number of bits
	static inline IPAddressMask make_mask (Word bits, bool is_v6) noexcept {
	
		IPAddressMask mask;
		
		if (is_v6) {
		
			mask.IPv6=0;
			
			for (Word i=0;i<bits;++i) mask.IPv6=(mask.IPv6>>1)|(~static_cast<UInt128>(std::numeric_limits<Int128>::max()));
		
		} else {
		
			mask.IPv4=0;
			
			for (Word i=0;i<bits;++i) mask.IPv4=(mask.IPv4>>1)|(~static_cast<UInt32>(std::numeric_limits<Int32>::max()));
		
		}
		
		return mask;
	
	}
	
	
	void BanModule::Install () {
	
		//	We need to grab the lists
		//	of bans from the backing
		//	store
		
		//	Banned users
		auto users=RunningServer->Data().GetValues(user_key);
		
		//	Loop and add to the set
		for (auto & s : users) {
		
			s.ToLower();
			
			banned_users.insert(std::move(s));
		
		}
		
		//	Banned IPs
		auto ips=RunningServer->Data().GetValues(ips_key);
		
		//	Loop
		for (auto & ip : ips) {
			
			//	Attempt to parse the IP
			//	from the string
			IPAddress parsed;
			try {
			
				parsed=IPAddress(ip);
			
			//	Log if that fails
			} catch (...) {
			
				RunningServer->WriteLog(
					String::Format(
						ip_format_log,
						ip
					),
					Service::LogType::Warning
				);
			
			}
			
			banned_ips.insert(std::move(parsed));
		
		}
		
		//	Banned IP ranges
		auto ip_ranges=RunningServer->Data().GetValues(ip_ranges_key);
		
		//	Loop
		for (auto & range : ip_ranges) {
			
			//	Attempt to match to extract
			//	components
			
			auto match=range_regex.Match(range);
			
			if (match.Success()) {
			
				//	Get an integer from
				//	the number of bits
				Word mask_bits;
				try {
				
					match[2].Value().ToInteger(&mask_bits);
				
				} catch (...) {
				
					RunningServer->WriteLog(
						String::Format(
							overflow_log,
							match[2].Value(),
							range
						),
						Service::LogType::Warning
					);
					
					continue;
				
				}
				
				//	Now attempt to get an IP
				IPAddress parsed;
				try {
				
					parsed=IPAddress(match[1].Value());
				
				} catch (...) {
				
					RunningServer->WriteLog(
						String::Format(
							ip_range_format_log,
							match[1].Value(),
							range
						),
						Service::LogType::Warning
					);
					
					continue;
				
				}
				
				//	Is the bit mask at all sane?
				Word bits_in_ip=parsed.IsV6() ? 128 : 32;
				
				if (mask_bits>bits_in_ip) {
				
					RunningServer->WriteLog(
						String::Format(
							too_many_bits,
							mask_bits,
							range
						),
						Service::LogType::Warning
					);
					
					continue;
				
				}
				
				//	Single IP ban
				if (mask_bits==bits_in_ip) {
				
					banned_ips.insert(std::move(parsed));
					
					continue;
				
				}
				
				//	Everyone is banned
				if (mask_bits==0) {
				
					RunningServer->WriteLog(
						String::Format(
							banned_everyone,
							range
						),
						Service::LogType::Warning
					);
					
					//	We'll actually go ahead
					//	and do this.
				
				}
				
				//	Create mask structure
				IPAddressMask mask(make_mask(mask_bits,parsed.IsV6()));
				
				banned_ip_ranges.EmplaceBack(
					std::move(parsed),
					std::move(mask)
				);
			
			}
		
		}
		
		//	Hook up our handlers
		
		//	To implement IP banning
		//	we need to hook into the
		//	onaccept event
		RunningServer->OnAccept.Add([=] (IPAddress ip, UInt16) {
		
			if (IsBanned(ip)) {
			
				RunningServer->WriteLog(
					String::Format(
						banned_template,
						ip
					),
					Service::LogType::Information
				);
			
				return false;
			
			}
			
			return true;
		
		});
		
		//	The user sends their username with
		//	0x02, so we can kill their connection
		//	that early in the connection process
		PacketHandler prev(std::move(RunningServer->Router[0x02]));
		
		RunningServer->Router[0x02]=[=] (SmartPointer<Client> client, Packet packet) {
		
			typedef PacketTypeMap<0x02> pt;
			
			//	Determine if user is banned based
			//	on username they sent.
			if (IsBanned(packet.Retrieve<pt,1>())) {
			
				//	User is banned
				
				//	Kill them
				client->Disconnect(ban_reason);
			
				//	Log
				RunningServer->WriteLog(
					String::Format(
						banned_template,
						packet.Retrieve<pt,1>()
					),
					Service::LogType::Information
				);
			
			//	Only chain if we're not disconnecting
			//	the user because they're banned
			} else if (prev) {
			
				prev(
					std::move(client),
					std::move(packet)
				);
			
			}
		
		};
	
	}
	
	
	bool BanModule::IsBanned (String username) const {
	
		username.ToLower();
	
		return users_lock.Read([&] () {	return banned_users.find(username)!=banned_users.end();	});
	
	}
	
	
	//	Tests to see if an IP address matches another
	//	IP address given a mask
	static inline bool ip_matches (IPAddress ip, const Tuple<IPAddress,IPAddressMask> & t) noexcept {
	
		//	If IPs aren't the same version
		//	we can't compare them at all
		if (ip.IsV6()!=t.Item<0>().IsV6()) return false;
		
		//	Branch based on version
		if (ip.IsV6()) {
		
			if (
				(static_cast<UInt128>(t.Item<0>())&t.Item<1>().IPv6)==
				(static_cast<UInt128>(ip)&t.Item<1>().IPv6)
			) return true;
		
		} else {
		
			if (
				(static_cast<UInt32>(t.Item<0>())&t.Item<1>().IPv6)==
				(static_cast<UInt32>(ip)&t.Item<1>().IPv4)
			) return true;
		
		}
		
		return false;
	
	}
	
	
	bool BanModule::IsBanned (IPAddress ip) const noexcept {
	
		return ips_lock.Read([&] () {
		
			//	Check the hash table first
			if (banned_ips.find(ip)!=banned_ips.end()) return true;
			
			//	Scan IP ranges
			for (auto & t : banned_ip_ranges) if (ip_matches(ip,t)) return true;
			
			//	IP isn't banned
			return false;
		
		});
	
	}
	
	
	void BanModule::Ban (String username) {
	
		username.ToLower();
	
		//	Add to the banlist
		if (users_lock.Write([&] () {
		
			//	Add to banlist cache

			//	Only proceed if the user wasn't
			//	already banned
			if (!banned_users.insert(username).second) return false;
			
			//	Ban user in the database
			RunningServer->Data().InsertValue(user_key,username);
			
			return true;
		
		})) {
		
			//	We had to add the user
			//	to the banlist, which means
			//	they weren't banned before,
			//	which means we need to scan
			//	connected users and boot
			//	them if they're on.
			RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
			
				if (client->GetUsername().ToLower()==username) {
				
					client->Disconnect(ban_reason);
				
				}
			
			});
		
		}
	
	}
	
	
	void BanModule::Unban (String username) {
	
		username.ToLower();
		
		//	Attempt to remove user from the
		//	banlist
		users_lock.Write([&] () {
		
			//	Only go to the database
			//	to remove a banned user
			//	if they were banned in
			//	the first place
			if (banned_users.erase(username)!=0) {
			
				RunningServer->Data().DeleteValues(user_key,username);
			
			}
		
		});
	
	}
	
	
	void BanModule::Ban (IPAddress ip) {
	
		//	Add to the banlist
		if (ips_lock.Write([&] () {
		
			//	Only proceed if the user wasn't
			//	already banned
			if (!banned_ips.insert(ip).second) return false;
			
			//	Ban user in the database
			RunningServer->Data().InsertValue(ips_key,ip);
			
			return true;
		
		})) {
		
			//	We had to add the IP to the banlist,
			//	which means a user with that IP might
			//	be on-line.
			//
			//	Scan all connected users and disconnect
			//	all who match.
			RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
			
				if (client->IP()==ip) client->Disconnect(ban_reason);
			
			});
		
		}
	
	}
	
	
	void BanModule::Unban (IPAddress ip) {
	
		//	Remove IP from the banlist
		ips_lock.Write([&] () {
		
			//	Only go to the database
			//	to remove an IP that was
			//	already banned
			if (banned_ips.erase(ip)!=0) {
			
				RunningServer->Data().DeleteValues(ips_key,ip);
			
			}
		
		});
	
	}
	
	
	static inline bool compare_ranges (
		const Tuple<IPAddress,IPAddressMask> & a,
		const Tuple<IPAddress,IPAddressMask> & b
	) noexcept {
	
		if (a.Item<0>()!=b.Item<0>()) return false;
		
		return a.Item<0>().IsV6() ? (a.Item<1>().IPv6==b.Item<1>().IPv6) : (a.Item<1>().IPv4==b.Item<1>().IPv4);
	
	}
	
	
	void BanModule::Ban (IPAddress ip, Word bits) {
	
		//	Check number of bits in mask
		if (bits==ip.IsV6() ? 128 : 32) {
		
			//	This is just a regular,
			//	single IP ban
			Ban(ip);
		
		} else if (ip.IsV6() ? (bits>128) : (bits>32)) {
		
			throw std::invalid_argument(too_many_bits_except);
		
		//	Ban
		} else {
		
			//	Create a mask
			IPAddressMask mask(make_mask(bits,ip.IsV6()));
			
			//	Make a tuple
			Tuple<IPAddress,IPAddressMask> tuple(
				std::move(ip),
				std::move(mask)
			);
			
			//	Lock
			ips_lock.Write([&] () {
			
				bool found=false;
				for (auto & t : banned_ip_ranges) {
				
					if (compare_ranges(t,tuple)) {
					
						found=true;
						
						break;
					
					}
				
				}
				
				//	Add only if ban didn't exist
				//	before
				if (!found) {
				
					//	Prepare backing store value
					String str(tuple.Item<0>());
					str << "/" << bits;
					
					//	Add to banlist
					banned_ip_ranges.Add(std::move(tuple));
					
					//	Add to backing store
					RunningServer->Data().InsertValue(ip_ranges_key,str);
				
				}
			
			});
		
		}
	
	}
	
	
	void BanModule::Unban (IPAddress ip, Word bits) {
	
		//	Ignore malformed masks
		if (ip.IsV6() ? (bits>128) : (bits>32)) return;
		
		//	Create a mask
		IPAddressMask mask(make_mask(bits,ip.IsV6()));
		
		//	Make a tuple
		Tuple<IPAddress,IPAddressMask> tuple(
			ip,
			std::move(mask)
		);
		
		//	Lock
		ips_lock.Write([&] () {
		
			//	Scan and unban
			for (Word i=0;i<banned_ip_ranges.Count();++i) {
			
				if (compare_ranges(banned_ip_ranges[i],tuple)) {
				
					//	Delete from list
					banned_ip_ranges.Delete(i);
					
					//	Prepare a string
					String str(ip);
					str << "/" << bits;
					
					//	Delete from the backing store
					RunningServer->Data().DeleteValues(ip_ranges_key,str);
					
					//	We're done
					break;
				
				}
			
			}
			
			//	If the mask is exactly the number
			//	of bits in the address, this
			//	could be a single IP too, so
			//	try and remove that as well
			if (bits==ip.IsV6() ? 128 : 32) {
			
				//	Check to see if the single IP
				//	is in the list
				if (banned_ips.erase(ip)!=0) {
				
					//	Go to the backing store and
					//	remove it there as well
					RunningServer->Data().DeleteValues(ips_key,ip);
				
				}
			
			}
		
		});
	
	}
	
	
	BanList BanModule::List () const {
	
		BanList returnthis;
		
		users_lock.Read([&] () {	for (const auto & s : banned_users) returnthis.Users.Add(s);	});
		
		ips_lock.Read([&] () {
		
			for (const auto & ip : banned_ips) returnthis.IPs.Add(ip);
			
			for (const auto & t : banned_ip_ranges) returnthis.Ranges.Add(t);
		
		});
		
		return returnthis;
	
	}
	
	
	Nullable<BanModule> Bans;


}


extern "C" {


	Module * Load () {
	
		if (Bans.IsNull()) try {
		
			Bans.Construct();
			
			return &(*Bans);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		Bans.Destroy();
	
	}


}

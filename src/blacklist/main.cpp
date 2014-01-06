#include <blacklist/blacklist.hpp>
#include <save/save.hpp>
#include <serializer.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <utility>


using namespace MCPP;


namespace MCPP {


	static const Word priority=1;
	static const String name("Blacklist");
	static const String blacklisted("Blacklisted");
	static const String save_key("blacklist");
	static const String verbose_key("blacklist");
	static const String blacklist("Blacklisting {0}");
	static const String unblacklist("Removing {0} from the blacklist");
	static const String error_parsing("Error parsing blacklist: \"{0}\" at byte {1}");
	
	
	bool Blacklist::is_verbose () {
	
		return Server::Get().IsVerbose(verbose_key);
	
	}
	
	
	void Blacklist::save () const {
	
		ByteBuffer buffer;
		
		lock.Read([&] () {	buffer.ToBytes(ranges);	});
		
		buffer.Save(save_key);
	
	}
	
	
	void Blacklist::load () {
	
		auto buffer=ByteBuffer::Load(save_key);
		
		//	If nothing was loaded from the backing
		//	store, don't even try and parse
		if (buffer.Count()==0) return;
		
		decltype(this->ranges) ranges;
		try {
		
			ranges=buffer.FromBytes<decltype(ranges)>();
		
		} catch (const ByteBufferError & e) {
		
			//	Log problem parsing
			Server::Get().WriteLog(
				String::Format(
					error_parsing,
					e.what(),
					e.Where()
				),
				Service::LogType::Error
			);
		
			return;
		
		}
		
		//	Load results
		this->ranges=std::move(ranges);
	
	}
	
	
	static Singleton<Blacklist> singleton;
	
	
	Blacklist & Blacklist::Get () noexcept {
	
		return singleton.Get();
	
	}
	
	
	Word Blacklist::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & Blacklist::Name () const noexcept {
	
		return name;
	
	}
	
	
	void Blacklist::Install () {
	
		auto & server=Server::Get();
		
		//	Hook into the accept event -- we intercept
		//	incoming connections and quash them if they're
		//	from blacklisted IPs
		server.OnAccept.Add([this] (IPAddress ip, UInt16, IPAddress, UInt16) {	return !Check(ip);	});
		
		//	Load blacklists from backing store
		load();
		
		//	Hook into save loop
		SaveManager::Get().Add([this] () {	save();	});
	
	}
	
	
	bool Blacklist::Check (IPAddress ip) const noexcept {
	
		return lock.Read([&] () {
		
			for (auto & range : ranges) if (range.Check(ip)) return true;
			
			return false;
		
		});
	
	}
	
	
	void Blacklist::Add (IPAddressRange range) {
	
		//	Attempt to insert IP into set of
		//	blacklisted IP ranges
		if (lock.Write([&] () mutable {	return ranges.insert(range).second;	})) {
		
			for (auto & client : Server::Get().Clients) {
			
				if (range.Check(client->IP())) client->Disconnect(blacklisted);
			
			}
			
			if (is_verbose()) Server::Get().WriteLog(
				String::Format(
					blacklist,
					range
				),
				Service::LogType::Debug
			);
		
		}
	
	}
	
	
	void Blacklist::Remove (IPAddressRange range) {
	
		if (
			lock.Write([&] () mutable {	return ranges.erase(range)!=0;	}) &&
			is_verbose()
		) Server::Get().WriteLog(
			String::Format(
				unblacklist,
				range
			),
			Service::LogType::Debug
		);
	
	}
	
	
	BlacklistInfo Blacklist::GetInfo () const {
	
		BlacklistInfo retr;
		
		lock.Read([&] () mutable {
			
			retr.Ranges=Vector<IPAddressRange>(ranges.size());
			for (const auto & range : ranges) retr.Ranges.Add(range);
		
		});
		
		return retr;
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(Blacklist::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

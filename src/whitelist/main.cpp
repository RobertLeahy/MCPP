#include <save/save.hpp>
#include <whitelist/whitelist.hpp>
#include <serializer.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <utility>


using namespace MCPP;


namespace MCPP {


	static const String name("Whitelist");
	static const Word priority=1;
	static const String settings_key("whitelist");
	static const String verbose_key("whitelist");
	static const String reason("Not whitelisted");
	static const String enabled("Whitelist enabled");
	static const String disabled("Whitelist disabled");
	static const String whitelisted("{0} whitelisted");
	static const String unwhitelisted("{0} removed from whitelist");
	static const String error_parsing("Error parsing whitelist: \"{0}\" at byte {1}");
	static const String true_string("true");
	static const String false_string("false");
	static const bool enabled_default=false;
	
	
	static bool is_verbose () {
	
		return Server::Get().IsVerbose(verbose_key);
	
	}
	
	
	static void normalize (String & str) {
	
		str.ToLower();
	
	}
	
	
	const String & Whitelist::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Whitelist::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void Whitelist::Install () {
	
		auto & server=Server::Get();
		
		//	Hook into the OnLogin handler
		server.OnLogin.Add([this] (SmartPointer<Client> client) {
		
			if (enabled && !Check(client->GetUsername())) client->Disconnect(reason);
			
		});
		
		//	Determine whether or not we're
		//	enabled or disabled
		enabled=server.Data().GetSetting(
			settings_key,
			enabled_default
		);
		
		//	Load the whitelist
		auto buffer=ByteBuffer::Load(settings_key);
		
		//	If nothing was loaded from the backing store,
		//	don't even try and parse
		if (buffer.Count()!=0) try {
		
			whitelist=buffer.FromBytes<decltype(whitelist)>();
		
		} catch (const ByteBufferError & e) {
		
			server.WriteLog(
				String::Format(
					error_parsing,
					e.what(),
					e.Where()
				),
				Service::LogType::Error
			);
		
		}
		
		//	Hook into the save loop
		SaveManager::Get().Add([this] () mutable {
		
			//	Save the whitelist
			
			ByteBuffer buffer;
			lock.Execute([&] () mutable {	buffer.ToBytes(whitelist);	});
			
			buffer.Save(settings_key);
			
			//	Save the enabled/disabled state
			Server::Get().Data().SetSetting(
				settings_key,
				enabled ? true_string : false_string
			);
		
		});
	
	}
	
	
	bool Whitelist::Enable () noexcept {
	
		//	Attempt to enable
		bool expected=false;
		if (enabled.compare_exchange_strong(expected,true)) {
		
			try {
			
				if (is_verbose()) Server::Get().WriteLog(
					MCPP::enabled,
					Service::LogType::Debug
				);
			
			//	Ignore errors
			} catch (...) {	}
			
			return true;
		
		}
		
		return false;
	
	}
	
	
	bool Whitelist::Disable () noexcept {
	
		//	Attempt to disable
		bool expected=true;
		if (enabled.compare_exchange_strong(expected,false)) {
		
			try {
			
				if (is_verbose()) Server::Get().WriteLog(
					disabled,
					Service::LogType::Debug
				);
			
			//	Ignore errors
			} catch (...) {	}
			
			return true;
			
		}
		
		return false;
	
	}
	
	
	bool Whitelist::Enabled () const noexcept {
	
		return enabled;
	
	}
	
	
	bool Whitelist::Add (String username) {
	
		//	Normalize username
		normalize(username);
		
		return lock.Execute([&] () mutable {
		
			auto pair=whitelist.emplace(std::move(username));
			
			if (pair.second) {
			
				if (is_verbose()) Server::Get().WriteLog(
					String::Format(
						whitelisted,
						*pair.first
					),
					Service::LogType::Debug
				);
			
				return true;
			
			}
			
			return false;
		
		});
	
	}
	
	
	bool Whitelist::Remove (String username) {
	
		normalize(username);
		
		if (lock.Execute([&] () mutable {	return whitelist.erase(username)!=0;	})) {
		
			if (is_verbose()) Server::Get().WriteLog(
				String::Format(
					unwhitelisted,
					username
				),
				Service::LogType::Debug
			);
			
			//	Scan all connected clients and disconnect
			//	any who have been unwhitelisted as a result
			//	of this
			for (auto & client : Server::Get().Clients) if (client->GetState()==ProtocolState::Play) {
			
				auto normalized=client->GetUsername();
				normalize(normalized);
				
				if (normalized==username) client->Disconnect(reason);
			
			}
		
			return true;
		
		}
		
		return false;
	
	}
	
	
	bool Whitelist::Check (String username) const {
	
		normalize(username);
		
		return lock.Execute([&] () mutable {	return whitelist.count(username)!=0;	});
	
	}
	
	
	Vector<String> Whitelist::GetInfo () const {
	
		return lock.Execute([&] () mutable {
		
			Vector<String> retr(whitelist.size());
			
			for (const auto & username : whitelist) retr.Add(username);
			
			return retr;
		
		});
	
	}
	
	
	static Singleton<Whitelist> singleton;
	
	
	Whitelist & Whitelist::Get () noexcept {
	
		return singleton.Get();
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(singleton.Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

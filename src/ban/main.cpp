#include <ban/ban.hpp>
#include <save/save.hpp>
#include <serializer.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <utility>


using namespace MCPP;


namespace MCPP {


	template <>
	class Serializer<BanInfo> {
	
	
		public:
		
		
			static BanInfo FromBytes (const Byte * & begin, const Byte * end) {
			
				BanInfo retr;
				retr.Username=Serializer<String>::FromBytes(begin,end);
				retr.By=Serializer<Nullable<String>>::FromBytes(begin,end);
				retr.Reason=Serializer<Nullable<String>>::FromBytes(begin,end);
				
				return retr;
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const BanInfo & obj) {
			
				Serializer<String>::ToBytes(buffer,obj.Username);
				Serializer<Nullable<String>>::ToBytes(buffer,obj.By);
				Serializer<Nullable<String>>::ToBytes(buffer,obj.Reason);
			
			}
	
	
	};
	
	
	template <>
	class Serializer<std::pair<const String,BanInfo>> {
	
	
		private:
		
		
			typedef std::pair<const String,BanInfo> type;
	
	
		public:
		
		
			static type FromBytes (const Byte * & begin, const Byte * end) {
			
				auto info=Serializer<BanInfo>::FromBytes(begin,end);
				String username=info.Username;
				
				return type{
					std::move(username),
					std::move(info)
				};
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const type & obj) {
			
				Serializer<BanInfo>::ToBytes(buffer,obj.second);
			
			}
	
	
	};


	static const String name("Ban Support");
	static const Word priority=1;
	static const String disconnect_reason("Banned");
	static const String save_key("bans");
	static const String verbose_key("ban");
	static const String error_parsing("Error parsing banlist: \"{0}\" at byte {1}");
	static const String banned("\"{0}\" was banned");
	static const String unbanned("\"{0}\" was unbanned");
	static const String by(" by {0}");
	static const String reason(" with reason \"{0}\"");
	
	
	static bool is_verbose () {
	
		return Server::Get().IsVerbose(verbose_key);
	
	}
	
	
	static void normalize (String & str) {
	
		str.ToLower();
	
	}
	
	
	const String & Bans::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Bans::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void Bans::Install () {
	
		auto & server=Server::Get();
	
		//	Hook into the login event to enforce
		//	bans
		server.OnLogin.Add([this] (SmartPointer<Client> client) {
		
			if (Check(client->GetUsername())) client->Disconnect(disconnect_reason);
		
		});
		
		//	Load configuration
		auto buffer=ByteBuffer::Load(save_key);
		if (buffer.Count()!=0) try {
		
			bans=buffer.FromBytes<decltype(bans)>();
		
		} catch (const ByteBufferError & e) {
		
			//	Log problem
			server.WriteLog(
				String::Format(
					error_parsing,
					e.what(),
					e.Where()
				),
				Service::LogType::Error
			);
		
		}
		
		//	Hook into save system
		SaveManager::Get().Add([this] () {
		
			ByteBuffer buffer;
			
			lock.Read([&] () {	buffer.ToBytes(bans);	});
			
			buffer.Save(save_key);
		
		});
	
	}
	
	
	bool Bans::Ban (BanInfo ban) {
	
		//	Normalize the username
		normalize(ban.Username);
	
		return lock.Write([&] () mutable {
		
			//	Attempt to ban the user
			String cpy(ban.Username);
			auto pair=bans.emplace(
				std::move(cpy),
				std::move(ban)
			);
			
			auto & server=Server::Get();
			auto & info=pair.first->second;
			
			//	Disconnect all connected users
			//	with the username-in-question
			for (auto & client : server.Clients) if (client->GetState()==ProtocolState::Play) {
			
				auto normalized=client->GetUsername();
				normalize(normalized);
				if (normalized==info.Username) client->Disconnect(disconnect_reason);
			
			}
			
			//	Log if applicable
			if (pair.second && is_verbose()) {
				
				String log(
					String::Format(
						banned,
						info.Username
					)
				);
				if (!info.By.IsNull()) log << String::Format(
					by,
					*info.By
				);
				if (!info.Reason.IsNull()) log << String::Format(
					reason,
					*info.Reason
				);
				
				Server::Get().WriteLog(
					log,
					Service::LogType::Debug
				);
			
			}
			
			return pair.second;
		
		});
	
	}
	
	
	bool Bans::Unban (String username, Nullable<String> by) {
	
		//	Normalize the username
		normalize(username);
		
		if (lock.Write([&] () mutable {	return bans.erase(username)!=0;	})) {
		
			//	User was unbanned, log if applicable
			if (is_verbose()) {
			
				String log(
					String::Format(
						unbanned,
						username
					)
				);
				if (!by.IsNull()) log << String::Format(
					::by,
					*by
				);
				
				Server::Get().WriteLog(
					log,
					Service::LogType::Debug
				);
				
			}
			
			return true;
		
		}
		
		return false;
	
	}
	
	
	bool Bans::Check (String username) const {
	
		//	Normalize the username
		normalize(username);
	
		return lock.Read([&] () {	return bans.count(username)!=0;	});
	
	}
	
	
	Nullable<BanInfo> Bans::Retrieve (String username) const {
	
		//	Normalize the username
		normalize(username);
	
		return lock.Read([&] () {
		
			Nullable<BanInfo> retr;
			
			auto loc=bans.find(username);
			
			if (loc!=bans.end()) retr.Construct(loc->second);
			
			return retr;
		
		});
	
	}
	
	
	Vector<BanInfo> Bans::GetInfo () const {
	
		return lock.Read([&] () {
		
			Vector<BanInfo> retr;
			
			for (auto & pair : bans) retr.Add(pair.second);
			
			return retr;
		
		});
	
	}
	
	
	static Singleton<Bans> singleton;
	
	
	Bans & Bans::Get () noexcept {
	
		return singleton.Get();
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(Bans::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

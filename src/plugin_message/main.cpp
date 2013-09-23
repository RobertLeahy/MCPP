#include <plugin_message/plugin_message.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <utility>


using namespace MCPP;


namespace MCPP {


	static const String name("Plugin Message Support");
	static const Word priority=1;
	static const String reg("REGISTER");
	static const String unreg("UNREGISTER");
	static const Regex is_builtin("^MC\\|");
	
	
	Word PluginMessages::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & PluginMessages::Name () const noexcept {
	
		return name;
	
	}
	
	
	static inline Vector<String> split (String str) {
	
		Vector<String> retr;
		
		String in_progress;
		for (const auto & gc : str) {
		
			if (
				(gc=='\0') &&
				(in_progress.Size()!=0)
			) retr.Add(std::move(in_progress));
			else in_progress << gc;
		
		}
		
		if (in_progress.Size()!=0) retr.Add(std::move(in_progress));
		
		return retr;
	
	}
	
	
	inline void PluginMessages::reg_channels (SmartPointer<Client> & client, const String & channels) {
	
		auto strs=split(channels);
		
		if (strs.Count()!=0) lock.Write([&] () mutable {
		
			auto iter=clients.find(client);
			if (iter!=clients.end()) for (auto & str : strs) iter->second.insert(std::move(str));
		
		});
	
	}
	
	
	inline void PluginMessages::unreg_channels (SmartPointer<Client> & client, const String & channels) {
	
		auto strs=split(channels);
		
		if (strs.Count()!=0) lock.Write([&] () mutable {
		
			auto iter=clients.find(client);
			if (iter!=clients.end()) for (const auto & str : strs) iter->second.erase(str);
		
		});
	
	}
	
	
	static inline String decode (const Vector<Byte> & buffer) {
	
		return UTF16().Decode(
			buffer.begin(),
			buffer.end()
		);
	
	}
	
	
	inline bool PluginMessages::handle (SmartPointer<Client> & client, String & channel, Vector<Byte> & buffer) {
	
		//	Is this a REGISTER directive?
		if (channel==reg) {
		
			reg_channels(
				client,
				decode(buffer)
			);
		
			return true;
		
		}
		
		//	is this an UNREGISTER directive?
		if (channel==unreg) {
		
			unreg_channels(
				client,
				decode(buffer)
			);
			
			return true;
		
		}
		
		//	Can we handle this plugin message
		//	channel?
		
		auto iter=callbacks.find(channel);
		
		//	NO
		
		if (iter==callbacks.end()) return false;
		
		//	YES
		
		iter->second(
			std::move(client),
			std::move(channel),
			std::move(buffer)
		);
		
		return true;
	
	}
	
	
	void PluginMessages::Install () {
	
		auto & server=Server::Get();
	
		//	Install ourselves as the handler
		//	of 0xFA
		
		auto prev=std::move(server.Router[0xFA]);
		
		server.Router[0xFA]=[=] (SmartPointer<Client> client, Packet packet) mutable {
			
			//	Dispatch to handler
			//
			//	Pass through if we couldn't
			//	handle it
			if (
				!(
					(client->GetState()==ClientState::Authenticated) &&
					handle(
						client,
						packet.Retrieve<
							PacketTypeMap<0xFA>,
							0
						>(),
						packet.Retrieve<Vector<Byte>>(1)
					)
				) &&
				prev
			) prev(
				std::move(client),
				std::move(packet)
			);
		
		};
		
		//	Install connect/disconnect
		//	handlers
		
		server.OnConnect.Add([this] (SmartPointer<Client> client) mutable {
		
			lock.Write([&] () mutable {
			
				clients.emplace(
					std::move(client),
					std::unordered_set<String>()
				);
			
			});
		
		});
		
		server.OnDisconnect.Add([this] (SmartPointer<Client> client, const String &) mutable {
		
			lock.Write([&] () mutable {	clients.erase(client);	});
		
		});
	
	}


	void PluginMessages::Add (String channel, std::function<void (SmartPointer<Client>, String, Vector<Byte>)> callback) {
	
		auto iter=callbacks.find(channel);
		if (iter==callbacks.end()) callbacks.emplace(
			std::move(channel),
			std::move(callback)
		);
		else iter->second=std::move(callback);
	
	}
	
	
	SmartPointer<SendHandle> PluginMessages::Send (SmartPointer<Client> client, String channel, Vector<Byte> buffer) {
	
		bool builtin=is_builtin.IsMatch(channel);
	
		//	Prepare a packet
	
		typedef PacketTypeMap<0xFA> pt;
		
		Packet packet;
		packet.SetType<pt>();
		packet.Retrieve<pt,0>()=std::move(channel);
		packet.Retrieve<pt,1>()=std::move(buffer);
		
		//	If this is a built-in channel,
		//	send at once
		if (builtin) return client->Send(packet);
	
		//	Otherwise only send if the client
		//	has registered for this channel
		return lock.Read([&] () {
		
			auto iter=clients.find(client);
			
			if (
				(iter==clients.end()) ||
				(iter->second.count(channel)==0)
			) return SmartPointer<SendHandle>();
			
			return client->Send(packet);
		
		});
	
	}


	static Singleton<PluginMessages> singleton;
	
	
	PluginMessages & PluginMessages::Get () noexcept {
	
		return singleton.Get();
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(PluginMessages::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

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
	
	
	static Vector<String> get_strings (const Vector<Byte> & buffer) {
	
		//	Decode
		auto str=UTF8().Decode(buffer.begin(),buffer.end());
		
		//	Separate on NULL character
		Vector<String> retr;
		for (auto cp : str.CodePoints()) {
		
			if (retr.Count()==0) retr.EmplaceBack();
			
			if (cp=='\0') {
			
				retr.EmplaceBack();
				
				continue;
			
			}
			
			retr[retr.Count()-1] << cp;
		
		}
		
		return retr;
	
	}
	
	
	void PluginMessages::reg (SmartPointer<Client> client, Vector<Byte> data) {
		
		auto channels=get_strings(data);
		
		lock.Write([&] () mutable {
		
			auto iter=clients.find(client);
			
			if (iter==clients.end()) return;
			
			for (auto & channel : channels) iter->second.insert(std::move(channel));
		
		});
	
	}
	
	
	void PluginMessages::unreg (SmartPointer<Client> client, Vector<Byte> data) {
	
		auto channels=get_strings(data);
		
		lock.Write([&] () mutable {
		
			auto iter=clients.find(client);
			
			if (iter==clients.end()) return;
			
			for (auto & channel : channels) iter->second.erase(channel);
		
		});
	
	}
	
	
	void PluginMessages::handler (PacketEvent event) {
	
		auto & packet=event.Data.Get<incoming>();
		
		//	Handle register/unregister separately
		if (packet.Channel==MCPP::reg) {
		
			reg(
				std::move(event.From),
				std::move(packet.Data)
			);
			
			return;
		
		} else if (packet.Channel==MCPP::unreg) {
		
			unreg(
				std::move(event.From),
				std::move(packet.Data)
			);
		
			return;
		
		}
	
		//	Try and get associated handler
		auto iter=callbacks.find(packet.Channel);
		
		//	If there's no associated handler,
		//	just pass
		if (iter==callbacks.end()) return;
		
		//	Invoke handler
		iter->second(PluginMessage{
			std::move(event.From),
			std::move(packet.Channel),
			std::move(packet.Data)
		});
	
	}
	
	
	Word PluginMessages::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & PluginMessages::Name () const noexcept {
	
		return name;
	
	}
	
	
	void PluginMessages::Install () {
	
		auto & server=Server::Get();
	
		server.Router(
			incoming::PacketID,
			incoming::State
		)=[this] (PacketEvent event) mutable {	handler(std::move(event));	};
		
		server.OnConnect.Add([this] (SmartPointer<Client> client) mutable {
		
			lock.Write([&] () mutable {
			
				clients.emplace(
					std::move(client),
					std::unordered_set<String>()
				);
			
			});
		
		});
		
		server.OnDisconnect.Add([this] (SmartPointer<Client> client, const String &) mutable {
		
			lock.Write([&] () mutable {
			
				clients.erase(client);
			
			});
		
		});
		
		server.OnShutdown.Add([this] () mutable {	callbacks.clear();	});
	
	}
	
	
	void PluginMessages::Add (String channel, std::function<void (PluginMessage)> callback) {
	
		auto iter=callbacks.find(channel);
		
		//	Replace handler if one already exists
		if (iter==callbacks.end()) {
		
			callbacks.emplace(
				std::move(channel),
				std::move(callback)
			);
		
		} else {
		
			iter->second=std::move(callback);
		
		}
	
	}
	
	
	SmartPointer<SendHandle> PluginMessages::Send (PluginMessage message) {
	
		SmartPointer<SendHandle> retr;
		
		if (message.Endpoint.IsNull()) return retr;
		
		if (
			is_builtin.IsMatch(message.Channel) ||
			lock.Read([&] () mutable {
			
				auto iter=clients.find(message.Endpoint);
				
				return !((iter==clients.end()) || (iter->second.count(message.Channel)==0));
			
			})
		) {
		
			outgoing packet;
			packet.Channel=std::move(message.Channel);
			packet.Data=std::move(message.Buffer);
			
			retr=message.Endpoint->Send(packet);
		
		}
		
		return retr;
	
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

#include <brand/brand.hpp>
#include <rleahylib/rleahylib.hpp>
#include <plugin_message/plugin_message.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <utility>


using namespace MCPP;


namespace MCPP {


	static const String name("MC|Brand Handling");
	static const Word priority=1;
	static const String channel("MC|Brand");
	static const String brand("Minecraft++");	//	TODO: Add version information
	static const String verbose_key("brand");
	static const String identifying_template("Identifying as \"{0}\" to {1}:{2}");
	static const String identified_template("{0}:{1} identified as \"{2}\"");


	const String & Brands::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Brands::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void Brands::Install () {
	
		auto & server=Server::Get();
		
		//	Encode brand string
		encoded=UTF8().Encode(brand);
	
		//	Install into server
		
		server.OnConnect.Add([this] (SmartPointer<Client> client) mutable {
		
			lock.Write([&] () {	brands.emplace(std::move(client),Nullable<String>());	});
		
		});
		
		server.OnDisconnect.Add([this] (SmartPointer<Client> client, const String &) mutable {
		
			lock.Write([&] () {	brands.erase(client);	});
		
		});
		
		server.OnLogin.Add([this] (SmartPointer<Client> client) {
		
			//	Debug
			auto & server=Server::Get();
			if (server.IsVerbose(verbose_key)) server.WriteLog(
				String::Format(
					identifying_template,
					brand,
					client->IP(),
					client->Port()
				),
				Service::LogType::Debug
			);
		
			//	Send brand
			PluginMessages::Get().Send(
				std::move(client),
				channel,
				encoded
			);
		
		});
		
		//	Install as handler for
		//	MC|Brand
		PluginMessages::Get().Add(
			channel,
			[this] (SmartPointer<Client> client, String, Vector<Byte> buffer) mutable {
			
				String client_brand=UTF8().Decode(
					buffer.begin(),
					buffer.end()
				);
				
				auto & server=Server::Get();
				if (server.IsVerbose(verbose_key)) server.WriteLog(
					String::Format(
						identified_template,
						client->IP(),
						client->Port(),
						client_brand
					),
					Service::LogType::Debug
				);
			
				lock.Write([&] () mutable {
				
					auto iter=brands.find(client);
					if (iter!=brands.end()) iter->second.Construct(
						std::move(client_brand)
					);
				
				});
			
			}
		);
	
	}
	
	
	Nullable<String> Brands::Get (const SmartPointer<Client> & client) {
	
		return lock.Read([&] () {
		
			Nullable<String> retr;
			
			auto iter=brands.find(client);
			
			if (!(
				(iter==brands.end()) ||
				(iter->second.IsNull())
			)) retr.Construct(*(iter->second));
			
			return retr;
		
		});
	
	}
	
	
	static Singleton<Brands> singleton;
	
	
	Brands & Brands::Get () noexcept {
	
		return singleton.Get();
	
	}
	
	
}


extern "C" {


	Module * Load () {
	
		return &(Brands::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

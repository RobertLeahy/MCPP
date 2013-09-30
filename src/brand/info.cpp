#include <brand/brand.hpp>
#include <info/info.hpp>
#include <rleahylib/rleahylib.hpp>
#include <server.hpp>
#include <client.hpp>


using namespace MCPP;


static const String name("Brand Information");
static const Word priority=1;
static const String identifier("brands");
static const String help("Displays the brand of all connected clients.");
static const String brand_banner("CLIENT BRANDS:");
static const String client_template("{0} ({1}:{2}): ");


class BrandInfo : public Module, public InformationProvider {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Install () override {
		
			Information::Get().Add(this);
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			message	<<	ChatStyle::Bold
					<<	brand_banner
					<<	ChatFormat::Pop;
					
			for (auto & client : Server::Get().Clients) {
			
				//	Ignore unauthenticated users
				if (client->GetState()==ClientState::Authenticated) {
				
					auto brand=Brands::Get().Get(client);
				
					message	<<	Newline
							<<	ChatStyle::Bold
							<<	String::Format(
									client_template,
									client->GetUsername(),
									client->IP(),
									client->Port()
								)
							<<	ChatFormat::Pop
							<<	(brand.IsNull() ? String() : *brand);
				
				}
			
			}
		
		}


};


static Nullable<BrandInfo> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

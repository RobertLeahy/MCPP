#include <info/info.hpp>
#include <data_provider.hpp>
#include <server.hpp>
#include <algorithm>


using namespace MCPP;


static const Word priority=1;
static const String name("Data Provider Information");
static const String identifier("dp");
static const String help("Displays information about the data provider the server is using.");
static const String dp_banner("DATA PROVIDER:");


class DPInfo : public Module, public InformationProvider {


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
		
			//	Get data about the data provider
			//	the server is using
			auto info=Server::Get().Data().GetInfo();
			
			//	Sort the key/value pairs
			std::sort(
				info.Data.begin(),
				info.Data.end(),
				[] (const DataProviderDatum & a, const DataProviderDatum & b) {
				
					return a.Name<b.Name;
				
				}
			);
			
			message	<< ChatStyle::Bold
					<< dp_banner
					<< Newline
					<< info.Name
					<< ":"
					<< ChatFormat::Pop;
					
			for (const auto & datum : info.Data) {
			
				message	<< Newline
						<< ChatStyle::Bold
						<< datum.Name
						<< ": "
						<< ChatFormat::Pop
						<< datum.Value;
			
			}
		
		}


};


INSTALL_MODULE(DPInfo)

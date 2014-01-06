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


class DataProviderInfo : public Module, public InformationProvider {


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
				info.Item<1>().begin(),
				info.Item<1>().end(),
				[] (const Tuple<String,String> & a, const Tuple<String,String> & b) {
				
					return a.Item<0>()<b.Item<0>();
				
				}
			);
			
			message	<< ChatStyle::Bold
					<< dp_banner
					<< Newline
					<< info.Item<0>()	//	Name of data provider
					<< ":"
					<< ChatFormat::Pop;
					
			for (const auto & t : info.Item<1>()) {
			
				message	<< Newline
						<< ChatStyle::Bold
						<< t.Item<0>()
						<< ": "
						<< ChatFormat::Pop
						<< t.Item<1>();
			
			}
		
		}


};


INSTALL_MODULE(DataProviderInfo)

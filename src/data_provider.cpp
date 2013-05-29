#include <data_provider.hpp>


namespace MCPP {


	DataProvider::DataProvider () noexcept {	}


	DataProvider::~DataProvider () noexcept {	}
	
	
	String DataProvider::GetLogType (Service::LogType type) {
	
		String type_str;
		
		switch (type) {
		
			case Service::LogType::Success:
			
				type_str="Success";
				
				break;
				
			case Service::LogType::Error:
			
				type_str="Error";
				
				break;
				
			case Service::LogType::Information:
			default:
			
				type_str="Information";
				
				break;
				
			case Service::LogType::Warning:
			
				type_str="Warning";
				
				break;
				
			case Service::LogType::Debug:
			
				type_str="Debug";
				
				break;
				
			case Service::LogType::SecuritySuccess:
			
				type_str="Security Success";
				
				break;
				
			case Service::LogType::SecurityFailure:
			
				type_str="Security Failure";
				
				break;
				
			case Service::LogType::Critical:
			
				type_str="Critical";
				
				break;
				
			case Service::LogType::Alert:
			
				type_str="Alert";
				
				break;
				
			case Service::LogType::Emergency:
			
				type_str="Emergency";
				
				break;
		
		}
		
		return type_str;

	}


}

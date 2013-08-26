#include <data_provider.hpp>


namespace MCPP {


	DataProvider::DataProvider () noexcept {	}


	DataProvider::~DataProvider () noexcept {	}
	
	
	static const String success("Success");
	static const String error("Error");
	static const String information("Information");
	static const String warning("Warning");
	static const String debug("Debug");
	static const String security_success("Security Success");
	static const String security_failure("Security Failure");
	static const String critical("Critical");
	static const String alert("Alert");
	static const String emergency("Emergency");
	
	
	const String & DataProvider::GetLogType (Service::LogType type) noexcept {
	
		switch (type) {
		
			case Service::LogType::Success:return success;
			case Service::LogType::Error:return error;
			case Service::LogType::Information:
			default:return information;
			case Service::LogType::Warning:return warning;
			case Service::LogType::Debug:return debug;
			case Service::LogType::SecuritySuccess:return security_success;
			case Service::LogType::SecurityFailure:return security_failure;
			case Service::LogType::Critical:return critical;
			case Service::LogType::Alert:return alert;
			case Service::LogType::Emergency:return emergency;
		
		}

	}


}

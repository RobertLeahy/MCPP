#include <world/world.hpp>
#include <random_device.hpp>
#include <server.hpp>


namespace MCPP {


	static const String log_new_seed("Generated new seed - {0}");
	static const String log_str_seed("Set seed to \"{0}\" => {1}");
	static const String log_int_seed("Set seed to {0}");


	void World::set_seed (Nullable<String> material) {
	
		String log;
	
		//	If there is no material, use the
		//	cryptographically secure PRNG to
		//	get the seed
		if (material.IsNull()) {
		
			seed=RandomDevice<UInt64>{}();
			
			log=String::Format(
				log_new_seed,
				seed
			);
		
		//	Otherwise figure out how to translate
		//	the string
		//
		//	Attempt to parse an integer from the
		//	string
		} else if (material->ToInteger(&seed)) {
		
			//	Generate log string
			
			log=String::Format(
				log_int_seed,
				seed
			);
		
		} else {
		
			//	That didn't work...
			
			//	Apply djb2
			
			//	Normalize first
			material->Normalize(NormalizationForm::NFC);
			
			seed=5381;
			
			for (auto cp : material->CodePoints()) {
			
				seed*=33;
				seed^=cp;
			
			}
			
			log=String::Format(
				log_str_seed,
				*material,
				seed
			);
		
		}
		
		//	Write to log
		Server::Get().WriteLog(
			log,
			Service::LogType::Information
		);
	
	}
	
	
	UInt64 World::Seed () const noexcept {
	
		return seed;
	
	}


}

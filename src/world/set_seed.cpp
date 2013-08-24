#include <world/world.hpp>


namespace MCPP {


	void WorldContainer::set_seed (Nullable<String> material) {
	
		//	If there is no material, use the
		//	cryptographically secure PRNG to
		//	get the seed
		if (material.IsNull()) {
		
			seed=CryptoRandom<UInt64>();
		
		//	Otherwise figure out how to translate
		//	the string
		//
		//	Attempt to parse an integer from the
		//	string
		} else if (!material->ToInteger(&seed)) {
		
			//	That didn't work...
			
			//	Apply djb2
			
			//	Normalize first
			material->Normalize(NormalizationForm::NFC);
			
			seed=5381;
			
			for (auto cp : material->CodePoints()) {
			
				seed*=33;
				seed^=cp;
			
			}
		
		}
	
	}


}

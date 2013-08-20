#include <world/world.hpp>
#include <cstring>


namespace MCPP {


	class HasSkylightHelper {
	
	
		private:
		
		
			bool has_skylight [256];
	
	
		public:
			
			
			inline bool & operator [] (SByte offset) noexcept {
			
				//	Union converts signed dimension
				//	into unsigned array index
				union {
					SByte in;
					Byte out;
				};
				
				in=offset;
				
				return has_skylight[out];
			
			}
			
			
			HasSkylightHelper () noexcept {
			
				memset(
					has_skylight,
					0,
					sizeof(has_skylight)
				);
				
				//	Overworld has skylight, all
				//	other vanilla dimensions do
				//	not
				(*this)[0]=true;
			
			}
	
	
	};
	
	
	static HasSkylightHelper has_skylight;
	
	
	bool HasSkylight (SByte dimension) noexcept {
	
		return has_skylight[dimension];
	
	}
	
	
	void SetHasSkylight (SByte dimension, bool has_skylight) noexcept {
	
		MCPP::has_skylight[dimension]=has_skylight;
	
	}


}

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <random_device.hpp>
#include <cstddef>
#include <iterator>


namespace MCPP {


	/**
	 *	A seed sequence which provides
	 *	cryptographically secure pseudo-random
	 *	numbers.
	 */
	class SeedSequence {
	
	
		public:
		
		
			typedef UInt32 result_type;
		
		
			template <typename RandomIt>
			void generate (RandomIt begin, RandomIt end) {
			
				RandomDevice<typename std::iterator_traits<RandomIt>::value_type> gen;
				
				do *begin=gen();
				while ((++begin)!=end);
			
			}
			
			
			std::size_t size () const noexcept {
			
				return 0;
			
			}
			
			
			template <typename OutputIt>
			void param (OutputIt) const noexcept {	}
	
	
	};


}

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <random>
#include <limits>
#include <stdexcept>


namespace MCPP {


	/**
	 *	Generates random numbers suitable for
	 *	use in cryptography.
	 *
	 *	\param [in] num
	 *		The number of cryptographically-secure
	 *		random bytes to generate.
	 *
	 *	\return
	 *		A buffer of random bytes.
	 */
	Vector<Byte> CryptoRandom (Word num);


	/**
	 *	Generates random numbers unsuitable
	 *	for use in cryptography in a thread
	 *	safe manner.
	 *
	 *	\tparam T
	 *		The type of integer that shall
	 *		be generated.
	 */
	template <typename T>
	class Random {
	
	
		private:
	
	
			Mutex lock;
			std::default_random_engine generator;
			std::uniform_int_distribution<T> distribution;
			
			
			inline void seed () {
			
				Vector<Byte> buffer(
					CryptoRandom(
						sizeof(typename decltype(generator)::result_type)
					)
				);
				
				union {
					Byte * byte_ptr;
					typename decltype(generator)::result_type * seed_ptr;
				};
				
				byte_ptr=static_cast<Byte *>(buffer);
				
				generator.seed(*seed_ptr);
			
			}
			
			
		public:
		
		
			Random () : distribution(
				std::numeric_limits<T>::min(),
				std::numeric_limits<T>::max()
			) {
			
				seed();
			
			}
			
			
			Random (T min, T max) : distribution(
				min,
				max
			) {
			
				seed();
			
			}
			
			
			T operator () () {
			
				return lock.Execute([&] () {	return distribution(generator);	});
			
			}
	
	
	};


}

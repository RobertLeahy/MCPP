/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <random>
#include <limits>


namespace MCPP {


	/**
	 *	Generates pseudo-random numbers suitable for
	 *	use in cryptography.
	 *
	 *	\param [in] ptr
	 *		A pointer at which to store the
	 *		cryptographically-secure pseudo-random
	 *		bytes.
	 *	\param [in] num
	 *		The number of cryptographically-secure
	 *		pseudo-random bytes to generate.
	 */
	void CryptoRandom (Byte * ptr, Word num);
	
	
	/**
	 *	Generates a pseudo-random number suitable for
	 *	use in cryptography.
	 *
	 *	\tparam T
	 *		The type of value to generate.
	 *
	 *	\return
	 *		A random value of type \em T.
	 */
	template <typename T>
	T CryptoRandom () {
	
		union {
			Byte bytes [sizeof(T)];
			T value;
		};
		
		CryptoRandom(bytes,sizeof(T));
		
		return value;
	
	}
	


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
			
				generator.seed(
					CryptoRandom<typename decltype(generator)::result_type>()
				);
			
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

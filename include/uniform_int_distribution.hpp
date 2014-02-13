/**
 *	\file
 */
 
 
#pragma once


#include <limits>
#include <random>
#include <type_traits>
#include <utility>


namespace MCPP {


	/**
	 *	Wraps the std::uniform_int_distribution
	 *	class and allows for that type to additionally
	 *	be instantiated for char, unsigned char, and
	 *	signed char.
	 */
	template <typename T>
	class UniformIntDistribution {
	
	
		private:
		
		
			typedef typename std::conditional<
				(
					std::is_same<T,char>::value ||
					std::is_same<T,unsigned char>::value ||
					std::is_same<T,signed char>::value
				),
				typename std::conditional<
					std::is_signed<T>::value,
					short,
					unsigned short
				>::type,
				T
			>::type type;
			
			
			typedef std::uniform_int_distribution<type> dist_type;
			
			
			dist_type dist;
		
		
		public:
		
		
			typedef T result_type;
		
		
			explicit UniformIntDistribution (
				T min=std::numeric_limits<T>::min(),
				T max=std::numeric_limits<T>::max()
			) noexcept(
				std::is_nothrow_constructible<
					dist_type,
					type,
					type
				>::value
			) : dist(
				static_cast<type>(min),
				static_cast<type>(max)
			) {	}
			
			
			template <typename TGenerator>
			T operator () (TGenerator & gen) noexcept(
				noexcept(std::declval<dist_type>()(gen))
			) {
			
				return static_cast<T>(dist(gen));
			
			}
			
			
			T min () const noexcept(
				noexcept(std::declval<dist_type>().min())
			) {
			
				return static_cast<T>(dist.min());
			
			}
			
			
			T max () const noexcept(
				noexcept(std::declval<dist_type>().max())
			) {
			
				return static_cast<T>(dist.max());
			
			}
	
	
	};


}

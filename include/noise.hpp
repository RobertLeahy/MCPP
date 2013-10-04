/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>
#include <utility>
#include <type_traits>
#include <cmath>
#include <cstring>
#include <functional>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <fma.hpp>


#pragma GCC optimize ("fast-math")
 
 
namespace MCPP {


	/**
	 *	Generates simplex noise.
	 */
	class Simplex {
	
	
		private:
		
		
			Byte permutation [512];
			
			
			template <typename T>
			inline Byte get_random (T & gen, Byte bound) noexcept(noexcept(gen())) {
			
				typedef typename T::result_type stype;
				typedef typename std::make_unsigned<stype>::type type;
				
				union {
					type r;
					stype r_in;
				};
				
				//	If the bound is the maximum integer
				//	for the unsigned result type, it cannot
				//	be affected by modulo bias, so we simply
				//	return the next integer generated
				if (bound==std::numeric_limits<type>::max()) {
				
					r_in=gen();
					
					return r;
				
				}
				
				type d=static_cast<type>(bound)+1;
				//	This is the maximum number the random
				//	number generator may yield, divided
				//	by the divisor.
				type q=std::numeric_limits<type>::max()/d;
				
				//	Loop until we retrieve a random
				//	number unaffected by modulo bias
				//	over the given range
				do r_in=gen();
				while(
					//	This eliminates modulo bias by
					//	asking:
					//
					//	If I restrict the inputs to
					//	modulus to the range [(r/d)*d,((r/d)+1)*d)
					//	could I obtain all values within
					//	the bound given the maximum output
					//	of the random number generator?
					//
					//	If the answer is no a new number is
					//	obtained.
					//
					//	Example:
					//
					//	The function is called with a generator
					//	which generates [-128,127] (i.e. the range
					//	of a signed byte), and a bound of 5 (i.e.
					//	the output range is [0,5].
					//
					//	The union of r and r_in will map this range
					//	to an output range of [0,255] (i.e. the range
					//	of an unsigned byte).
					//
					//	If the random number generates a number on
					//	the range [252,255], modulo bias would occur.
					//
					//	Consider the outputs that would be yielded
					//	by blindly applying modulus to this range:
					//
					//	252 = 0
					//	253 = 1
					//	254 = 2
					//	255 = 3
					//
					//	The numbers 4 and 5 from the desired output
					//	range are not present, increasing the likelihood
					//	of obtaining outputs in the range [0,3].
					//
					//	In this case:
					//
					//	d=6
					//	q=255/6=42
					//
					//	Consider the case r=252:
					//
					//	(r/d)+1=43
					//
					//	This means that in order for 252 not to
					//	produce modulo bias (if not discarded)
					//	the random number generator would have to
					//	be capable of generating numbers up to
					//	at least (43*d)-1=257 (which it is not).
					//
					//	But we see that ((r/d)+1)>q, which
					//	causes this result to be discarded.
					((r/d)+1)>q
				);
					
				//	Current result will not produce
				//	modulo bias, apply modulus and return
				return static_cast<Byte>(r%d);
			
			}
			
			
			template <typename T>
			inline void init (T & gen) noexcept(noexcept(gen())) {
			
				//	Populate and shuffle the first half
				//	of the table using Fisher-Yates-Knuth
				//	shuffle
				for (Word i=0;i<(sizeof(permutation)/2);++i) {
				
					permutation[i]=static_cast<Byte>(i);
					
					std::swap(
						permutation[i],
						permutation[
							get_random(
								gen,
								static_cast<Byte>(i)
							)
						]
					);
				
				}
				
				//	Copy the first half of the table
				//	into the last half
				memcpy(
					&permutation[sizeof(permutation)/2],
					permutation,
					sizeof(permutation)/2
				);
			
			}
			
			
		public:
		
		
			/**
			 *	Creates a new simplex noise generator
			 *	with a randomly-chosen seed.
			 */
			Simplex ();
			/**
			 *	Creates a new simplex noise generator
			 *	seeded with a certain seed.
			 *
			 *	\param [in] seed
			 *		The seed which shall be used to
			 *		seed the noise generator.
			 */
			explicit Simplex (UInt64 seed) noexcept;
			/**
			 *	Creates a new simplex noise generator
			 *	seeded by a provided random number
			 *	generator.
			 *
			 *	\param [in] gen
			 *		A random number generator which
			 *		shall be used to seed the noise
			 *		generator.
			 */
			template <typename T, typename=typename std::enable_if<!std::numeric_limits<T>::is_integer>::type>
			explicit Simplex (T & gen) noexcept(noexcept(std::declval<Simplex>().init(gen))) {
			
				init(gen);
			
			}
			
			
			/**
			 *	Retrieves raw 2D noise.
			 *
			 *	\param [in] x
			 *		The x value.
			 *	\param [i] y
			 *		The y value.
			 *
			 *	\return
			 *		The raw noise output.
			 */
			Double operator () (Double x, Double y) const noexcept;
			/**
			 *	Retrieves raw 3D noise.
			 *
			 *	\param [in] x
			 *		The x value.
			 *	\param [in] y
			 *		The y value.
			 *	param [in] z
			 *		The z value.
			 *
			 *	\return
			 *		The raw noise output.
			 */
			Double operator () (Double x, Double y, Double z) const noexcept;
			/**
			 *	Retrieves raw 4D noise.
			 *
			 *	\param [in] w
			 *		The w value.
			 *	\param [in] x
			 *		The x value.
			 *	\param [in] y
			 *		The y value.
			 *	\param [in] z
			 *		The z value.
			 *
			 *	\return
			 *		The raw noise output.
			 */
			Double operator () (Double w, Double x, Double y, Double z) const noexcept;
	
	
	};
	
	
	inline Double Select (Double lo, Double hi, Double val) noexcept {
	
		return fma(
			hi-lo,
			val,
			lo
		);
	
	}
	
	
	inline Double Normalize (Double lo, Double hi, Double val) noexcept {
	
		if (lo==hi) {

			if (val==lo) return 0.5;
			if (val>lo) return 1;
			return 0;

		}

		bool complement;
		if (lo>hi) {

			complement=true;
			std::swap(lo,hi);

		} else {

			complement=false;

		}

		Double difference=val-lo;
		Double range=hi-lo;

		if (difference<0) return complement ? 1 : 0;
		if (difference>range) return complement ? 0 : 1;

		Double result=difference/range;

		return complement ? (1-result) : result;
	
	}
	
	
	inline Double Scale (Double lo, Double hi, Double in_lo, Double in_hi, Double in) noexcept {
	
		return Select(
			lo,
			hi,
			Normalize(
				in_lo,
				in_hi,
				in
			)
		);
	
	}
	
	
	template <typename... Args>
	Double Scale (Double lo, Double hi, const Simplex & gen, Args &&... args) noexcept {
	
		return fma(
			gen(std::forward<Args>(args)...),
			(hi-lo)/2,
			(hi+lo)/2
		);
	
	}
	
	
	/**
	 *	\cond
	 */
	
	
	inline Double octave_helper (Double arg, Double frequency) noexcept {
	
		return arg*frequency;
	
	}
	
	
	/**
	 *	\endcond
	 */
	
	
	template <typename T, typename... Args>
	Double Octave (Word octaves, Double persistence, Double frequency, T && func, Args &&...args) noexcept(
		noexcept(func(std::forward<Args>(args)...))
	) {
	
		Double total=0;
		Double amplitude=1;
		Double max_amp=0;
		
		for (Word i=0;i<octaves;++i) {
		
			total=fma(
				func(
					octave_helper(
						std::forward<Args>(args),
						frequency
					)...
				),
				amplitude,
				total
			);
			
			frequency*=2;
			max_amp+=amplitude;
			amplitude*=persistence;
		
		}
		
		return total/max_amp;
	
	}
	
	
	inline Double Bias (Double bias, Double input) noexcept {
	
		return pow(
			input,
			log(bias)/log(0.5)
		);
	
	}
	
	
	inline Double Gain (Double gain, Double input) noexcept {
	
		Double complement=1-gain;
		input*=2;
	
		return (
			(input<0.5)
				?
					(Bias(
						complement,
						input
					)/2)
				:	(1-(Bias(
						complement,
						2-input
					)/2))
		);
		
	}
	
	
	inline Double Ridged (Double input) noexcept {
	
		return 1-fabs(input);
	
	}


}


#pragma GCC reset_options

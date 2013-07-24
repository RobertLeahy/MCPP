/**
 *	\file
 */
 
 
#include <rleahylib/rleahylib.hpp>
#include <utility>
#include <type_traits>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <limits>
 
 
namespace MCPP {


	/**
	 *	Generates simplex noise.
	 */
	class Simplex {
	
	
		private:
		
		
			Byte permutation [512];
			
			
			template <typename T>
			inline void init (T & gen) noexcept(
				noexcept(
					std::shuffle(
						std::declval<Byte *>(),
						std::declval<Byte *>(),
						gen
					)
				)
			) {
			
				//	Populate the first half of the table
				for (Word i=0;i<(sizeof(permutation)/2);++i) permutation[i]=static_cast<Byte>(i);
				
				//	Shuffle
				std::shuffle(
					permutation,
					&permutation[sizeof(permutation)/2],
					gen
				);
				
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
			explicit Simplex (T & gen) noexcept(noexcept(init(gen))) {
			
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
	
	
	/**
	 *	\cond
	 */
	
	
	template <typename T, typename=void>
	class SimplexChain;
	
	
	template <typename T>
	class SimplexChain<T,typename std::enable_if<std::is_rvalue_reference<T &&>::value>::type> {
	
	
		public:
		
		
			typedef typename std::remove_reference<T>::type Type;
	
	
		private:
		
		
			Type wrapped;
			
			
		public:
		
		
			SimplexChain () = delete;
			SimplexChain (T && wrap) noexcept(std::is_nothrow_move_constructible<typename std::decay<T>::type>::value) : wrapped(std::move(wrap)) {	}
			SimplexChain (const SimplexChain &) = default;
			SimplexChain (SimplexChain &&) = default;
			SimplexChain & operator = (const SimplexChain &) = default;
			SimplexChain & operator = (SimplexChain &&) = default;
			
			
			template <typename... Args>
			decltype(
				std::declval<Type>()(std::declval<Args>()...)
			) operator () (Args &&... args) const noexcept(
				noexcept(std::declval<Type>()(std::declval<Args>()...))
			) {
			
				return wrapped(std::forward<Args>(args)...);
			
			}
	
	
	};
	
	
	template <typename T>
	class SimplexChain<T,typename std::enable_if<std::is_lvalue_reference<T &&>::value>::type> {
	
	
		public:
		
		
			typedef const typename std::remove_reference<T>::type & Type;
		
		
		private:
		
		
			Type wrapped;
			
			
		public:
		
		
			SimplexChain () = delete;
			SimplexChain (Type wrap) noexcept : wrapped(wrap) {	}
			SimplexChain (const SimplexChain &) = default;
			SimplexChain (SimplexChain &&) = default;
			SimplexChain & operator = (const SimplexChain &) = default;
			SimplexChain & operator = (SimplexChain &&) = default;
			
			
			template <typename... Args>
			decltype(
				std::declval<Type>()(std::declval<Args>()...)
			) operator () (Args &&... args) const noexcept(
				noexcept(std::declval<Type>()(std::declval<Args>()...))
			) {
			
				return wrapped(std::forward<Args>(args)...);
			
			}
	
	
	};
	
	
	template <typename T>
	class Scale {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			Double lo;
			Double hi;
			
			
		public:
		
		
			Scale () = delete;
			Scale (
				T wrap,
				Double lo,
				Double hi
			) noexcept(std::is_nothrow_constructible<SimplexChain<T>,T>::value)
				:	wrapped(std::forward<T>(wrap)),
					lo(lo),
					hi(hi)
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(std::declval<SimplexChain<T>>()(std::declval<Args>()...))
			) {
			
				return ((wrapped(
					std::forward<Args>(args)...
				)*(hi-lo))/2)+((hi+lo)/2);
			
			}
	
	
	};
	
	
	template <typename T>
	class Octave {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			Word octaves;
			Double persistence;
			Double frequency;
			
			
			inline Double process_arg (Double arg, Double frequency) const noexcept {
			
				return arg*frequency;
			
			}
			
			
		public:
		
		
			Octave () = delete;
			Octave (
				T wrap,
				Word octaves,
				Double persistence,
				Double frequency
			) noexcept(std::is_nothrow_constructible<SimplexChain<T>,T>::value)
				:	wrapped(std::forward<T>(wrap)),
					octaves(octaves),
					persistence(persistence),
					frequency(frequency)
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(std::declval<SimplexChain<T>>()(std::declval<Args>()...))
			) {
			
				Double total=0;
				Double amplitude=1;
				Double max_amp=0;
				Double frequency=this->frequency;
				
				for (Word i=0;i<octaves;++i) {
				
					total+=wrapped(process_arg(args,frequency)...)*amplitude;
					
					frequency*=2;
					max_amp+=amplitude;
					amplitude*=persistence;
				
				}
				
				return total/max_amp;
			
			}
	
	
	};
	
	
	template <typename T>
	class Bias {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			Double bias;
			
			
		public:
		
		
			Bias () = delete;
			Bias (
				T wrap,
				Double bias
			) noexcept(std::is_nothrow_constructible<SimplexChain<T>,T>::value)
				:	wrapped(std::forward<T>(wrap)),
					bias(bias)
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(std::declval<SimplexChain<T>>()(std::declval<Args>()...))
			) {
			
				return (pow(
					(wrapped(std::forward<Args>(args)...)+1)/2,
					log(bias)/log(0.5)
				)*2)-1;
			
			}
	
	
	};
	
	
	template <typename T>
	class Gain {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			Double gain;
			
			
		public:
		
		
			Gain () = delete;
			Gain (
				T wrap,
				Double gain
			) noexcept(std::is_nothrow_constructible<SimplexChain<T>,T>::value)
				:	wrapped(std::forward<T>(wrap)),
					gain(gain)
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(std::declval<SimplexChain<T>>()(std::declval<Args>()...))
			) {
			
				Double noise=wrapped(std::forward<Args>(args)...)+1;
				Double input=1-gain;
				Double bias=(noise>1) ? 2-noise : noise;
				
				Double result=pow(
					input,
					log(bias)/log(0.5)
				);
				
				if (noise>1) result=2-result;
				
				return result-1;
			
			}
	
	
	};
	
	
	template <typename TargetT, typename T>
	class Convert {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			
			
		public:
		
		
			Convert () = delete;
			Convert (
				T wrap
			) noexcept(std::is_nothrow_constructible<SimplexChain<T>,T>::value)
				:	wrapped(std::forward<T>(wrap))
			{	}
			
			
			template <typename... Args>
			TargetT operator () (Args &&... args) const noexcept(
				noexcept(std::declval<SimplexChain<T>>()(std::declval<Args>()...))
			) {
			
				return TargetT(
					wrapped(std::forward<Args>(args)...)
				);
			
			}
	
	
	};
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Adds a scale filter to the filter
	 *	chain ontop of a simplex noise
	 *	generator.
	 *
	 *	A scale filter takes the noise generator's
	 *	output, which is in the range (-1,1), and
	 *	scales it such that it is in the
	 *	range (<i>lo</i>,<i>hi</i>).
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter
	 *		to the end of.
	 *	\param [in] lo
	 *		The lower bound on the new range of
	 *		the noise generator's output.
	 *	\param [in] hi
	 *		The upper bound on the new range of
	 *		the noise generator's output.
	 *
	 *	\return
	 *		A filter which is the new end of the
	 *		filter chain.
	 */
	template <typename T>
	Scale<T> MakeScale (T && wrap, Double lo, Double hi) noexcept(
		std::is_nothrow_constructible<Scale<T>,T &&,Double,Double>::value
	) {
	
		return Scale<T>(
			std::forward<T>(wrap),
			lo,
			hi
		);
	
	}
	
	
	/**
	 *	Adds an octave filter to the filter
	 *	chain ontop of a simplex noise generator.
	 *
	 *	An octave filter gets output from the noise
	 *	generator for each octave, and for each such
	 *	octave adds that output to the noise
	 *	generator's output.  For each successive
	 *	octave the function shall be higher frequency
	 *	and lower amplitude.
	 *
	 *	The higher the persistence, the more of each
	 *	successive octave shall be added to the result.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to
	 *		the end of.
	 *	\param [in] octaves
	 *		The number of octaves.
	 *	\param [in] persistence
	 *		A value between 0 and 1 which specifies
	 *		the amount of preceding octaves which
	 *		shall persist on each successive octave.
	 *	\param [in] frequency
	 *		The starting frequency.
	 *
	 *	\return
	 *		A filter which is the new end of the
	 *		filter chain.
	 */
	template <typename T>
	Octave<T> MakeOctave (T && wrap, Word octaves, Double persistence, Double frequency) noexcept(
		std::is_nothrow_constructible<Scale<T>,T &&,Word,Double,Double>::value
	) {
	
		return Octave<T>(
			std::forward<T>(wrap),
			octaves,
			persistence,
			frequency
		);
	
	}
	
	
	/**
	 *	Adds a bias filter to the filter chain ontop
	 *	of a simplex noise generator.
	 *
	 *	A bias filter gets output from the noise generator
	 *	and &quot;pushes&quot; the values towards 1
	 *	if the bias is greater than 0.5, or towards
	 *	-1 if the bias is less than 0.5.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to the
	 *		end of.
	 *	\param [in] bias
	 *		The bias value for this filter.
	 *
	 *	\return
	 *		A filter which is the new end of the
	 *		filter chain.
	 */
	template <typename T>
	Bias<T> MakeBias (T && wrap, Double bias) noexcept(
		std::is_nothrow_constructible<Bias<T>,T &&,Double>::value
	) {
	
		return Bias<T>(
			std::forward<T>(wrap),
			bias
		);
	
	}
	
	
	/**
	 *	Adds a gain filter to the filter chain ontop
	 *	of a simplex noise generator.
	 *
	 *	A gain filter gets output from the noise generator
	 *	and &quot;pushes&quot; it towards 0 if the gain is
	 *	less than 0.5, or towards -1 and 1 if the gain
	 *	is greater than 0.5.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to the
	 *		end of.
	 *	\param [in] gain
	 *		The gain value for this filter.
	 *
	 *	\return
	 *		A filter which is the new end of the filter
	 *		chain.
	 */
	template <typename T>
	Gain<T> MakeGain (T && wrap, Double gain) noexcept(
		std::is_nothrow_constructible<Gain<T>,T &&,Double>::value
	) {
	
		return Gain<T>(
			std::forward<T>(wrap),
			gain
		);
	
	}
	
	
	/**
	 *	Adds a convert filter to the filter chain ontop of
	 *	a simplex noise generator.
	 *
	 *	A convert filter takes the output of the filter chain
	 *	and converts it to some type other than Double.
	 *
	 *	\tparam TargetT
	 *		The type to convert the output of the
	 *		noise generator to.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to the
	 *		end of.
	 *
	 *	\return
	 *		A filter which is the new end of the filter
	 *		chain.
	 */
	template <typename TargetT, typename T>
	typename std::enable_if<
		std::is_convertible<
			Double,
			TargetT
		>::value,
		Convert<TargetT,T>
	>::type MakeConvert (T && wrap) noexcept(
		std::is_nothrow_constructible<Convert<TargetT,T>,T &&>::value
	) {
	
		return Convert<TargetT,T>(
			std::forward<T>(wrap)
		);
	
	}


}
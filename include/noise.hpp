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
	
	
	/**
	 *	\cond
	 */
	 
	 
	template <Word dimensions, typename=void>
	class Gradient;
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Generates a gradient.
	 *
	 *	\tparam dimensions
	 *		The number of dimensions of
	 *		the gradient generator.
	 */
	template <Word dimensions>
	class Gradient<dimensions,typename std::enable_if<(dimensions>0)>::type> {
	
	
		private:
		
		
			Double points [dimensions];
			Double origin [dimensions];
			Double len;
			
			
			template <Word>
			static constexpr Double dot_product () noexcept {
			
				return 0;
			
			}
			
			
			template <Word i, typename... Args>
			inline Double dot_product (Double x, Args &&... args) const noexcept {
			
				return fma(
					x,
					points[i],
					dot_product<i+1>(std::forward<Args>(args)...)
				);
			
			}
			
			
			template <Word>
			static inline void assign_points () noexcept {	}
			
			
			template <Word i, typename... Args>
			inline void assign_points (Double x, Args &&... args) noexcept {
			
				points[i]=x;
				origin[i]=0;
				
				assign_points<i+1>(std::forward<Args>(args)...);
			
			}
			
			
			inline void calc_len () noexcept {
			
				len=0;
				for (Word i=0;i<dimensions;++i) len=fma(
					points[i],
					points[i],
					len
				);
				len=sqrt(len);
			
			}
			
			
			template <Word>
			static inline void assign_points_and_origin () noexcept {	}
			
			
			template <Word i, typename... Args>
			inline void assign_points_and_origin (Double x, Args &&... args) noexcept {
			
				if (i>=dimensions) {
				
					Word offset=i%dimensions;
					
					points[offset]=x-origin[offset];
				
				} else {
				
					origin[i]=x;
				
				}
				
				assign_points_and_origin<i+1>(std::forward<Args>(args)...);
			
			}
			
			
			template <Word>
			inline void offset () const noexcept {	}
			
			
			template <Word i, typename... Args>
			inline void offset (Double & curr, Args &... args) const noexcept {
			
				curr-=origin[i];
				
				offset<i+1>(args...);
			
			}
			
			
			template <typename... Args>
			inline Double impl (Args... args) const noexcept {
			
				offset<0>(args...);
				
				Double dp=dot_product<0>(args...);
				
				if (dp<=0) return -1;
				
				Double proj_len=dp/len;
				
				if (proj_len>=len) return 1;
				
				return fma(
					proj_len/len,
					2,
					-1
				);
			
			}
	
	
		public:
		
		
			Gradient () = delete;
			/**
			 *	Creates a new gradient generator based on
			 *	a specific line.
			 *
			 *	\param [in] point
			 *		Together with \em args gives the
			 *		coordinate of the vector which represents
			 *		the line along which the gradient
			 *		shall lie.
			 *	\param [in] args
			 *		Together with \em point gives the
			 *		coordinate of the vector which represents
			 *		the line along which the gradient
			 *		shall lie.
			 */
			template <typename... Args>
			explicit Gradient (typename std::enable_if<
				sizeof...(Args)==(dimensions-1),
				Double
			>::type point, Args &&... args) noexcept {
			
				points[0]=point;
				
				assign_points<1>(std::forward<Args>(args)...);
				
				calc_len();
			
			}
			/**
			 *	Creates a new gradient generator based on
			 *	a specific line.
			 *
			 *	\param [in] offset
			 *		Together wiht \em args gives the
			 *		start and end (respectively)
			 *		coordinates of the vector which
			 *		represents the line along which
			 *		the gradient shall lie.
			 *	\param [in] args
			 *		Together with \em point gives the
			 *		start and end (respectively)
			 *		coordinates of the vector which
			 *		represents the line along which the
			 *		gradient shall lie.
			 *		
			 */
			template <typename... Args>
			Gradient (typename std::enable_if<
				sizeof...(Args)==((dimensions*2)-1),
				Double
			>::type offset, Args &&... args) noexcept {
			
				origin[0]=offset;
				
				assign_points_and_origin<1>(std::forward<Args>(args)...);
				
				calc_len();
			
			}
			
			
			/**
			 *	Retrieves the value at a specific point.
			 *
			 *	\param [in] args
			 *		A point in space at which to retrieve
			 *		the value of the gradient.
			 *
			 *	\return
			 *		The gradient value which is in the range
			 *		[-1,1].
			 */
			template <typename... Args>
			typename std::enable_if<
				sizeof...(Args)==dimensions,
				Double
			>::type operator () (Args &&... args) const noexcept {
			
				if (len==0) return -1;
				
				return impl(Double(args)...);
			
			}
	
	
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
			
			
			static inline Double process_arg (Double arg, Double frequency) noexcept {
			
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
				
					total=fma(
						wrapped(process_arg(args,frequency)...),
						amplitude,
						total
					);
					
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
			
				return fma(
					pow(
						fma(
							wrapped(std::forward<Args>(args)...),
							0.5,
							0.5
						),
						log(bias)/log(0.5)
					),
					2,
					-1
				);
			
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
				Double bias=(noise>1) ? 2-noise : noise;
				
				Double result=pow(
					1-gain,
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
	
	
	template <typename T>
	class Turbulence {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			Double size;
			
			
			static inline Double process_arg (Double arg, Double size) noexcept {
			
				return arg/size;
			
			}
			
			
		public:
		
		
			Turbulence () = delete;
			Turbulence (
				T wrap,
				Double size
			) noexcept(std::is_nothrow_constructible<SimplexChain<T>,T>::value)
				:	wrapped(std::forward<T>(wrap)),
					size(size)
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(std::declval<SimplexChain<T>>()(std::declval<Args>()...))
			) {
			
				Double value=0;
				Double curr_size=size;
				Double cumulator=0;
				
				while (curr_size>=1) {
				
					value+=wrapped(
						process_arg(args,curr_size)...
					)*curr_size;
				
					cumulator+=curr_size;
					curr_size/=2;
				
				}
				
				return value/cumulator;
			
			}
	
	
	};
	
	
	template <typename CallbackT, typename T>
	class Range {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			CallbackT callback;
			
			
		public:
		
		
			Range () = delete;
			Range (
				T wrap,
				CallbackT callback
			) noexcept(
				std::is_nothrow_constructible<SimplexChain<T>,T>::value &&
				std::is_nothrow_constructible<CallbackT,CallbackT &&>::value
			)	:	wrapped(std::forward<T>(wrap)),
					callback(std::move(callback))
			{	}
			
			
			template <typename... Args>
			auto operator () (Args &&... args) const noexcept(
				noexcept(std::declval<SimplexChain<T>>()(std::declval<Args>()...)) &&
				noexcept(callback(wrapped(std::forward<Args>(args)...)))
			) -> decltype(callback(wrapped(std::forward<Args>(args)...))) {
			
				return callback(wrapped(std::forward<Args>(args)...));
			
			}
	
	
	};
	
	
	template <Word flags, typename T, typename TNoise>
	class PerturbateDomain {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			SimplexChain<TNoise> noise;
			
			
			template <Word>
			static inline void perturb (Double) noexcept {	}
			
			
			template <Word i, typename... Args>
			static inline void perturb (Double amount, Double & curr, Args &... args) noexcept {
			
				if (((static_cast<Word>(1)<<i)&flags)!=0) curr+=amount;
				
				perturb<i+1>(amount,args...);
			
			}
			
			
			template <typename... Args>
			inline Double impl (Double amount, Args... args) const noexcept(
				noexcept(wrapped(args...))
			) {
			
				perturb<0>(amount,args...);
				
				return wrapped(args...);
			
			}
			
			
		public:
		
		
			PerturbateDomain () = delete;
			PerturbateDomain (
				T wrap,
				TNoise noise
			) noexcept(
				std::is_nothrow_constructible<SimplexChain<T>,T>::value &&
				std::is_nothrow_constructible<SimplexChain<T>,TNoise>::value
			)	:	wrapped(std::forward<T>(wrap)),
					noise(std::forward<TNoise>(noise))
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(wrapped(Double(args)...)) &&
				noexcept(noise(std::forward<Args>(args)...))
			) {
			
				Double amount=Double(
					noise(
						std::forward<Args>(args)...
					)
				);
				
				return impl(
					amount,
					Double(args)...
				);
			
			}
	
	
	};
	
	
	template <Word flags, typename T>
	class ScaleDomain {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			Double factor;
			
			
			template <Word>
			inline void scale () const noexcept {	}
			
			
			template <Word i, typename... Args>
			inline void scale (Double & curr, Args &... args) const noexcept {
			
				if (((static_cast<Word>(1)<<i)&flags)!=0) curr*=factor;
				
				scale<i+1>(args...);
			
			}
			
			
			template <typename... Args>
			inline Double impl (Args... args) const noexcept(noexcept(wrapped(args...))) {
			
				scale<0>(args...);
				
				return wrapped(args...);
			
			}
			
			
		public:
		
		
			ScaleDomain () = delete;
			ScaleDomain (
				T wrap,
				Double factor
			) noexcept(std::is_nothrow_constructible<SimplexChain<T>,T>::value)
				:	wrapped(std::forward<T>(wrap)),
					factor(factor)
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(wrapped(Double(args)...))
			) {
			
				return impl(Double(args)...);
			
			}
	
	
	};
	
	
	template <typename T>
	class Ridged {
	
	
		private:
		
		
			SimplexChain<T> wrapped;
			
			
		public:
		
		
			Ridged () = delete;
			Ridged (
				T wrap
			) noexcept(std::is_nothrow_constructible<SimplexChain<T>,T>::value)
				:	wrapped(std::forward<T>(wrap))
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(wrapped(std::forward<Args>(args)...))
			) {
			
				return fma(
					1-fabs(wrapped(std::forward<Args>(args)...)),
					2,
					-1
				);
			
			}
	
	
	};
	
	
	template <typename T1, typename T2, typename TCallback>
	class Combine {
	
	
		private:
		
		
			SimplexChain<T1> wrapped1;
			SimplexChain<T2> wrapped2;
			TCallback callback;
			
			
		public:
		
		
			Combine () = delete;
			Combine (
				T1 wrap1,
				T2 wrap2,
				TCallback callback
			) noexcept(
				std::is_nothrow_constructible<
					TCallback,
					decltype(std::move(callback))
				>::value &&
				std::is_nothrow_constructible<
					SimplexChain<T1>,
					T1 &&
				>::value &&
				std::is_nothrow_constructible<
					SimplexChain<T2>,
					T2 &&
				>::value
			)	:	wrapped1(std::forward<T1>(wrap1)),
					wrapped2(std::forward<T2>(wrap2)),
					callback(std::move(callback))
			{	}
			
			
			template <typename... Args>
			Double operator () (Args &&... args) const noexcept(
				noexcept(
					callback(
						wrapped1(std::forward<Args>(args)...),
						wrapped2(std::forward<Args>(args)...)
					)
				)
			) {
			
				return callback(
					wrapped1(std::forward<Args>(args)...),
					wrapped2(std::forward<Args>(args)...)
				);
			
			}
	
	
	};
	
	
	/**
	 *	\endcond
	 */
	
	
	/**
	 *	Adds a scale filter to the filter
	 *	chain ontop of a noise
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
	 *	chain ontop of a noise generator.
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
	 *	of a noise generator.
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
	 *	of a noise generator.
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
	
	
	/**
	 *	Adds a turbulence filter to the filter chain ontop
	 *	of a noise generator.
	 *
	 *	A turbulence filter combines various noise samples
	 *	together at various scales.  So a certain pass might
	 *	create a gentle curve, and then a subsequent pass will
	 *	unsmooth that curve by adding small inperfections obtained
	 *	by getting more output from the noise generator, scaling
	 *	it down, and adding it to the resulting noise.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to the end of.
	 *	\param [in] size
	 *		The start zooming factor of this filter.
	 *
	 *	\return
	 *		A filter which is the new end of the filter
	 *		chain.
	 */
	template <typename T>
	Turbulence<T> MakeTurbulence (T && wrap, Double size) noexcept(
		std::is_nothrow_constructible<Turbulence<T>,T &&,Double>::value
	) {
	
		return Turbulence<T>(
			std::forward<T>(wrap),
			size
		);
	
	}
	
	
	/**
	 *	Adds a range filter to the filter chain ontop of
	 *	a noise generator.
	 *
	 *	A range filter uses a provided callback to transform
	 *	the range in some way.  The provided callback is passed
	 *	the output of the noise generator, and returns a replacement
	 *	value.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to the end of.
	 *	\param [in] callback
	 *		The callback which shall transform the chain's
	 *		range.
	 *
	 *	\return
	 *		A filter which is the new end of the filter chain.
	 */
	template <typename T, typename CallbackT>
	Range<CallbackT,T> MakeRange (T && wrap, CallbackT && callback) noexcept(
		std::is_nothrow_constructible<
			Range<CallbackT,T>,
			T &&,
			CallbackT &&
		>::value
	) {
	
		return Range<CallbackT,T>(
			std::forward<T>(wrap),
			std::forward<CallbackT>(callback)
		);
	
	}
	
	
	/**
	 *	Adds a domain perturbation filter to the filter chain
	 *	ontop of a noise generator.
	 *
	 *	A domain perturbation filter uses the output of a
	 *	noise filter stack to alter the domain of another
	 *	noise filter stack.
	 *
	 *	\tparam flags
	 *		Bit flags which specify which components of
	 *		the domain to perturbate.  Bit flags are set
	 *		from lowest to highest order.  So for three
	 *		dimensional noise the low order bit refers
	 *		to the X component, the second lowest order bit
	 *		to the Y component, and the third lowest order
	 *		bit to the Z component.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to the
	 *		end of.
	 *	\param [in] noise
	 *		The filter chain to use to acquire noise for
	 *		perturbation.
	 *
	 *	\return
	 *		A filter which is the new end of the filter
	 *		chain.
	 */
	template <Word flags, typename T, typename TNoise>
	PerturbateDomain<flags,T,TNoise> MakePerturbateDomain (T && wrap, TNoise && noise) noexcept(
		std::is_nothrow_constructible<
			PerturbateDomain<flags,T,TNoise>,
			T &&,
			TNoise &&
		>::value
	) {
	
		return PerturbateDomain<flags,T,TNoise>(
			std::forward<T>(wrap),
			std::forward<TNoise>(noise)
		);
	
	}
	
	
	/**
	 *	Adds a domain scaling filter to the filter chain
	 *	ontop of a noise generator.
	 *
	 *	A domain scaling filter scales certain components
	 *	of the domain of a noise filter stack by some
	 *	constant.
	 *
	 *	\tparam flags
	 *		Bit flags which specify which components of
	 *		the domain to scale.  Bit flags are set
	 *		from lowest to highest order.  So for three
	 *		dimensional noise the low order bit refers
	 *		to the X component, the second lowest order bit
	 *		to the Y component, and the third lowest order
	 *		bit to the Z component.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to the
	 *		end of.
	 *	\param [in] factor
	 *		The factor by which all domain components
	 *		specified by \em flags shall be multiplied.
	 *
	 *	\return
	 *		A filter which is the new end of the filter
	 *		chain.
	 */
	template <Word flags, typename T>
	ScaleDomain<flags,T> MakeScaleDomain (T && wrap, Double factor) noexcept(
		std::is_nothrow_constructible<ScaleDomain<flags,T>,T &&>::value
	) {
	
		return ScaleDomain<flags,T>(
			std::forward<T>(wrap),
			factor
		);
	
	}
	
	
	/**
	 *	Adds a ridged multifractal filter to the filter
	 *	chain ontop of a noise generator.
	 *
	 *	\param [in] wrap
	 *		The filter chain to add this filter to the
	 *		end of.
	 *
	 *	\return
	 *		A filter which is the new end of the filter
	 *		chain.
	 */
	template <typename T>
	Ridged<T> MakeRidged (T && wrap) noexcept(
		std::is_nothrow_constructible<Ridged<T>,T &&>::value
	) {
	
		return Ridged<T>(
			std::forward<T>(wrap)
		);
	
	}
	
	
	/**
	 *	Adds a combine filter to the filter chain ontop
	 *	of a noise generator.
	 *
	 *	A combine filter takes the output of two filter
	 *	chains and combines that output into a single
	 *	output using a supplied callback function.
	 *
	 *	\param [in] wrap1
	 *		The first filter chain to be combined.
	 *	\param [in] wrap2
	 *		The second filter chain to be combined.
	 *	\param [in] callback
	 *		The callback which shall be used to combine
	 *		the values of \em wrap1 and \em wrap2.
	 *
	 *	\return
	 *		A filter which is the new end of the filter
	 *		chain.
	 */
	template <typename T1, typename T2, typename TCallback>
	Combine<T1,T2,TCallback> MakeCombine (T1 && wrap1, T2 && wrap2, TCallback && callback) noexcept(
		std::is_nothrow_constructible<
			Combine<T1,T2,TCallback>,
			T1 &&,
			T2 &&,
			TCallback &&
		>::value
	) {
	
		return Combine<T1,T2,TCallback>(
			std::forward<T1>(wrap1),
			std::forward<T2>(wrap2),
			std::forward<TCallback>(callback)
		);
	
	}


}
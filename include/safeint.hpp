/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace RLeahyLib {


	/**
	 *	Casts one type of integer safely to another.
	 *
	 *	An exception will be thrown if the value
	 *	of \em f is not representable as type \em To.
	 *
	 *	\tparam To
	 *		The integer type to cast to.
	 *	\tparam From
	 *		The integer type being cast from.  Should
	 *		be deduced.
	 *
	 *	\param [in] f
	 *		The integer to cast.
	 *
	 *	\return
	 *		\em f cast to type \em To.
	 */
	template <typename To, typename From>
	To safe_cast (From f) {
	
		return static_cast<To>(
			SafeInt<From>(f)
		);
	
	}
	
	
	/**
	 *	Wraps an integer in a SafeInt wrapper.
	 *
	 *	\tparam T
	 *		The type of integer to wrap.  Should
	 *		be deduced.
	 *
	 *	\param [in] t
	 *		The integer to wrap.
	 *
	 *	\return
	 *		\em t wrapped in a SafeInt<T>.
	 */
	template <typename T>
	SafeInt<T> MakeSafe (T t) noexcept {
	
		return SafeInt<T>(t);
	
	}


}

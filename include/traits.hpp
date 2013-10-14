/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	\cond
	 */


	namespace GetTypeImpl {


		template <Word target, Word i, typename T, typename... Args>
		class GetType {
		
		
			public:
			
			
				typedef typename GetType<target,i+1,Args...>::Type Type;
		
		
		};
		
		
		template <Word target, typename T, typename... Args>
		class GetType<target,target,T,Args...> {
		
		
			public:
			
			
				typedef T Type;
		
		
		};
		
		
	}
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	Contains a typedef which is the
	 *	\em ith type of the variadic template
	 *	argument \em Args.
	 *
	 *	\tparam i
	 *		The zero-relative index of the
	 *		type to retrieve.
	 *	\tparam Args
	 *		The types to choose from.
	 */
	template <Word i, typename... Args>
	class GetType {
	
	
		public:
		
		
			/**
			 *	A typedef which contains the \em ith
			 *	type of the variadic template argument
			 *	\em Args.
			 */
			typedef typename GetTypeImpl::GetType<i,0,Args...>::Type Type;
	
	
	};


}

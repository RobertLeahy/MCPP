/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <atomic>
#include <type_traits>
#include <utility>


namespace MCPP {


	/**
	 *	Contains a singleton.
	 *
	 *	The singleton is not constructed
	 *	until an attempt is made to access it,
	 *	in which case it is thread safely
	 *	constructed and persists until manually
	 *	destroyed.
	 *
	 *	Destruction is not thread safe.
	 *
	 *	After the singleton is manually destroyed,
	 *	an further attempts to access it will
	 *	reconstruct it.
	 */
	template <typename T>
	class Singleton {
	
	
		private:
		
		
			Nullable<T> obj;
			Mutex lock;
			std::atomic<bool> constructed;
			
			
		public:
		
		
			/**
			 *	Creates a new singleton container.
			 */
			Singleton () noexcept {
			
				constructed=false;
			
			}
			
			
			Singleton (const Singleton &) = delete;
			Singleton (Singleton &&) = delete;
			Singleton & operator = (const Singleton &) = delete;
			Singleton & operator = (Singleton &&) = delete;
			
			
			/**
			 *	Retrieves the singleton, constructing
			 *	it if necessary.
			 *
			 *	Thread safe.
			 *
			 *	\return
			 *		A reference to the contained
			 *		singleton.
			 */
			T & Get () noexcept(std::is_nothrow_constructible<T>::value) {
			
				if (!constructed) {
				
					lock.Execute([&] () {
					
						if (!constructed) {
					
							obj.Construct();
							
							constructed=true;
							
						}
					
					});
				
				}
				
				return *obj;
			
			}
			
			
			/**
			 *	Destroys the contained singleton.
			 *
			 *	Not thread safe.
			 */
			void Destroy () noexcept {
			
				obj.Destroy();
				
				constructed=false;
			
			}
			
	
	};


}

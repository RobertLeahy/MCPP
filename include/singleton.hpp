/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <atomic>
#include <type_traits>
#include <utility>


namespace MCPP {


	template <typename T>
	class Singleton {
	
	
		private:
		
		
			Nullable<T> obj;
			Mutex lock;
			std::atomic<bool> constructed;
			
			
		public:
		
		
			Singleton () noexcept {
			
				constructed=false;
			
			}
			
			
			Singleton (const Singleton &) = delete;
			Singleton (Singleton &&) = delete;
			Singleton & operator = (const Singleton &) = delete;
			Singleton & operator = (Singleton &&) = delete;
			
			
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
			
			
			void Destroy () noexcept {
			
				obj.Destroy();
			
			}
			
	
	};


}

/**
 *	\file
 */
 
 
#pragma once
 
 
#include <rleahylib/rleahylib.hpp>
#include <functional>
#include <type_traits>


/**
 *	\cond
 */


namespace std {


	template <>
	struct hash<String> {
	
	
		public:
	
	
			size_t operator () (String str) const {
			
				//	Normalize string
				str.Normalize(NormalizationForm::NFC);
				
				//	djb2
				size_t retr=5381;
				
				for (auto cp : str.CodePoints()) {
				
					retr*=33;
					retr^=cp;
				
				}
				
				return retr;
			
			}
	
	
	};
	
	
	template <>
	struct hash<IPAddress> {
	
	
		public:
	
	
			size_t operator () (IPAddress ip) const noexcept {
			
				//	Is this an IPv4 or IPv6
				//	address?
				if (ip.IsV6()) {
				
					//	This static assert ensures the operation
					//	planned will work
					static_assert(
						(sizeof(UInt128)%sizeof(size_t))==0,
						"Hash for IP addresses not compatible with integer sizes on this platform"
					);
					
					union {
						UInt128 raw_ip;
						size_t split [sizeof(UInt128)/sizeof(size_t)];
					};
					
					raw_ip=static_cast<UInt128>(ip);
					
					size_t retr=23;
					
					for (auto m : split) {
					
						retr*=31;
						retr+=m;
					
					}
					
					return retr;
				
				}
				
			
				//	Just return the IPv4 address
				//	to use as the hash
				return static_cast<size_t>(
					static_cast<UInt32>(ip)
				);
			
			}
	
	
	};
	
	
	template <typename... Args>
	struct hash<Tuple<Args...>> {
	
	
		private:
		
		
			template <Word i>
			typename std::enable_if<
				i<sizeof...(Args),
				size_t
			>::type compute_hash (size_t curr, const Tuple<Args...> & t) const {
			
				curr*=31;
				
				curr+=std::hash<
					typename std::decay<
						decltype(t.template Item<i>())
					>::type
				>()(t.template Item<i>());
				
				return compute_hash<i+1>(curr,t);
			
			}
			
			
			template <Word i>
			typename std::enable_if<
				i>=sizeof...(Args),
				size_t
			>::type compute_hash (size_t curr, const Tuple<Args...> &) const noexcept {
			
				return curr;
			
			}
			
			
		public:
		
		
			size_t operator () (const Tuple<Args...> & t) const {
			
				return compute_hash<0>(23,t);
			
			}
	
	
	};
	
	
	template <typename T>
	struct hash<Vector<T>> {
	
	
		private:
		
		
			typedef hash<typename std::decay<T>::type> hasher;
	
	
		public:
		
		
			size_t operator () (const Vector<T> & vec) const noexcept(
				std::is_nothrow_constructible<hasher>::value &&
				noexcept(
					declval<hasher>()(declval<const T &>())
				)
			) {
			
				hasher h;
				
				size_t retr=(23*31)+vec.Count();
				
				for (const auto & i : vec) {
				
					retr*=31;
					retr+=h(i);
				
				}
				
				return retr;
			
			}
	
	
	};


}


/**
 *	\endcond
 */

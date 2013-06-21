/**
 *	\file
 */
 
 
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
				
				size_t returnthis=0;
				for (auto cp : str.CodePoints()) {
				
					returnthis+=101*static_cast<size_t>(cp);
				
				}
				
				return returnthis;
			
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
					
					size_t returnthis=0;
					for (Word i=0;i<(sizeof(UInt128)/sizeof(size_t));++i) {
					
						returnthis+=split[i];
					
					}
					
					return returnthis;
				
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
			>::type compute_hash (const Tuple<Args...> & t) const {
			
				size_t hash=compute_hash<i+1>(t);
			
				std::hash<decltype(t.template Item<i>())> hasher;
				
				hash+=hasher(t.template Item<i>());
				
				return hash;
			
			}
			
			
			template <Word i>
			constexpr typename std::enable_if<
				i>=sizeof...(Args),
				size_t
			>::type compute_hash (const Tuple<Args...> &) const noexcept {
			
				return 0;
			
			}
			
			
		public:
		
		
			size_t operator () (const Tuple<Args...> & t) const {
			
				return compute_hash<0>(t);
			
			}
	
	
	};


}


/**
 *	\endcond
 */

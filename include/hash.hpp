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
	struct hash<UInt128> {
	
	
		public:
		
		
			size_t operator () (UInt128 i) const noexcept {
			
				constexpr size_t si=sizeof(i);
				constexpr size_t ss=sizeof(size_t);
			
				//	Make sure sizes are compatible
				static_assert(
					(si<=ss) || ((si%ss)==0),
					"Size of size_t is not greater than or equal to size of 128-bit unsigned integer, and does not "
					"divide it evenly"
				);
				
				constexpr size_t count=(si<=ss) ? 1 : (si/ss);
				
				union {
					UInt128 in;
					size_t out [count];
				};
				if (ss>si) out[0]=0;
				in=i;
				
				if (count==1) return out[0];
				
				size_t retr=23;
				for (auto & o : out) {
				
					retr*=31;
					retr+=o;
				
				}
				
				return retr;
			
			}
	
	
	};
	
	
	template <>
	struct hash<Int128> {
	
	
		public:
		
		
			size_t operator () (Int128 i) const noexcept {
			
				union {
					Int128 in;
					UInt128 out;
				};
				in=i;
				
				return hash<UInt128>{}(out);
			
			}
	
	
	};
	
	
	template <>
	struct hash<IPAddress> {
	
	
		public:
	
	
			size_t operator () (IPAddress ip) const noexcept {
			
				//	Use the hash for UInt128 for IPv6
				//	addresses, the hash for UInt32 for
				//	IPv4 addresses
				return ip.IsV6() ? hash<UInt128>{}(
					static_cast<UInt128>(ip)
				) : hash<UInt32>{}(
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

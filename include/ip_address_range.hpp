/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <hash.hpp>
#include <serializer.hpp>
#include <variant.hpp>
#include <functional>


namespace RLeahyLib {


	/**
	 *	Compares two IP addresses and determines
	 *	which is lesser.
	 *
	 *	IPv4 addresses are less than IPv6 addresses.
	 *	IP addresses in the same family are compared
	 *	numerically.
	 *
	 *	\param [in] a
	 *		The left hand side of the comparison.
	 *	\param [in] b
	 *		The right hand side of the comparison.
	 *
	 *	\return
	 *		\em true if \em a is less than \em b,
	 *		\em false otherwise.
	 */
	bool operator < (const IPAddress & a, const IPAddress & b) noexcept;


}


namespace MCPP {


	/**
	 *	Encapsulates a range of IP addresses.
	 */
	class IPAddressRange {
	
	
		private:
		
		
			enum class RangeType : Byte {
			
				None=0,
				IPAddress=1,
				StartEnd=2,
				Mask=3,
			
			};
			
			
			static void to_bytes (Vector<Byte> &, RangeType);
		
		
			class StartEnd {
			
			
				private:
				
				
					IPAddress start;
					IPAddress end;
			
			
				public:
				
				
					static StartEnd FromBytes (const Byte * &, const Byte *);
				
				
					StartEnd (IPAddress, IPAddress);
					
					
					bool Check (const IPAddress &) const noexcept;
					
					
					explicit operator String () const;
					
					
					void ToBytes (Vector<Byte> &) const;
					
					
					bool operator == (const StartEnd &) const noexcept;
					
					
					bool operator < (const StartEnd &) const noexcept;
					
					
					std::size_t Hash () const noexcept;
			
			
			};
			
			
			class Mask {
			
			
				private:
				
				
					IPAddress base;
					IPAddress mask;
					
					
				public:
				
				
					static Mask FromBytes (const Byte * &, const Byte *);
				
				
					Mask (IPAddress, Word) noexcept;
					Mask (IPAddress, IPAddress);
					
					
					bool Check (const IPAddress &) const noexcept;
					
					
					explicit operator String () const;
					
					
					void ToBytes (Vector<Byte> &) const;
					
					
					bool operator == (const Mask &) const noexcept;
					
					
					bool operator < (const Mask &) const noexcept;
					
					
					std::size_t Hash () const noexcept;
			
			
			};
			
			
			Variant<IPAddress,Mask,StartEnd> item;
	
	
		public:
		
		
			/**
			 *	Creates an IP address range from a
			 *	base IP and a mask IP.
			 *
			 *	\param [in] base
			 *		The base IP.
			 *	\param [in] mask
			 *		The mask IP.
			 *
			 *	\return
			 *		An IP address range which corresponds
			 *		to all IPs which match \em base with
			 *		the mask given by \em mask.
			 */
			static IPAddressRange CreateMask (IPAddress base, IPAddress mask);
		
		
			/**
			 *	Constructs an empty range of IP
			 *	addresses.
			 */
			IPAddressRange () = default;
			/**
			 *	Constructs a range which contains one
			 *	IP address.
			 *
			 *	\param [in] ip
			 *		The sole IP which the range shall
			 *		contain.
			 */
			IPAddressRange (IPAddress ip) noexcept;
			/**
			 *	Constructs a range which contains an
			 *	arbitrary, inclusive, contiguous range
			 *	of IPs.
			 *
			 *	\param [in] start
			 *		The lower bound on the range of IPs.
			 *	\param [in] end
			 *		The upper bound on the range of IPs.
			 */
			IPAddressRange (IPAddress start, IPAddress end);
			/**
			 *	Constructs a range which matches some
			 *	base IP by having some number of high
			 *	order bits which are identical.
			 *
			 *	\param [in] base
			 *		The base IP for this range.
			 *	\param [in] bits
			 *		The number of high order bits which
			 *		another IP must share with \em base
			 *		to be considered to be in this range.
			 */
			IPAddressRange (IPAddress base, Word bits) noexcept;
			
			
			/**
			 *	Determines whether some IP is in this range.
			 *
			 *	\param [in] ip
			 *		The IP address-in-question.
			 *
			 *	\return
			 *		\em true if \em ip is in this range,
			 *		\em false otherwise.
			 */
			bool Check (const IPAddress & ip) const noexcept;
			
			
			/**
			 *	Obtains a string representation of this
			 *	IP range.
			 *
			 *	\return
			 *		A human readable representation of this
			 *		IP range.
			 */
			explicit operator String () const;
			
			
			/**
			 *	Checks to see if this IP range is identical
			 *	to some other IP range.
			 *
			 *	\param [in] other
			 *		The range with which to compare.
			 *
			 *	\return
			 *		\em true if this range and \em other are
			 *		the same range, \em false otherwise.
			 */
			bool operator == (const IPAddressRange & other) const noexcept;
			
			
			/**
			 *	Checks to see if this IP range should be considered
			 *	to be less than some other IP range.
			 *
			 *	\param [in] other
			 *		The IP range with which to compare.
			 *
			 *	\return
			 *		\em true if this IP should be considered to be
			 *		less than \em other, \em false otherwise.
			 */
			bool operator < (const IPAddressRange & other) const noexcept;
			
			
			/**
			 *	\cond
			 */
			
			
			static IPAddressRange FromBytes (const Byte * &, const Byte *);
			void ToBytes (Vector<Byte> &) const;
			std::size_t Hash () const noexcept;
			
			
			/**
			 *	\endcond
			 */
	
	
	};
	
	
	/**
	 *	\cond
	 */
	
	
	template <>
	class Serializer<IPAddressRange> {
	
	
		public:
		
		
			static IPAddressRange FromBytes (const Byte * & begin, const Byte * end) {
			
				return IPAddressRange::FromBytes(begin,end);
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const IPAddressRange & obj) {
			
				obj.ToBytes(buffer);
			
			}
	
	
	};
	
	
	/**
	 *	\endcond
	 */


}


namespace std {


	/**
	 *	\cond
	 */


	template <>
	struct hash<MCPP::IPAddressRange> {
	
	
		public:
		
		
			size_t operator () (const MCPP::IPAddressRange & obj) const noexcept {
			
				return obj.Hash();
			
			}
	
	
	};
	
	
	/**
	 *	\endcond
	 */


}
 
#include <ip_address_range.hpp>
#include <limits>
#include <stdexcept>


namespace RLeahyLib {


	bool operator < (const IPAddress & a, const IPAddress & b) noexcept {
	
		return (
			a.IsV6()
				?	(
						b.IsV6()
							?	(static_cast<UInt128>(a)<static_cast<UInt128>(b))
							:	false
					)
				:	(
						b.IsV6()
							?	true
							:	(static_cast<UInt32>(a)<static_cast<UInt32>(b))
					)
		);
	
	}


}


namespace MCPP {


	IPAddressRange::StartEnd IPAddressRange::StartEnd::FromBytes (const Byte * & begin, const Byte * end) {
	
		//	Get the start address
		auto start=Serializer<IPAddress>::FromBytes(begin,end);
		//	Get the end address and
		//	return
		return StartEnd(
			start,
			Serializer<IPAddress>::FromBytes(begin,end)
		);
	
	}


	static const char * not_same_addr_family="IP addresses are not in the same address family";
	static const char * start_greater_than_end="End IP address less than start IP address";


	IPAddressRange::StartEnd::StartEnd (IPAddress start, IPAddress end) : start(start), end(end) {
	
		//	Verify that the start and end addresses
		//	are in the same family
		if (start.IsV6()!=end.IsV6()) throw std::invalid_argument(not_same_addr_family);
		
		//	Verify that the start and end addresses
		//	are properly ordered
		if (end<start) throw std::invalid_argument(start_greater_than_end);
	
	}
	
	
	bool IPAddressRange::StartEnd::Check (const IPAddress & other) const noexcept {
	
		return !(other<start) && ((other<end) || (other==end));
	
	}
	
	
	IPAddressRange::StartEnd::operator String () const {
	
		String retr(start);
		retr << "-" << String(end);
		
		return retr;
	
	}
	
	
	void IPAddressRange::StartEnd::ToBytes (Vector<Byte> & buffer) const {
	
		Serializer<IPAddress>::ToBytes(buffer,start);
		Serializer<IPAddress>::ToBytes(buffer,end);
	
	}
	
	
	bool IPAddressRange::StartEnd::operator == (const StartEnd & other) const noexcept {
	
		return (start==other.start) && (end==other.end);
	
	}
	
	
	bool IPAddressRange::StartEnd::operator < (const StartEnd & other) const noexcept {
	
		return (start==other.start) ? (end<other.end) : (start<other.start);
	
	}
	
	
	std::size_t IPAddressRange::StartEnd::Hash () const noexcept {
	
		std::size_t retr=23;
		
		std::hash<IPAddress> hasher;
		retr*=33;
		retr^=hasher(start);
		retr*=33;
		retr^=hasher(end);
		
		return retr;
	
	}
	
	
	IPAddressRange::Mask IPAddressRange::Mask::FromBytes (const Byte * & begin, const Byte * end) {
	
		//	Get the base address
		auto base=Serializer<IPAddress>::FromBytes(begin,end);
		//	Get the mask and return
		return Mask(
			base,
			Serializer<IPAddress>::FromBytes(begin,end)
		);
	
	}
	
	
	IPAddressRange::Mask::Mask (IPAddress base, Word bits) noexcept {
	
		//	Determine the number of bits in this
		//	IP address family
		Word count=(base.IsV6() ? sizeof(UInt128) : sizeof(UInt32))*BitsPerByte();
		
		//	Adjust the bit count to be a count
		//	of the number of low order bits that
		//	shall not be set
		bits=(bits>count) ? 0 : (count-bits);
		
		//	Create a mask
		if (base.IsV6()) {
		
			UInt128 mask=0;
			for (Word i=0;i<bits;++i) mask|=static_cast<UInt128>(1)<<i;
			
			mask=~mask;
			
			this->base=IPAddress(static_cast<UInt128>(base)&mask);
			this->mask=IPAddress(mask);
		
		} else {
		
			UInt32 mask=0;
			for (Word i=0;i<bits;++i) mask|=static_cast<UInt32>(1)<<i;
			
			mask=~mask;
			
			this->base=IPAddress(static_cast<UInt32>(base)&mask);
			this->mask=IPAddress(mask);
		
		}
	
	}
	
	
	IPAddressRange::Mask::Mask (IPAddress base, IPAddress mask) : mask(mask) {
	
		//	Make sure the base and the mask
		//	are in the same address family
		if (base.IsV6()!=mask.IsV6()) throw std::invalid_argument(not_same_addr_family);
		
		this->base=base.IsV6() ? IPAddress(
			static_cast<UInt128>(base)&static_cast<UInt128>(mask)
		) : IPAddress(
			static_cast<UInt32>(base)&static_cast<UInt32>(mask)
		);
	
	}
	
	
	bool IPAddressRange::Mask::Check (const IPAddress & other) const noexcept {
	
		if (other.IsV6()) {
		
			if (base.IsV6()) {
			
				auto mask=static_cast<UInt128>(this->mask);
				
				return (static_cast<UInt128>(other)&mask)==(static_cast<UInt128>(base)&mask);
			
			}
		
		} else if (!base.IsV6()) {
			
			auto mask=static_cast<UInt32>(this->mask);
			
			return (static_cast<UInt32>(other)&mask)==(static_cast<UInt32>(base)&mask);
		
		}
		
		return false;
	
	}
	
	
	template <typename T>
	Nullable<Word> count_bits (T num) noexcept {
	
		Nullable<Word> retr;
		
		Word bits=0;
		bool last=true;
		for (T mask=1;mask!=0;mask<<=1) {
		
			if ((num&mask)==0) {
			
				if (!last) return retr;
				
				++bits;
			
			} else {
			
				last=false;
			
			}
		
		}
		
		retr.Construct((sizeof(T)*BitsPerByte())-bits);
		return retr;
	
	}
	
	
	IPAddressRange::Mask::operator String () const {
	
		//	Start string off with base IP
		String str(base);
	
		//	Attempt to count the number of
		//	contiguous unset low order bits
		//	in the mask
		auto bits=base.IsV6() ? count_bits<UInt128>(static_cast<UInt128>(mask)) : count_bits<UInt32>(static_cast<UInt32>(mask));
		
		//	If bits is null, that means that
		//	the mask has non-contiguous bits,
		//	which means we can't represent it
		//	in CIDR format
		if (bits.IsNull()) str << " mask " << String(mask);
		//	Otherwise, we use CIDR format
		else str << "/" << String(*bits);
		
		return str;
	
	}
	
	
	void IPAddressRange::Mask::ToBytes (Vector<Byte> & buffer) const {
	
		Serializer<IPAddress>::ToBytes(buffer,base);
		Serializer<IPAddress>::ToBytes(buffer,mask);
	
	}
	
	
	bool IPAddressRange::Mask::operator == (const Mask & other) const noexcept {
	
		return (base==other.base) && (mask==other.mask);
	
	}
	
	
	bool IPAddressRange::Mask::operator < (const Mask & other) const noexcept {
	
		return (base==other.base) ? (mask<other.mask) : (base<other.base);
	
	}
	
	
	std::size_t IPAddressRange::Mask::Hash () const noexcept {
	
		std::size_t retr=23;
		
		std::hash<IPAddress> hasher;
		retr*=33;
		retr^=hasher(base);
		retr*=33;
		retr^=hasher(mask);
		
		return retr;
	
	}
	
	
	IPAddressRange IPAddressRange::CreateMask (IPAddress base, IPAddress mask) {
	
		IPAddressRange retr;
		
		retr.item=Mask(base,mask);
		
		return retr;
	
	}
	
	
	IPAddressRange::IPAddressRange (IPAddress ip) noexcept : item(ip) {	}
	
	
	IPAddressRange::IPAddressRange (IPAddress start, IPAddress end) : item(StartEnd(start,end)) {	}
	
	
	IPAddressRange::IPAddressRange (IPAddress base, Word bits) noexcept : item(Mask(base,bits)) {	}
	
	
	bool IPAddressRange::Check (const IPAddress & ip) const noexcept {
	
		if (!item.IsNull()) switch (item.Type()) {
		
			case 0:return item.Get<IPAddress>()==ip;
			case 1:return item.Get<Mask>().Check(ip);
			case 2:return item.Get<StartEnd>().Check(ip);
			default:break;
		
		}
		
		return false;
	
	}
	
	
	IPAddressRange::operator String () const {
	
		if (!item.IsNull()) switch (item.Type()) {
		
			case 0:return String(item.Get<IPAddress>());
			case 1:return String(item.Get<Mask>());
			case 2:return String(item.Get<StartEnd>());
			default:break;
		
		}
		
		return "EMPTY";
	
	}
	
	
	bool IPAddressRange::operator == (const IPAddressRange & other) const noexcept {
	
		if (item.IsNull()) return other.item.IsNull();
		
		if (item.Type()==other.item.Type()) switch (item.Type()) {
		
			case 0:return item.Get<IPAddress>()==other.item.Get<IPAddress>();
			case 1:return item.Get<Mask>()==other.item.Get<Mask>();
			case 2:return item.Get<StartEnd>()==other.item.Get<StartEnd>();
			default:break;
		
		}
		
		return false;
	
	}
	
	
	bool IPAddressRange::operator < (const IPAddressRange & other) const noexcept {
	
		if (item.IsNull()) return true;
		
		if (other.item.IsNull()) return false;
		
		if (item.Type()!=other.item.Type()) return item.Type()<other.item.Type();
		
		switch (item.Type()) {
		
			case 0:return item.Get<IPAddress>()<other.item.Get<IPAddress>();
			case 1:return item.Get<Mask>()<other.item.Get<Mask>();
			case 2:return item.Get<StartEnd>()<other.item.Get<StartEnd>();
			default:return true;
		
		}
	
	}
	
	
	IPAddressRange IPAddressRange::FromBytes (const Byte * & begin, const Byte * end) {
	
		IPAddressRange retr;
	
		//	Get the first byte, which represents what
		//	kind of range this is
		switch (static_cast<RangeType>(Serializer<Byte>::FromBytes(begin,end))) {
		
			case RangeType::None:
			default:break;
			case RangeType::IPAddress:
				retr.item=Serializer<IPAddress>::FromBytes(begin,end);
				break;
			case RangeType::StartEnd:
				retr.item=StartEnd::FromBytes(begin,end);
				break;
			case RangeType::Mask:
				retr.item=Mask::FromBytes(begin,end);
				break;
		
		}
		
		return retr;
	
	}
	
	
	void IPAddressRange::to_bytes (Vector<Byte> & buffer, RangeType type) {
	
		Serializer<Byte>::ToBytes(
			buffer,
			static_cast<Byte>(type)
		);
	
	}
	
	
	void IPAddressRange::ToBytes (Vector<Byte> & buffer) const {
	
		if (!item.IsNull()) switch (item.Type()) {
		
			case 0:
				to_bytes(buffer,RangeType::IPAddress);
				Serializer<IPAddress>::ToBytes(
					buffer,
					item.Get<IPAddress>()
				);
				return;
			case 1:
				to_bytes(buffer,RangeType::Mask);
				item.Get<Mask>().ToBytes(buffer);
				return;
			case 2:
				to_bytes(buffer,RangeType::StartEnd);
				item.Get<StartEnd>().ToBytes(buffer);
				return;
			default:break;
		
		}
		
		to_bytes(buffer,RangeType::None);
	
	}
	
	
	std::size_t IPAddressRange::Hash () const noexcept {
	
		//	Empty ranges hash to 0
		if (item.IsNull()) return 0;
		
		std::size_t hash;
		switch (item.Type()) {
		
			case 0:
				hash=std::hash<IPAddress>()(item.Get<IPAddress>());
				break;
			case 1:
				hash=item.Get<Mask>().Hash();
				break;
			case 2:
				hash=item.Get<StartEnd>().Hash();
				break;
			//	Unknown types hash to 0
			default:return 0;
		
		}
		
		std::size_t retr=23;
		
		retr*=33;
		retr^=static_cast<std::size_t>(item.Type());
		retr*=33;
		retr^=hash;
		
		return retr;
	
	}


}

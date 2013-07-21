#include <system_error>
#include <type_traits>
#include <limits>


void print_bytes (const Vector<Byte> & buffer) {

	bool first=true;
	for (const Byte b : buffer) {
	
		if (first) first=false;
		else StdOut << " ";
		
		StdOut << String(b,16);
	
	}
	
	StdOut << Newline;

}


bool test () {

	typedef PacketTypeMap<0x2C> pt;
	
	Vector<Tuple<UInt128,Double,SByte>> uuids;
	uuids.EmplaceBack(
		14,
		19.6,
		2
	);
	uuids.EmplaceBack(
		15,
		25.3,
		14
	);

	Packet packet;
	packet.SetType<pt>();
	packet.Retrieve<pt,0>().Payload.EmplaceBack(
		"Hello",
		114.3,
		std::move(uuids)
	);
	
	uuids.EmplaceBack(
		static_cast<UInt128>(std::numeric_limits<UInt64>::max())+1,
		163.0,
		-3
	);
	
	packet.Retrieve<pt,0>().Payload.EmplaceBack(
		"a\xCC\x81",
		43.0,
		std::move(uuids)
	);
		
	
	Vector<Byte> bytes(packet.ToBytes());
	
	print_bytes(bytes);
	
	Packet from_bytes;
	from_bytes.FromBytes(bytes);
	
	StdOut << from_bytes.ToString() << Newline;

	//return false;
	return true;

}

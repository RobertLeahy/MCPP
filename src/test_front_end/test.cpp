#include <curl.h>
#include <system_error>
#include <type_traits>


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

	return false;

	SHA1 hash;
	hash.Update(UTF8().Encode("hello"));
	hash.Update(UTF8().Encode(" world"));
	Vector<Byte> digest(hash.Complete());
	StdOut << digest.Count() << " bytes" << Newline;
	for (Byte b : digest) {
	
		String byte(b,16);
		
		if (byte.Count()==1) StdOut << "0";
		
		StdOut << byte;
		
	}
	StdOut << Newline;

	return true;

	Vector<IPAddress> mc_rleahy_ca(IPAddress::GetHostIPs("mc.rleahy.ca"));
	
	if (mc_rleahy_ca.Count()==0) {
	
		StdOut << "No IPs" << Newline;
		
		return true;
	
	}

	Socket s(Socket::Type::Stream,false);
	
	s.SetBlocking(true);
	
	s.Connect(
		mc_rleahy_ca[0],
		25563
	);
	
	Packet packet;
	packet.SetType<PacketTypeMap<0x02>>();
	
	packet.Retrieve<SByte>(0)=61;
	packet.Retrieve<String>(1)="Drainedsoul";
	packet.Retrieve<String>(2)="mc.rleahy.ca";
	packet.Retrieve<Int32>(3)=25563;
	
	Vector<Byte> buffer=packet.ToBytes();
	
	while (buffer.Count()!=0) s.Send(&buffer);
	
	//	PRINT	
	try {
	
		for (Word printed=0;;) {
		
			//if (buffer.Capacity()==buffer.Count()) buffer.SetCapacity();
		
			if (s.Receive(&buffer)==0) break;
			
			for (Byte b : buffer) {
			
				if (printed!=0) {
				
					if ((printed%8)==0) StdOut << Newline;
					else StdOut << " ";
				
				}
				
				StdOut << "0x";
				
				String byte(b,16);
				
				if (byte.Count()==1) StdOut << "0";
				
				StdOut << byte;
				
				++printed;
			
			}
			
			buffer.Clear();
		
		}
		
	} catch (...) {
	
		StdOut << Newline << "Done" << Newline;
	
	}

	//return false;
	return true;

}

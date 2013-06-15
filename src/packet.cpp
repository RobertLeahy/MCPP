#include <packet.hpp>


namespace MCPP {


	static const char * invalid_packet_type="Invalid packet type";
	static const char * no_type="Packet has not been imbued with a type";
	
	
	const Word ProtocolVersion=61;
	const Word MinecraftMajorVersion=1;
	const Word MinecraftMinorVersion=5;
	const Word MinecraftSubminorVersion=2;
	
	
	void Packet::destroy_buffer () noexcept {
	
		//	Cleanup buffer only if
		//	necessary
		if (buffer!=nullptr) {
		
			//	Loop over the metadata
			for (const auto & t : metadata) {
			
				if (
					//	If that item is constructed
					t.Item<1>() &&
					//	If there's an invocable target
					//	in the cleanup function
					t.Item<2>()
				) {
				
					//	Cleanup that object
					t.Item<2>()(buffer+t.Item<0>());
				
				}
			
			}
			
			//	Cleanup the memory
			Memory::Free(buffer);
			
			//	Prevent this from being cleaned
			//	up again
			buffer=nullptr;
		
		}
	
	}


	void Packet::destroy () noexcept {
	
		//	Cleanup the buffer
		destroy_buffer();
		
		//	If there's a factor, clean it up
		if (factory!=nullptr) delete factory;
	
	}


	Packet::Packet () noexcept : buffer(nullptr), metadata(0), curr(0), complete(false), factory(nullptr) {	}
	
	
	Packet::~Packet () noexcept {
	
		destroy();
	
	}
	
	
	Packet::Packet (Packet && other) noexcept : buffer(other.buffer), metadata(std::move(other.metadata)), curr(other.curr), complete(other.complete), factory(other.factory) {

		//	Prevent cleanup in
		//	the other object of
		//	pointers we took
		//	ownership of
		other.buffer=nullptr;
		other.factory=nullptr;
	
	}
	
	
	Packet & Packet::operator = (Packet && other) noexcept {
	
		//	Guard against self-assignment
		if (&other!=this) {
	
			//	Clean out the buffers and
			//	factory in this object
			destroy();
			
			//	Move from other item
			buffer=other.buffer;
			factory=other.factory;
			curr=other.curr;
			complete=other.complete;
			metadata=std::move(other.metadata);
			
			//	Prevent cleanup of
			//	pointers we took ownership
			//	of
			other.factory=nullptr;
			other.buffer=nullptr;
			
		}
		
		//	Return self reference
		return *this;
	
	}
	
	
	bool Packet::FromBytes (Vector<Byte> & buffer) {
	
		//	Do we need to create a new factory?
		if (
			(factory==nullptr) ||
			complete
		) {
		
			//	Yes
			
			//	Is there a byte in the stream
			//	to use to decide which factory
			//	to use?
			//
			//	If not abort
			if (buffer.Count()==0) return false;
			
			//	Attempt to acquire a factory
			PacketFactory * factory=PacketFactory::Make(buffer[0]);
			
			//	If that failed, there's a protocol
			//	error, throw
			if (factory==nullptr) throw std::runtime_error(invalid_packet_type);
			
			//	Install factory
			factory->Install(*this);
			
			//	Remove that first byte
			buffer.Delete(0);
		
		}
		
		//	Attempt to acquire bytes
		return factory->FromBytes(*this,buffer);
	
	}
	
	
	Byte Packet::Type () const {
	
		if (factory==nullptr) throw std::runtime_error(no_type);
		
		return factory->Type();
	
	}
	
	
	Word Packet::Size () const {
	
		if (factory==nullptr) throw std::runtime_error(no_type);
		
		return factory->Size(*this);
	
	}
	
	
	Vector<Byte> Packet::ToBytes () const {
	
		if (factory==nullptr) throw std::runtime_error(no_type);
		
		return factory->ToBytes(*this);
	
	}
	
	
	#include "protocol_analysis.cpp"
	
	
	String Packet::ToString () const {
	
		return protocol_analysis(*this);
	
	}
	
	
	Packet::operator String () const {
	
		return protocol_analysis(*this);
	
	}


}

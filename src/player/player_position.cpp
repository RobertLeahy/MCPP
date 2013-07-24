#include <player/player.hpp>


namespace MCPP {


	PlayerPosition::PlayerPosition (bool on_ground) noexcept : on_ground(on_ground) {
	
		if (!on_ground) on_ground_timer=Timer::CreateAndStart();
	
	}
	
	
	PlayerPosition::PlayerPosition (Double x, Double y, Double z, Double stance, bool on_ground) noexcept : PlayerPosition(on_ground) {
	
		X=x;
		Y=y;
		Z=z;
		Stance=stance;
	
	}
	
	
	PlayerPosition::PlayerPosition (
		Double x,
		Double y,
		Double z,
		Single yaw,
		Single pitch,
		Double stance,
		bool on_ground
	) noexcept : PlayerPosition(on_ground) {
	
		X=x;
		Y=y;
		Z=z;
		Yaw=yaw;
		Pitch=pitch;
		Stance=stance;
	
	}
	
	
	bool PlayerPosition::IsOnGround () const noexcept {
	
		return on_ground;
	
	}
	
	
	UInt64 PlayerPosition::SetOnGround (bool on_ground) {
	
		UInt64 time=0;
	
		if (on_ground!=this->on_ground) {
		
			if (on_ground) time=on_ground_timer.ElapsedNanoseconds();
			else on_ground_timer=Timer::CreateAndStart();
			
			this->on_ground=on_ground;
		
		}
		
		return time;
	
	}


	Packet PlayerPosition::ToPacket (Byte type) const {
	
		Packet packet;
	
		switch (type) {
		
			case 0x0A:{
			
				typedef PacketTypeMap<0x0A> pt;
				
				packet.SetType<pt>();
				
				packet.Retrieve<pt,0>()=on_ground;
			
			}break;
			
			case 0x0B:{
			
				typedef PacketTypeMap<0x0B> pt;
				
				packet.SetType<pt>();
				
				packet.Retrieve<pt,0>()=X;
				packet.Retrieve<pt,1>()=Y;
				packet.Retrieve<pt,2>()=Stance;
				packet.Retrieve<pt,3>()=Z;
				packet.Retrieve<pt,4>()=on_ground;
			
			}break;
			
			case 0x0C:{
			
				typedef PacketTypeMap<0x0C> pt;
				
				packet.SetType<pt>();
				
				packet.Retrieve<pt,0>()=Yaw;
				packet.Retrieve<pt,1>()=Pitch;
				packet.Retrieve<pt,2>()=on_ground;
			
			}break;
			
			case 0x0D:
			default:{
			
				typedef PacketTypeMap<0x0D> pt;
				
				packet.SetType<pt>();
				
				packet.Retrieve<pt,0>()=X;
				packet.Retrieve<pt,1>()=Y;
				packet.Retrieve<pt,2>()=Stance;
				packet.Retrieve<pt,3>()=Z;
				packet.Retrieve<pt,4>()=Yaw;
				packet.Retrieve<pt,5>()=Pitch;
				packet.Retrieve<pt,6>()=on_ground;
			
			}break;
		
		}
		
		return packet;
	
	}
	
	
	UInt64 PlayerPosition::FromPacket (const Packet & packet) {
	
		UInt64 time=0;
	
		switch (packet.Type()) {
		
			default:break;
			
			case 0x0A:{
			
				typedef PacketTypeMap<0x0A> pt;
			
				time=SetOnGround(packet.Retrieve<pt,0>());
			
			}break;
			
			case 0x0B:{
			
				typedef PacketTypeMap<0x0B> pt;
				
				X=packet.Retrieve<pt,0>();
				Y=packet.Retrieve<pt,1>();
				Stance=packet.Retrieve<pt,2>();
				Z=packet.Retrieve<pt,3>();
				time=SetOnGround(packet.Retrieve<pt,4>());
			
			}break;
			
			case 0x0C:{
			
				typedef PacketTypeMap<0x0C> pt;
				
				Yaw=packet.Retrieve<pt,0>();
				Pitch=packet.Retrieve<pt,1>();
				time=SetOnGround(packet.Retrieve<pt,2>());
			
			}break;
			
			case 0x0D:{
			
				typedef PacketTypeMap<0x0D> pt;
				
				X=packet.Retrieve<pt,0>();
				Y=packet.Retrieve<pt,1>();
				Stance=packet.Retrieve<pt,2>();
				Z=packet.Retrieve<pt,3>();
				Yaw=packet.Retrieve<pt,4>();
				Pitch=packet.Retrieve<pt,5>();
				time=SetOnGround(packet.Retrieve<pt,6>());
			
			}break;
		
		}
		
		return time;
	
	}


}

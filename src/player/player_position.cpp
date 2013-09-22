#include <player/player.hpp>
#include <fma.hpp>
#include <cmath>


namespace MCPP {


	PlayerPosition::PlayerPosition (bool on_ground) noexcept : on_ground(on_ground) {
	
		if (!on_ground) on_ground_timer=Timer::CreateAndStart();
		last_packet_timer=Timer::CreateAndStart();
	
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
	
	
	Tuple<Double,UInt64,UInt64> PlayerPosition::FromPacket (const Packet & packet) {
	
		UInt64 time=0;
		
		//	Time since last packet
		UInt64 last=last_packet_timer.ElapsedNanoseconds();
		last_packet_timer=Timer::CreateAndStart();
		
		//	Create local copies of x, y, and z
		//	coordinates so we can compute the
		//	distance the player moved
		Double x=X;
		Double y=Y;
		Double z=Z;
	
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
		
		//	Calculate distance traveled
		Double distance;
		//	If the before/after positions
		//	are identical, don't do expensive
		//	computations (sqrt is particularly
		//	expensive)
		if ((x==X) && (y==Y) && (z==Z)) {
		
			distance=0;
		
		} else {
		
			//	Deltas
			Double d_x=X-x;
			Double d_y=Y-y;
			Double d_z=Z-z;
			
			//	3D Pythagoras
			distance=sqrt(
				fma(
					d_x,
					d_x,
					fma(
						d_y,
						d_y,
						d_z*d_z
					)
				)
			);
		
		}
		
		return Tuple<Double,UInt64,UInt64>(
			distance,
			last,
			time
		);
	
	}


}

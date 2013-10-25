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


	Packets::Play::Clientbound::PlayerPositionAndLook PlayerPosition::ToPacket () const noexcept {
	
		Packets::Play::Clientbound::PlayerPositionAndLook retr;
		retr.X=X;
		retr.Y=Y;
		retr.Z=Z;
		retr.Yaw=Yaw;
		retr.Pitch=Pitch;
		retr.OnGround=on_ground;
		
		return retr;
	
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
		
		typedef Packets::Play::Serverbound::Player p;
		typedef Packets::Play::Serverbound::PlayerPosition pp;
		typedef Packets::Play::Serverbound::PlayerLook pl;
		typedef Packets::Play::Serverbound::PlayerPositionAndLook ppl;
		
		switch (packet.ID) {
		
			default:break;
			
			case p::PacketID:{
			
				auto & pa=packet.Get<p>();
				
				time=SetOnGround(pa.OnGround);
			
			}break;
			
			case pp::PacketID:{
			
				auto & pa=packet.Get<pp>();
				
				X=pa.X;
				Y=pa.X;
				Stance=pa.Stance;
				Z=pa.Z;
				time=SetOnGround(pa.OnGround);
			
			}break;
			
			case pl::PacketID:{
			
				auto & pa=packet.Get<pl>();
				
				Yaw=pa.Yaw;
				Pitch=pa.Pitch;
				time=SetOnGround(pa.OnGround);
			
			}break;
			
			case ppl::PacketID:{
			
				auto & pa=packet.Get<ppl>();
				
				X=pa.X;
				Y=pa.Y;
				Stance=pa.Stance;
				Z=pa.Z;
				Yaw=pa.Yaw;
				Pitch=pa.Pitch;
				time=SetOnGround(pa.OnGround);
			
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

/**
 *	\file
 */
 
 
#pragma once


#include <common.hpp>
#include <unordered_set>


namespace MCPP {


	/**
	 *	Encapsulates information about a player's
	 *	current position.
	 */
	class PlayerPosition {
	
	
		private:
		
		
			bool on_ground;
			Timer on_ground_timer;
	
	
		public:
		
		
			explicit PlayerPosition (bool on_ground) noexcept;
			PlayerPosition (Double x, Double y, Double z, Double stance, bool on_ground) noexcept;
			PlayerPosition (Single yaw, Single pitch, bool on_ground) noexcept;
			PlayerPosition (Double x, Double y, Double z, Single yaw, Single pitch, Double stance, bool on_ground) noexcept;
			PlayerPosition () = default;
		
		
			/**
			 *	The player's current X position.
			 */
			Double X;
			/**
			 *	The player's current Y position.
			 */
			Double Y;
			/**
			 *	The player's current Z position.
			 */
			Double Z;
			/**
			 *	The player's current absolute rotation
			 *	about the X axis.
			 */
			Single Yaw;
			/**
			 *	The player's current absolute rotation
			 *	about the Y axis.
			 */
			Single Pitch;
			/**
			 *	The client's current stance.
			 */
			Double Stance;
			
			
			/**
			 *	Determines whether the player is on
			 *	the ground or not.
			 *
			 *	\return
			 *		\em true if the player is on the
			 *		ground, \em false otherwise.
			 */
			bool IsOnGround () const noexcept;
			/**
			 *	Sets whether the player is on the ground
			 *	or not.
			 *
			 *	\param [in] on_ground
			 *		Whether the player is on the ground
			 *		or not.
			 *
			 *	\return
			 *		If \em on_ground caused the player to
			 *		transition from not being on the ground
			 *		to being on the ground, the number of
			 *		nanoseconds the player had been off the
			 *		ground.  Zero in all other instances.
			 */
			UInt64 SetOnGround (bool on_ground);
			
			
			/**
			 *	Gets a packet representing the information
			 *	in this object.
			 *
			 *	\param [in] type
			 *		The type of packet to retrieve.  May
			 *		be 0x0A, 0x0B, 0x0C, or 0x0D.  If
			 *		none of these values 0x0D is assumed.
			 *		Defaults to 0x0D.
			 */
			Packet ToPacket (Byte type=0x0D) const;
			/**
			 *	Updates this object with the values from
			 *	a packet.
			 *
			 *	If the packet is not 0x0A, 0x0B, 0x0C,
			 *	or 0x0D nothing happens.
			 *
			 *	\param [in] packet
			 *		The packet from which to populate
			 *		this object.
			 *
			 *	\return
			 *		If the player transitioned from being
			 *		in the air to on the ground, the number
			 *		of nanoseconds they spent in the air.
			 *		Zero otherwise.
			 */
			UInt64 FromPacket (const Packet & packet);
	
	
	};


	/**
	 *	Represents a player.
	 *
	 *	Players are distincts from clients.  A
	 *	client is a distinct connection whereas
	 *	a player represents a client logged in,
	 *	present in the game world.
	 */
	class Player {
	
	
		
	
	
	};


	/**
	 *	Provides management utilities for
	 *	players.
	 */
	class PlayerContainer : public Module {
	
	
		private:
		
		
			
		
		
		public:
		
		
			/**
			 *	\cond
			 */
			 
			 
			const String & Name () const noexcept override;
			Word Priority () const noexcept override;
			void Install () override;
			 
			 
			/**
			 *	\endcond
			 */
	
	
	};
	
	
	extern Nullable<PlayerContainer> Players;


}

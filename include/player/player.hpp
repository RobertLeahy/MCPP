/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <world/world.hpp>
#include <client.hpp>
#include <concurrency_manager.hpp>
#include <packet.hpp>
#include <packet_router.hpp>
#include <functional>
#include <unordered_set>
#include <unordered_map>


namespace MCPP {


	/**
	 *	Encapsulates information about a player's
	 *	current position.
	 */
	class PlayerPosition {
	
	
		private:
		
		
			bool on_ground;
			Timer on_ground_timer;
			Timer last_packet_timer;
	
	
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
			 *	\return
			 *		A packet representing the player's current
			 *		position and facing.
			 */
			Packets::Play::Clientbound::PlayerPositionAndLook ToPacket () const noexcept;
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
			 *		A tuple with three elements.  The first
			 *		element is the distance (in blocks) that
			 *		the player moved from their old position.
			 *		The second element is the number of nanoseconds
			 *		since the player's position was last updated.
			 *		The third element is the number of nanoseconds
			 *		the player spent in the air, if this packet
			 *		transitioned them from being in the air
			 *		to being on the ground, zero otherwise.
			 */
			Tuple<Double,UInt64,UInt64> FromPacket (const Packet & packet);
	
	
	};


	/**
	 *	Represents a player.
	 *
	 *	Players are distinct from clients.  A
	 *	client is a distinct connection whereas
	 *	a player represents a client logged in,
	 *	present in the game world.
	 */
	class Player {
	
	
		public:
		
		
			~Player () noexcept;
	
	
			Int32 ID;
		
		
			Mutex Lock;
			SmartPointer<Client> Conn;
			PlayerPosition Position;
			SByte Dimension;
			std::unordered_set<ColumnID> Columns;
	
	
	};


	/**
	 *	Provides management utilities for
	 *	players.
	 */
	class Players : public Module {
	
	
		private:
		
		
			//	SETTINGS
			
			
			//	Specifies the size of the square
			//	of columns which shall be sent to
			//	players
			Word view_distance;
			//	Specifies the size of the square of
			//	columns which shall be allowed to
			//	remain on the client
			Word cache_distance;
			
			
			//	Determines the spawn location
			Int32 spawn_x;
			Int32 spawn_y;
			Int32 spawn_z;
			SByte spawn_dimension;
			RWLock spawn_lock;
		
		
			//	Contains all player data
			std::unordered_map<
				SmartPointer<Client>,
				SmartPointer<Player>
			> players;
			RWLock players_lock;
			
			
			//	Manages column generation/sending
			//	tasks, insuring that they don't
			//	swamp the server's thread pool
			Nullable<ConcurrencyManager> cm;
			
			
			//	Contains the columns around the
			//	spawn area that we are interested
			//	in
			Vector<ColumnID> interested;
			Mutex interested_lock;
			CondVar interested_wait;
			bool interested_locked;
			Word interested_count;
			Timer interest_timer;
			
			
			//	Retrieves a player given a client
			//	handle.  Returns a null smart pointer
			//	if the client is not associated with
			//	a player object.
			SmartPointer<Player> get (const SmartPointer<Client> &) noexcept;
			//	Performs the necessary maintenance
			//	tasks (sending/removing columns)
			//	necessary after a player's position
			//	has changed
			void update_position (SmartPointer<Player> &, std::function<void ()> then=std::function<void ()>());
			
			
			//	EVENT HANDLERS
			void on_connect (SmartPointer<Client>);
			void on_disconnect (SmartPointer<Client>, const String &);
			void on_login (SmartPointer<Client>);
			void position_handler (ReceiveEvent);
			void set_spawn (std::function<void ()>);
			void set_spawn (Int32, Int32, Int32, bool);
			void get_spawn (std::function<void ()> then=std::function<void ()>());
			void express_interest (Word, std::function<void ()>);
		
		
		public:
		
		
			static Players & Get () noexcept;
			
			
			Players () noexcept;
		
		
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


}

/**
 *	\file
 */
 
 
#include <rleahylib/rleahylib.hpp>
#include <client.hpp>
#include <packet.hpp>
#include <functional>


namespace MCPP {


	/**
	 *	Encapsulates information about a packet
	 *	being received from a client.
	 */
	class ReceiveEvent {
	
	
		public:
		
		
			/**
			 *	The client from which the packet was
			 *	received.
			 */
			SmartPointer<Client> & From;
			/**
			 *	The packet which was received.
			 */
			Packet & Data;
	
	
	};


	/**
	 *	Provides the facilities for routing
	 *	incoming packets to handlers.
	 */
	class PacketRouter {
	
	
		public:
		
		
			/**
			 *	The type of callback which may subscribe
			 *	to receive events.
			 */
			typedef std::function<void (ReceiveEvent)> Type;
	
	
		private:
		
		
			//	Routes
			Type play_routes [PacketImpl::LargestID+1];
			Type status_routes [PacketImpl::LargestID+1];
			Type login_routes [PacketImpl::LargestID+1];
			Type handshake_routes [PacketImpl::LargestID+1];
			
			
			inline void destroy () noexcept;
			inline void init () noexcept;
			
			
		public:
		
		
			/**
			 *	Creates a new packet router with no
			 *	routes.
			 */
			PacketRouter () noexcept;
			
			
			/**
			 *	Cleans up a packet router.
			 */
			~PacketRouter () noexcept;
			
			
			/**
			 *	Fetches a packet route.
			 *
			 *	\param [in] id
			 *		The ID of the packet whole route
			 *		shall be retrieved.
			 *	\param [it] state
			 *		The state of the packet whose route
			 *		shall be retreived.
			 *
			 *	\return
			 *		A reference to the requested route.
			 */
			Type & operator () (UInt32 id, ProtocolState state) noexcept;
			/**
			 *	Fetches a packet route.
			 *
			 *	\param [in] id
			 *		The ID of the packet whole route
			 *		shall be retrieved.
			 *	\param [it] state
			 *		The state of the packet whose route
			 *		shall be retreived.
			 *
			 *	\return
			 *		A reference to the requested route.
			 */
			const Type & operator () (UInt32 id, ProtocolState state) const noexcept;
			
			
			/**
			 *	Dispatches a packet to the appropriate
			 *	handler.
			 *
			 *	\param [in] event
			 *		The event object which represents
			 *		this receive event.
			 *	\param [in] state
			 *		The state the protocol is in.
			 */
			void operator () (ReceiveEvent event, ProtocolState state) const;
			
			
			/**
			 *	Clears all handlers.
			 */
			void Clear () noexcept;
	
	
	};


}

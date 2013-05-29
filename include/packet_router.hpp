/**
 *	\file
 */
 
 
#include <rleahylib/rleahylib.hpp>
#include <packet.hpp>
#include <client.hpp>
#include <limits>
#include <functional>
#include <new>


namespace MCPP {


	typedef std::function<void (SmartPointer<Client> &&, Packet &&)> PacketHandler;


	/**
	 *	Provides the facilities for routing
	 *	incoming packets to handlers.
	 */
	class PacketRouter {
	
	
		private:
		
		
			//	An array of routes
			PacketHandler routes [std::numeric_limits<Byte>::max()];
			//	Whether packets destined for
			//	non-existent routes should
			//	cause the offending client to
			//	be kicked
			bool ignore_dne;
			
			
			inline void destroy () noexcept;
			inline void init () noexcept;
			
			
		public:
		
		
			/**
			 *	Creates a new packet router with no
			 *	routes.
			 *
			 *	\param [in] ignore_dne
			 *		\em true if the router should ignore
			 *		incoming packets with no handler,
			 *		\em false if the router should kill
			 *		offending clients.  Defaults to \em true.
			 */
			PacketRouter (bool ignore_dne=false) noexcept;
			
			
			/**
			 *	Cleans up a packet router.
			 */
			~PacketRouter () noexcept;
			
			
			/**
			 *	Fetches a packet route.
			 *
			 *	\param [in] offset
			 *		The packet type to retrieve the
			 *		route for.
			 *
			 *	\return
			 *		The currently installed route
			 *		for those types of packets.
			 */
			PacketHandler & operator [] (Byte type) noexcept;
			
			
			/**
			 *	Dispatches a packet to the appropriate
			 *	handler.
			 *
			 *	\param [in] client
			 *		The client the packet is from.
			 *	\param [in] packet
			 *		The packet-in-question.
			 */
			void operator () (SmartPointer<Client> client, Packet && packet) const;
			
			
			/**
			 *	Clears all handlers.
			 */
			void Clear () noexcept;
	
	
	};


}

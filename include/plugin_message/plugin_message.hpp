/**
 *	\file
 */
 
 
#pragma once
 

#include <rleahylib/rleahylib.hpp>
#include <client.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <packet_router.hpp>
#include <functional>
#include <unordered_map>
#include <unordered_set>


namespace MCPP {


	/**
	 *	Represents a plugin message, either
	 *	to be sent or received.
	 */
	class PluginMessage {
	
	
		public:
		
		
			/**
			 *	The client either that this plugin
			 *	message should be sent to, or that
			 *	this plugin message was received
			 *	from.
			 */
			SmartPointer<Client> Endpoint;
			/**
			 *	The channel on which this plugin
			 *	message was received, or on which
			 *	it should be sent.
			 */
			String Channel;
			/**
			 *	The plugin message to be sent, or
			 *	the plugin message which was received.
			 */
			Vector<Byte> Buffer;
	
	
	};


	/**
	 *	Handles sending and receiving 0xFA plugin
	 *	messages.
	 */
	class PluginMessages : public Module {
	
	
		private:
		
		
			//	Packet types
			typedef Packets::Play::Serverbound::PluginMessage incoming;
			typedef Packets::Play::Clientbound::PluginMessage outgoing;
		
		
			//	Maps channels to callbacks
			//	which handle incoming messages
			//	on that channel
			std::unordered_map<
				String,
				std::function<void (PluginMessage)>
			> callbacks;
			//	Maps clients to channels they've
			//	subscribed to
			std::unordered_map<
				SmartPointer<Client>,
				std::unordered_set<String>
			> clients;
			RWLock lock;
			
			
			void reg (SmartPointer<Client>, Vector<Byte>);
			void unreg (SmartPointer<Client>, Vector<Byte>);
			void handler (PacketEvent);
			
			
		public:
		
		
			/**
			 *	Retrieves a reference to a valid
			 *	instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance
			 *		of this class.
			 */
			static PluginMessages & Get () noexcept;
			
			
			/**
			 *	\cond
			 */
			
			
			virtual Word Priority () const noexcept override;
			virtual const String & Name () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
			
			
			/**
			 *	Adds a handler to a channel.
			 *
			 *	\param [in] channel
			 *		The channel the handler
			 *		shall be associated with.
			 *	\param [in] callback
			 *		The callback to be invoked
			 *		whenever a plugin message
			 *		on \em channel is received.
			 */
			void Add (String channel, std::function<void (PluginMessage)> callback);
			/**
			 *	Attempts to send a plugin message
			 *	on a certain channel to a certain
			 *	client.
			 *
			 *	If the client has not registered
			 *	for the channel-in-question, the
			 *	message will not be sent.
			 *
			 *	\param [in] message
			 *		The message to send.
			 *
			 *	\return
			 *		A send handle if the message
			 *		was sent, null otherwise.
			 */
			SmartPointer<SendHandle> Send (PluginMessage message);
	
	
	};


}

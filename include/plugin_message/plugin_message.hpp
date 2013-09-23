/**
 *	\file
 */
 
 
#pragma once
 

#include <rleahylib/rleahylib.hpp>
#include <client.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <functional>
#include <unordered_map>
#include <unordered_set>


namespace MCPP {


	/**
	 *	Handles sending and receiving 0xFA plugin
	 *	messages.
	 */
	class PluginMessages : public Module {
	
	
		private:
		
		
			//	Maps channels to callbacks
			//	which handle incoming messages
			//	on that channel
			std::unordered_map<
				String,
				std::function<void (SmartPointer<Client>, String, Vector<Byte>)>
			> callbacks;
			//	Maps clients to channels they've
			//	subscribe to
			std::unordered_map<
				SmartPointer<Client>,
				std::unordered_set<String>
			> clients;
			RWLock lock;
			
			
			inline bool handle (SmartPointer<Client> &, String &, Vector<Byte> &);
			inline void reg_channels (SmartPointer<Client> &, const String &);
			inline void unreg_channels (SmartPointer<Client> &, const String &);
			
			
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
			void Add (String channel, std::function<void (SmartPointer<Client>, String, Vector<Byte>)> callback);
			/**
			 *	Attempts to send a plugin message
			 *	on a certain channel to a certain
			 *	client.
			 *
			 *	If the client has not registered
			 *	for the channel-in-question, the
			 *	message will not be sent.
			 *
			 *	\param [in] client
			 *		The client to send to.
			 *	\param [in] channel
			 *		The channel to send on.
			 *	\param [in] buffer
			 *		The message to send.
			 *
			 *	\return
			 *		A send handle if the message
			 *		was sent, null otherwise.
			 */
			SmartPointer<SendHandle> Send (SmartPointer<Client> client, String channel, Vector<Byte> buffer);
	
	
	};


}

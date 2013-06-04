/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <connection_manager.hpp>
#include <unordered_map>
#include <utility>
#include <packet.hpp>
#include <aes_128_cfb_8.hpp>
#include <atomic>


namespace MCPP {


	/**
	 *	\cond
	 */
	 
	 
	class ClientList;
	
	
	/**
	 *	\endcond
	 */
	 
	
	/**
	 *	The states that a client connection
	 *	goes through.
	 */
	enum class ClientState : Word {
	
		Connected=0,		/**<	Client is newly-connected, and hasn't entered any state yet.	*/
		Authenticating=1,	/**<	Client is in the process of authenticating.	*/
		Authenticated=2		/**<	Client has logged in.	*/
	
	};


	/**
	 *	Contains information about a connected
	 *	client.
	 */
	class Client {
	
	
		friend class ClientList;
	
	
		private:
		
		
			//	Connection to client
			SmartPointer<Connection> conn;
			
			//	Encryption
			
			//	Encryption worker
			Nullable<AES128CFB8> encryptor;
			//	Whether encryption should
			//	be used or not
			volatile bool encrypted;
			
			//	Receive buffers
			
			//	Packet currently being built
			Packet in_progress;
			//	Decrypted bytes
			Vector<Byte> encryption_buffer;
			
			//	Client's current state
			std::atomic<Word> state;
			
			
		public:
		
			
			/**
			 *	Creates a new client that wraps a
			 *	given connection.
			 *
			 *	\param [in] conn
			 *		The connection to wrap.
			 */
			Client (SmartPointer<Connection> && conn) noexcept;
		
		
			/**
			 *	Enables encryption on this client's connection.
			 *
			 *	Once this call returns all further
			 *	communications with the given client shall
			 *	be transparently encrypted.
			 *
			 *	\param [in] key
			 *		The encryption key.
			 *	\param [in] iv
			 *		The initialization vector.
			 */
			void EnableEncryption (const Vector<Byte> & key, const Vector<Byte> & iv);
			
			
			/**
			 *	Sends data to the client.
			 *
			 *	\param [in] buffer
			 *		A buffer of bytes to send to the client.
			 *
			 *	\return
			 *		A send handle which can be used to wait
			 *		for the data to be flushed out to the
			 *		client.
			 */
			SmartPointer<SendHandle> Send (Vector<Byte> && buffer);
			/**
			 *	Receives data from a buffer.
			 *
			 *	Data shall be transparently decrypted as required.
			 *
			 *	Data shall be removed from \em buffer as
			 *	it is consumed.
			 *
			 *	\param [in,out] buffer
			 *		The buffer from which to extract bytes.
			 *
			 *	\return
			 *		\em true if a packet is ready for consumption,
			 *		\em false otherwise.
			 */
			bool Receive (Vector<Byte> * buffer);
			
			
			/**
			 *	Completes a receive, clearing the in progress
			 *	packet and retrieving it.
			 *
			 *	Calling this function except immediately after
			 *	Receive returns \em true results in undefined
			 *	behaviour.
			 *
			 *	\return
			 *		The completed packet.
			 */
			Packet CompleteReceive () noexcept;
			
			
			/**
			 *	Disconnects the client.
			 *
			 *	The client will not receive the appropriate
			 *	kick/disconnect packet unless the caller
			 *	explicitly sends it and waits on the send
			 *	handle returned by Send before calling
			 *	this function.
			 *
			 *	\param [in] reason
			 *		The reason the client is being disconnected,
			 *		defaults to the empty string.
			 */
			void Disconnect (const String & reason=String()) noexcept;
			/**
			 *	Disconnects the client.
			 *
			 *	The client will not receive the appropriate
			 *	kick/disconnect packet unless the caller
			 *	explicitly sends it and waits on the send
			 *	handle returned by Send before calling
			 *	this function.
			 *
			 *	\param [in] reason
			 *		The reason the client is being disconnected.
			 */
			void Disconnect (String && reason) noexcept;
			
			
			/**
			 *	Sets the client's state.
			 */
			void SetState (ClientState state) noexcept;
			/**
			 *	Retrieves the client's state.
			 *
			 *	\return
			 *		The client's current state.
			 */
			ClientState GetState () const noexcept;
	
	
	};
	
	
	/**
	 *	Thread safe container which maps
	 *	Connection objects to Client objects and
	 *	allows for threadsafe addition,
	 *	removal, and retrieval.
	 */
	class ClientList {
	
	
		private:
		
		
			std::unordered_map<const Connection *,SmartPointer<Client>> map;
			mutable RWLock map_lock;
			
			
		public:
		
		
			/**
			 *	Adds a client.
			 *
			 *	\param [in] client
			 *		The client to add.
			 */
			void Add (const SmartPointer<Client> & client);
		
		
			/**
			 *	Retrieves a client from their connection.
			 *
			 *	\param [in] conn
			 *		The connection of the client that should be
			 *		returned.
			 *
			 *	\return
			 *		The client whose connection is given
			 *		by \em conn.
			 */
			SmartPointer<Client> operator [] (const Connection & conn);
			
			
			/**
			 *	Removes a client.
			 *
			 *	\param [in] conn
			 *		The connection of the client that should be
			 *		removed.
			 */
			void Remove (const Connection & conn);
			
			
			/**
			 *	Retrieves the number of clients.
			 *
			 *	\return
			 *		Number of clients.
			 */
			Word Count () const noexcept;
			
			
			/**
			 *	Retrieves the number of authenticated
			 *	clients.
			 *
			 *	\return
			 *		Number of authenticated
			 *		clients.
			 */
			Word AuthenticatedCount () const noexcept;
			
			
			/**
			 *	Removes all clients.
			 */
			void Clear () noexcept;
			
	
	};


}

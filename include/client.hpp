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
		Authenticated=1		/**<	Client has logged in.	*/
	
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
			
			//	This lock must be held
			//	while performing IO so
			//	that the state of the
			//	encryptor may be changed
			//	in a threadsafe manner
			RWLock comm_lock;
			
			//	Encryption
			
			//	Encryption worker
			Nullable<AES128CFB8> encryptor;
			
			//	Receive buffer
			
			//	Packet currently being built
			Packet in_progress;
			//	Decrypted bytes
			Vector<Byte> encryption_buffer;
			//	If a packet has had at
			//	least one byte added to it
			//	this is set to true
			bool packet_in_progress;
			//	If packet_in_progress is
			//	true and the packet
			//	was started with encryption
			//	this is true
			bool packet_encrypted;
			
			//	Client's current state
			std::atomic<Word> state;
			
			//	Client's username
			String username;
			mutable Mutex username_lock;
			
			//	Client's inactivity
			//	duration
			mutable Timer inactive;
			mutable Mutex inactive_lock;
			
			
		public:
		
			
			/**
			 *	Creates a new client that wraps a
			 *	given connection.
			 *
			 *	\param [in] conn
			 *		The connection to wrap.
			 */
			Client (SmartPointer<Connection> conn) noexcept;
		
		
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
			SmartPointer<SendHandle> Send (const Packet & packet);
			/**
			 *	Sends data to the client and then
			 *	atomically enables encryption and sets
			 *	the client to authenticated state.
			 *
			 *	\param [in] buffer
			 *		A buffer of bytes to send to the client.
			 *	\param [in] key
			 *		The encryption key.
			 *	\param [in] iv
			 *		The initialization vector.
			 *	
			 *	\return
			 *		A send handle which can be used to wait
			 *		for the data to be flushed out to the
			 *		client.
			 */
			SmartPointer<SendHandle> Send (const Packet & packet, const Vector<Byte> & key, const Vector<Byte> & iv);
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
			 */
			void Disconnect () noexcept;
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
			void Disconnect (const String & reason) noexcept;
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
			
			
			/**
			 *	Sets the client's username.
			 *
			 *	Thread safe.
			 *
			 *	\param [in] username
			 *		The username to set.
			 */
			void SetUsername (String username) noexcept;
			/**
			 *	Retrieves the client's username.
			 *
			 *	Thread safe.
			 *
			 *	\return
			 *		The client's username.
			 */
			String GetUsername () const;
			
			
			/**
			 *	Retrieves the client's IP.
			 *
			 *	\return
			 *		The IP from which the client
			 *		is connected.
			 */
			const IPAddress & IP () const noexcept;
			/**
			 *	Retrieves the client's port.
			 *
			 *	\return
			 *		The port from which the client
			 *		is connected.
			 */
			UInt16 Port () const noexcept;
			
			
			/**
			 *	Retrieves a pointer to the wrapped
			 *	connection object.
			 */
			const Connection * GetConn () const noexcept;
			
			
			/**
			 *	Called when the client undergoes some
			 *	external activity not managed by the
			 *	built in network stack to reset
			 *	their inactivity timer.
			 */
			void Active ();
			/**
			 *	Determines the number of milliseconds
			 *	the client has been considered to be
			 *	inactive for.
			 *
			 *	\return
			 *		The number of milliseconds that have
			 *		elapsed since the client's last
			 *		activity.
			 */
			Word Inactive () const;
	
	
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
			void Add (SmartPointer<Client> client);
		
		
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
			
			
			/**
			 *	Scans the client list, iterating
			 *	over each connected client.
			 *
			 *	The collection is guaranteed not
			 *	to change from the time the scan
			 *	begins to the time it ends.
			 *
			 *	Do not try to modify the collection
			 *	while scanning, this will lead to
			 *	a deadlock.
			 *
			 *	\param [in] func
			 *		The callable object which is to be
			 *		called for each client.  It shall
			 *		be passed each client in turn.
			 */
			template <typename T>
			void Scan (T && func) {
			
				map_lock.Read([&] () mutable {	for (auto & pair : map) func(pair.second);	});
			
			}
			
	
	};


}

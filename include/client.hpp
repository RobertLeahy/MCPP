/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <network.hpp>
#include <unordered_map>
#include <utility>
#include <packet.hpp>
#include <aes_128_cfb_8.hpp>
#include <atomic>
#include <functional>
#include <type_traits>


namespace MCPP {


	/**
	 *	\cond
	 */
	
	
	template <typename T, typename... Args>
	class IsCallable {


		class dummy {	};


		template <typename T1, typename=void>
		class inner {
		
		
			public:
			
			
				static const bool Value=false;
		
		
		};
		
		
		template <typename T1>
		class inner<T1,typename std::enable_if<
			!std::is_same<
				decltype(
					std::declval<T1>()(
						std::declval<Args>()...
					)
				),
				dummy
			>::value
		>::type> {
		
		
			public:
			
			
				static const bool Value=true;
		
		
		};
		
		
		public:
		
		
			static const bool Value=inner<T>::Value;


	};
	 
	 
	/**
	 *	\endcond
	 */


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
	enum class ClientState {
	
		Connected,		/**<	Client is newly-connected, and hasn't entered any state yet.	*/
		Authenticated	/**<	Client has logged in.	*/
	
	};


	/**
	 *	Contains information about a connected
	 *	client.
	 */
	class Client {
	
	
		friend class ClientList;
	
	
		private:
		
		
			//	Connection to connection
			SmartPointer<Connection> conn;
			
			//	This lock must be held
			//	while performing IO so
			//	that the state of the
			//	encryptor may be changed
			//	in a threadsafe manner
			mutable RWLock comm_lock;
			//	The following allow comm_lock
			//	to be "recursively" acquired
			//	during send atomic callbacks
			mutable std::atomic<bool> comm_locked;
			mutable std::atomic<Word> comm_locked_id;
			
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
			ClientState state;
			
			//	Client's username
			String username;
			mutable Mutex username_lock;
			
			//	Client's inactivity
			//	duration
			mutable Timer inactive;
			mutable Mutex inactive_lock;
			
			//	Client's connected time
			mutable Timer connected;
			mutable Mutex connected_lock;
			
			
			void enable_encryption (const Vector<Byte> &, const Vector<Byte> &);
			SmartPointer<SendHandle> send (const Packet &);
			
			
			template <typename T>
			typename std::enable_if<
				std::is_same<
					decltype(std::declval<T>()()),
					void
				>::value
			>::type read (T && callback) const noexcept(noexcept(callback())) {
			
				bool locked;
				if (
					comm_locked &&
					(Thread::ID()==comm_locked_id)
				) {
				
					locked=false;
				
				} else {
				
					locked=true;
					
					comm_lock.Read();
				
				}
				
				try {
				
					callback();
					
				} catch (...) {
				
					if (locked) comm_lock.CompleteRead();
					
					throw;
				
				}
				
				if (locked) comm_lock.CompleteRead();
			
			}
			template <typename T>
			typename std::enable_if<
				!std::is_same<
					decltype(std::declval<T>()()),
					void
				>::value,
				decltype(std::declval<T>()())
			>::type read (T && callback) const noexcept(
				std::is_nothrow_move_constructible<decltype(callback())>::value &&
				noexcept(callback())
			) {
			
				bool locked;
				if (
					comm_locked &&
					(Thread::ID()==comm_locked_id)
				) {
				
					locked=false;
				
				} else {
				
					locked=true;
					
					comm_lock.Read();
				
				}
				
				Nullable<decltype(callback())> returnthis;
				
				try {
				
					returnthis.Construct(callback());
					
				} catch (...) {
				
					if (locked) comm_lock.CompleteRead();
					
					throw;
				
				}
				
				if (locked) comm_lock.CompleteRead();
				
				return *returnthis;
			
			}
			
			
			template <typename T>
			typename std::enable_if<
				std::is_same<
					decltype(std::declval<T>()()),
					void
				>::value
			>::type write (T && callback) const noexcept(noexcept(callback())) {
			
				bool locked;
				if (
					comm_locked &&
					(Thread::ID()==comm_locked_id)
				) {
				
					locked=false;
				
				} else {
				
					locked=true;
					
					comm_lock.Write();
				
				}
				
				try {
				
					callback();
				
				} catch (...) {
				
					if (locked) comm_lock.CompleteWrite();
					
					throw;
				
				}
				
				if (locked) comm_lock.CompleteWrite();
			
			}
			template <typename T>
			typename std::enable_if<
				!std::is_same<
					decltype(std::declval<T>()()),
					void
				>::value,
				decltype(std::declval<T>()())
			>::type write (T && callback) const noexcept(
				std::is_nothrow_move_constructible<decltype(callback())>::value &&
				noexcept(callback())
			) {
			
				bool locked;
				if (
					comm_locked &&
					(Thread::ID()==comm_locked_id)
				) {
				
					locked=false;
				
				} else {
				
					locked=true;
					
					comm_lock.Write();
				
				}
				
				Nullable<decltype(callback())> returnthis;
				
				try {
				
					returnthis.Construct(callback());
				
				} catch (...) {
				
					if (locked) comm_lock.CompleteWrite();
					
					throw;
				
				}
				
				if (locked) comm_lock.CompleteWrite();
				
				return *returnthis;
			
			}
			
			
		public:
		
		
			/**
			 *	The client's latency in milliseconds
			 *	as last reported.
			 */
			std::atomic<Word> Ping;
		
			
			/**
			 *	Creates a new client that wraps a
			 *	given connection.
			 *
			 *	\param [in] conn
			 *		The connection to wrap.
			 */
			Client (SmartPointer<Connection> conn);
		
		
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
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			SmartPointer<SendHandle> Send (const Packet & packet);
			/**
			 *	Sends data to the client and
			 *	atomically performs some action.
			 *
			 *	\tparam T
			 *		The type of a callback to be
			 *		executed atomically after
			 *		data is sent.
			 *	\tparam Args
			 *		The types of the arguments
			 *		which shall be passed to
			 *		the callback.
			 *
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *	\param [in] callback
			 *		The callback which will be executed
			 *		after data is sent.  Will be passed
			 *		a reference to this object, a handle-
			 *		which it may use to monitor the progress
			 *		of the asynchronous send operation,
			 *		as well as any other arguments passed
			 *		to this function.
			 *	\param [in] args
			 *		Arguments of types Args which shall
			 *		be forwarded through to \em callback.
			 *		
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			template <typename T, typename... Args>
			auto Send (const Packet & packet, T && callback, Args &&... args) -> typename std::enable_if<
				IsCallable<T,Client &,SmartPointer<SendHandle>,Args...>::Value,
				SmartPointer<SendHandle>
			>::type {
			
				SmartPointer<SendHandle> handle;
				
				bool locked;
				if (
					comm_locked &&
					(Thread::ID()==comm_locked_id)
				) {
				
					locked=false;
				
				} else {
				
					locked=true;
				
					comm_lock.Write();
					
					comm_locked=true;
					comm_locked_id=Thread::ID();
				
				}
				
				try {
				
					handle=send(packet);
					
					callback(
						*this,
						handle,
						std::forward<Args>(args)...
					);
				
				} catch (...) {
				
					if (locked) {
					
						comm_locked=false;
					
						comm_lock.CompleteWrite();
					
					}
					
					throw;
				
				}
				
				if (locked) {
				
					comm_locked=false;
					
					comm_lock.CompleteWrite();
				
				}
				
				return handle;
			
			}
			/**
			 *	Sends data to the client and atomically
			 *	updates the client's state.
			 *
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *	\param [in] state
			 *		The state to set the client's state
			 *		to.
			 *
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			SmartPointer<SendHandle> Send (const Packet & packet, ClientState state);
			/** 
			 *	Sends data to the client, atomically
			 *	updates the client's state, and performs
			 *	some action.
			 *
			 *	\tparam T
			 *		The type of a callback to be
			 *		executed atomically after
			 *		data is sent.
			 *	\tparam Args
			 *		The types of the arguments
			 *		which shall be passed to
			 *		the callback.
			 *
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *	\param [in] state
			 *		The state to set the client's state
			 *		to.
			 *	\param [in] callback
			 *		The callback which will be executed
			 *		after data is sent.  Will be passed
			 *		a reference to this object, a handle-
			 *		which it may use to monitor the progress
			 *		of the asynchronous send operation,
			 *		as well as any other arguments passed
			 *		to this function.
			 *	\param [in] args
			 *		Arguments of types Args which shall
			 *		be forwarded through to \em callback.
			 *		
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			template <typename T, typename... Args>
			auto Send (
				const Packet & packet,
				ClientState state,
				T && callback,
				Args &&... args
			) -> typename std::enable_if<
				IsCallable<T,Client &,SmartPointer<SendHandle>,Args...>::Value,
				SmartPointer<SendHandle>
			>::type {
			
				SmartPointer<SendHandle> handle;
			
				bool locked;
				if (
					comm_locked &&
					(Thread::ID()==comm_locked_id)
				) {
				
					locked=false;
				
				} else {
				
					locked=true;
				
					comm_lock.Write();
					
					comm_locked=true;
					comm_locked_id=Thread::ID();
				
				}
				
				try {
				
					this->state=state;
					
					handle=send(packet);
					
					callback(
						*this,
						handle,
						std::forward<Args>(args)...
					);
				
				} catch (...) {
				
					if (locked) {
					
						comm_locked=false;
					
						comm_lock.CompleteWrite();
					
					}
					
					throw;
				
				}
				
				if (locked) {
				
					comm_locked=false;
					
					comm_lock.CompleteWrite();
				
				}
				
				return handle;
			
			}
			/**
			 *	Sends data to the client and atomically
			 *	enables encryption.
			 *
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *	\param [in] key
			 *		The encryption key.
			 *	\param [in] iv
			 *		The initialization vector.
			 *	\param [in] before
			 *		\em true if \em packet should be
			 *		sent before encryption is enabled,
			 *		\em false otherwise.  Defaults to
			 *		\em true.
			 *
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			SmartPointer<SendHandle> Send (
				const Packet & packet,
				const Vector<Byte> & key,
				const Vector<Byte> & iv,
				bool before=true
			);
			/** 
			 *	Sends data to the client, atomically
			 *	enables encryption, and performs some
			 *	action.
			 *
			 *	\tparam T
			 *		The type of a callback to be
			 *		executed atomically after
			 *		data is sent.
			 *	\tparam Args
			 *		The types of the arguments
			 *		which shall be passed to
			 *		the callback.
			 *
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *	\param [in] key
			 *		The encryption key.
			 *	\param [in] iv
			 *		The initialization vector.
			 *	\param [in] before
			 *		\em true if \em packet should be
			 *		sent before encryption is enabled,
			 *		\em false otherwise.
			 *	\param [in] callback
			 *		The callback which will be executed
			 *		after data is sent.  Will be passed
			 *		a reference to this object, a handle-
			 *		which it may use to monitor the progress
			 *		of the asynchronous send operation,
			 *		as well as any other arguments passed
			 *		to this function.
			 *	\param [in] args
			 *		Arguments of types Args which shall
			 *		be forwarded through to \em callback.
			 *		
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			template <typename T, typename... Args>
			auto Send (
				const Packet & packet,
				const Vector<Byte> & key,
				const Vector<Byte> & iv,
				bool before,
				T && callback,
				Args &&... args
			) -> typename std::enable_if<
				IsCallable<T,Client &,SmartPointer<SendHandle>,Args...>::Value,
				SmartPointer<SendHandle>
			>::type {
			
				SmartPointer<SendHandle> handle;
			
				bool locked;
				if (
					comm_locked &&
					(Thread::ID()==comm_locked_id)
				) {
				
					locked=false;
				
				} else {
				
					locked=true;
				
					comm_lock.Write();
					
					comm_locked=true;
					comm_locked_id=Thread::ID();
				
				}
				
				try {
				
					if (before) handle=send(packet);
					
					if (encryptor.IsNull()) enable_encryption(key,iv);
					
					if (!before) handle=send(packet);
					
					callback(
						*this,
						handle,
						std::forward<Args>(args)...
					);
				
				} catch (...) {
				
					if (locked) {
					
						comm_locked=false;
					
						comm_lock.CompleteWrite();
					
					}
					
					throw;
				
				}
				
				if (locked) {
				
					comm_locked=false;
					
					comm_lock.CompleteWrite();
				
				}
				
				return handle;
			
			}
			/**
			 *	Sends data to the client and atomically
			 *	enables encryption and sets the client's
			 *	state.
			 *
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *	\param [in] key
			 *		The encryption key.
			 *	\param [in] iv
			 *		The initialization vector.
			 *	\param [in] state
			 *		The state to set the client's state
			 *		to.
			 *	\param [in] before
			 *		\em true if \em packet should be
			 *		sent before encryption is enabled,
			 *		\em false otherwise.  Defaults to
			 *		\em true.
			 *
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			SmartPointer<SendHandle> Send (
				const Packet & packet,
				const Vector<Byte> & key,
				const Vector<Byte> & iv,
				ClientState state,
				bool before=true
			);
			/**
			 *	Sends data to the client, atomically
			 *	enables encryption, sets the client's
			 *	state, and performs some action.
			 *
			 *	\tparam T
			 *		The type of a callback to be
			 *		executed atomically after
			 *		data is sent.
			 *	\tparam Args
			 *		The types of the arguments
			 *		which shall be passed to
			 *		the callback.
			 *
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *	\param [in] key
			 *		The encryption key.
			 *	\param [in] iv
			 *		The initialization vector.
			 *	\param [in] before
			 *		\em true if \em packet should be
			 *		sent before encryption is enabled,
			 *		\em false otherwise.
			 *	\param [in] state
			 *		The state to set the client's state
			 *		to.
			 *	\param [in] callback
			 *		The callback which will be executed
			 *		after data is sent.  Will be passed
			 *		a reference to this object, a handle-
			 *		which it may use to monitor the progress
			 *		of the asynchronous send operation,
			 *		as well as any other arguments passed
			 *		to this function.
			 *	\param [in] args
			 *		Arguments of types Args which shall
			 *		be forwarded through to \em callback.
			 *		
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			template <typename T, typename... Args>
			auto Send (
				const Packet & packet,
				const Vector<Byte> & key,
				const Vector<Byte> & iv,
				bool before,
				ClientState state,
				T && callback,
				Args &&... args
			) -> typename std::enable_if<
				IsCallable<T,Client &,SmartPointer<SendHandle>,Args...>::Value,
				SmartPointer<SendHandle>
			>::type {
			
				SmartPointer<SendHandle> handle;
				
				bool locked;
				if (
					comm_locked &&
					(Thread::ID()==comm_locked_id)
				) {
				
					locked=false;
				
				} else {
				
					locked=true;
				
					comm_lock.Write();
					
					comm_locked=true;
					comm_locked_id=Thread::ID();
				
				}
				
				try {
				
					this->state=state;
					
					if (before) handle=send(packet);
					
					if (encryptor.IsNull()) enable_encryption(key,iv);
					
					if (!before) handle=send(packet);
					
					callback(
						*this,
						handle,
						std::forward<Args>(args)...
					);
				
				} catch (...) {
				
					if (locked) {
					
						comm_locked=false;
					
						comm_lock.CompleteWrite();
					
					}
					
					throw;
				
				}
				
				if (locked) {
				
					comm_locked=false;
					
					comm_lock.CompleteWrite();
				
				}
				
				return handle;
			
			}
			
			
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
			IPAddress IP () const noexcept;
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
			
			
			/**
			 *	Retrieves the number of bytes of
			 *	cleartext in the object's internal
			 *	buffer.
			 *
			 *	Not thread safe.
			 *
			 *	\return
			 *		The number of bytes in the buffer.
			 */
			Word Count () const noexcept;
			
			
			/**
			 *	Determines how long the client has been
			 *	connected.
			 *
			 *	\return
			 *		The number of milliseconds the client
			 *		has been connected.
			 */
			Word Connected () const;
			
			
			/**
			 *	Retrieves the number of bytes received on
			 *	this connection.
			 *
			 *	\return
			 *		The number of bytes received on this
			 *		connection.
			 */
			UInt64 Received () const noexcept;
			/**
			 *	Retrieves the number of bytes sent on this
			 *	connection.
			 *
			 *	\return
			 *		The number of bytes sent on this connection.
			 */
			UInt64 Sent () const noexcept;
			/**
			 *	Retrieves the number of send operations to this
			 *	client which are pending.
			 *
			 *	\return
			 *		The number of send operations waiting to
			 *		complete to this client.
			 */
			Word Pending () const noexcept;
			
	
	
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

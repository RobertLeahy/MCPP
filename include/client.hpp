/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <aes_128_cfb_8.hpp>
#include <network.hpp>
#include <packet.hpp>
#include <recursive_mutex.hpp>
#include <scope_guard.hpp>
#include <atomic>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <utility>


namespace MCPP {


	/**
	 *	\cond
	 */
	 
	 
	class ClientList;
	
	
	/**
	 *	\endcond
	 */


	/**
	 *	Contains information about a connected
	 *	client.
	 */
	class Client {
	
	
		friend class ClientList;
		
		
		public:
		
		
			typedef Vector<Promise<bool>> AtomicType;
	
	
		private:
		
		
			//	Connection to connection
			SmartPointer<Connection> conn;
			
			
			//	Guards state sends, and encryptor
			mutable RecursiveMutex lock;
			
			//	Encryption
			
			//	Encryption worker
			Nullable<AES128CFB8> encryptor;
			
			//	Receive buffer
			
			//	Packet currently being built
			PacketParser parser;
			//	Decrypted bytes
			Vector<Byte> encryption_buffer;
			
			//	Client's current state
			ProtocolState state;
			
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
			void log (const Packet &, ProtocolState, ProtocolDirection, const Vector<Byte> &, const Vector<Byte> &) const;
			
			
			template <typename T>
			Promise<bool> send (const T & packet) {
			
				auto buffer=Serialize(packet);
				Vector<Byte> ciphertext;
				
				if (encryptor.IsNull()) {
				
					log(
						packet,
						T::State,
						T::Direction,
						buffer,
						ciphertext
					);
					
					return conn->Send(std::move(buffer));
				
				}
				
				ciphertext=encryptor->Encrypt(buffer);
				
				log(
					packet,
					T::State,
					T::Direction,
					buffer,
					ciphertext
				);
				
				return conn->Send(std::move(ciphertext));
			
			}
			
			
			template <typename T>
			typename std::enable_if<
				std::is_base_of<Packet,typename std::decay<T>::type>::value
			>::type atomic_perform (AtomicType & sends, T && packet) {
			
				sends.Add(send(std::forward<T>(packet)));
			
			}
			
			
			template <typename T>
			typename std::enable_if<
				std::is_convertible<typename std::decay<T>::type,std::function<void ()>>::value
			>::type atomic_perform (const AtomicType &, T && callback) noexcept(noexcept(callback())) {
			
				callback();
			
			}
			
			
			void atomic_perform (const AtomicType &, const Tuple<Vector<Byte>,Vector<Byte>> &);
			
			
			void atomic_perform (const AtomicType &, ProtocolState) noexcept;
			
			
			void atomic_perform (AtomicType &, Vector<Byte>);
			
			
			static void atomic (const AtomicType &) noexcept {	}
			
			
			template <typename T, typename... Args>
			void atomic (AtomicType & sends, T && arg, Args &&... args) {
			
				atomic_perform(sends,std::forward<T>(arg));
				
				atomic(sends,std::forward<Args>(args)...);
			
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
			 *	Performs an arbitrary number of actions,
			 *	atomically, in an arbitrary order.
			 *
			 *	Each parameter to this variadic member
			 *	function template gives one action, actions
			 *	shall be performed in the order they are passed,
			 *	from left to right.
			 *
			 *	Passing in a ProtocolState shall change the
			 *	client's state to that state.
			 *
			 *	Passing in any object which is a Packet will
			 *	send that packet to this client.
			 *
			 *	Passing in a Vector<Byte> object will send that
			 *	raw buffer of bytes to the client.
			 *
			 *	Passing in a tuple of two Vector<Byte> objects
			 *	will enable encryption with the first item
			 *	in the tuple as the secret key and the second
			 *	as the initializaton vector.
			 *
			 *	Passing in any object which is convertible to
			 *	std::function<void ()> will invoke that callback.
			 *
			 *	\tparam Args
			 *		The types of arguments to this variadic
			 *		member function template.
			 *
			 *	\param [in] args
			 *		The arguments to this variadic member
			 *		function template.
			 *
			 *	\return
			 *		A collection of send results, one for each
			 *		packet that was sent, with the send results
			 *		corresponding to the sent packets in order
			 *		from left to right.
			 */
			template <typename... Args>
			AtomicType Atomic (Args &&... args) {
			
				lock.Acquire();
				auto guard=AtExit([&] () {	lock.Release();	});
				
				AtomicType retr;
				atomic(retr,std::forward<Args>(args)...);
				
				return retr;
			
			}
		
		
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
			 *		A buffer of bytes to send to the
			 *		client.
			 *
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			Promise<bool> Send (Vector<Byte> buffer);
			/**
			 *	Sends data to the client.
			 *
			 *	\tparam T
			 *		The type of packet which shall be
			 *		sent.
			 *
			 *	\param [in] packet
			 *		The packet to send to the client.
			 *
			 *	\return
			 *		A send handle which can be used to
			 *		monitor the progress of the asynchronous
			 *		send operation.
			 */
			template <typename T>
			Promise<bool> Send (const T & packet) {
			
				return lock.Execute([&] () {	return send(packet);	});
			
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
			bool Receive (Vector<Byte> & buffer);
			/**
			 *	Gets a received packet.
			 *
			 *	\return
			 *		A reference to the last packet
			 *		received.
			 */
			Packet & GetPacket () noexcept;
			
			
			/**
			 *	Disconnects the client.
			 *
			 *	The client will not receive the appropriate
			 *	kick/disconnect packet unless the caller
			 *	explicitly sends it and waits on the send
			 *	handle returned by Send before calling
			 *	this function.
			 */
			void Disconnect ();
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
			void Disconnect (const String & reason);
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
			void Disconnect (String && reason);
			
			
			/**
			 *	Sets the client's state.
			 */
			void SetState (ProtocolState state) noexcept;
			/**
			 *	Retrieves the client's state.
			 *
			 *	\return
			 *		The client's current state.
			 */
			ProtocolState GetState () const noexcept;
			
			
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
			
	
	
	};
	
	
	/**
	 *	\cond
	 */
	 
	 
	class ClientList;
	
	
	class ClientListIterator {
	
	
		friend class ClientList;
		
		
		private:
		
		
			typedef std::unordered_map<
				const Connection *,
				SmartPointer<Client>
			>::iterator iter_type;
			
			
			iter_type iter;
			ClientList * list;
			
			
			ClientListIterator (ClientList *, iter_type) noexcept;
			
			
		public:
		
		
			ClientListIterator () = delete;
			~ClientListIterator () noexcept;
			ClientListIterator (const ClientListIterator &) noexcept;
			ClientListIterator & operator = (const ClientListIterator &) noexcept;
			
			
			SmartPointer<Client> & operator * () noexcept;
			SmartPointer<Client> * operator -> () noexcept;
			ClientListIterator & operator ++ () noexcept;
			ClientListIterator operator ++ (int) noexcept;
			
			
			bool operator == (const ClientListIterator &) const noexcept;
			bool operator != (const ClientListIterator &) const noexcept;
	
	
	};
	 
	 
	/**
	 *	\endcond
	 */
	 
	 
	/**
	 *	The type of search that may be
	 *	performed for a client based on
	 *	their username.
	 */
	enum class ClientSearch {
	
		Exact,	/**<	Clients whose usernames match the exact string (ignoring case) provided will be returned.	*/
		Begin,	/**<	Clients whose usernames begin with the string provided will be returned.	*/
		End,	/**<	Clients whose usernames end with the exact string provided will be returned.	*/
		Match	/**<	Clients whose usernames contain the substring provided will be returned.	*/
	
	};
	
	
	/**
	 *	Thread safe container which maps
	 *	Connection objects to Client objects and
	 *	allows for threadsafe addition,
	 *	removal, and retrieval.
	 */
	class ClientList {
	
	
		friend class ClientListIterator;
	
	
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
			
			
			ClientListIterator begin () noexcept;
			ClientListIterator end () noexcept;
			
			
			/**
			 *	Retrieves a list of clients whose
			 *	usernames match a given string.
			 *
			 *	\param [in] str
			 *		The string to match.
			 *	\param [in] type
			 *		The type of search to perform.
			 *
			 *	\return
			 *		A collection of all clients
			 *		whose usernames matched
			 *		\em str based on the type of
			 *		match given by \em type.
			 */
			Vector<SmartPointer<Client>> Get (const String & str, ClientSearch type=ClientSearch::Exact) const;
			
	
	};


}

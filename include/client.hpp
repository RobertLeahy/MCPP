/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <aes_128_cfb_8.hpp>
#include <network.hpp>
#include <packet.hpp>
#include <atomic>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include <utility>


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
	
	
	namespace ClientImpl {
	
	
		template <typename...>
		class ContainsPacketImpl {	};
		
		
		template <typename T, typename... Args_i>
		class ContainsPacketImpl<T,Args_i...> {
		
		
			public:
			
			
				constexpr static Word Value=ContainsPacketImpl<Args_i...>::Value+(
					std::is_base_of<
						Packet,
						typename std::decay<T>::type
					>::value
						?	1
						:	0
				);
		
		
		};
		
		
		template <>
		class ContainsPacketImpl<> {
		
		
			public:
			
			
				constexpr static Word Value=0;
		
		
		};
	
	
		template <typename... Args>
		class ContainsPacket {

		
			public:
			
			
				constexpr static Word Value=ContainsPacketImpl<Args...>::Value;
		
		
		};
		
		
	}
	 
	 
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
			SmartPointer<SendHandle> send (const T & packet) {
			
				auto buffer=Serialize(packet);
				Vector<Byte> ciphertext;
				
				SmartPointer<SendHandle> retr;
				
				if (encryptor.IsNull()) {
				
					log(
						packet,
						T::State,
						T::Direction,
						buffer,
						ciphertext
					);
					
					retr=conn->Send(std::move(buffer));
				
				} else {
				
					encryptor->BeginEncrypt();
					
					try {
					
						ciphertext=encryptor->Encrypt(buffer);
						
						log(
							packet,
							T::State,
							T::Direction,
							buffer,
							ciphertext
						);
						
						retr=conn->Send(std::move(ciphertext));
					
					} catch (...) {
					
						encryptor->EndEncrypt();
						
						throw;
					
					}
					
					encryptor->EndEncrypt();
				
				}
				
				return retr;
				
			}
			
			
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
			
			
			template <typename T>
			inline typename std::enable_if<
				std::is_base_of<Packet,T>::value,
				SmartPointer<SendHandle>
			>::type atomic_perform (const T & packet) {
			
				return send(packet);
			
			}
			
			
			inline SmartPointer<SendHandle> atomic_perform (const Tuple<Vector<Byte>,Vector<Byte>> & t) {
			
				if (encryptor.IsNull()) enable_encryption(
					t.Item<0>(),
					t.Item<1>()
				);
				
				return SmartPointer<SendHandle>();
			
			}
			
			
			template <typename T>
			static typename std::enable_if<
				std::is_convertible<T,std::function<void ()>>::value,
				SmartPointer<SendHandle>
			>::type atomic_perform (T && callback) noexcept(noexcept(callback())) {
			
				callback();
				
				return SmartPointer<SendHandle>();
			
			}
			
			
			inline SmartPointer<SendHandle> atomic_perform (ProtocolState state) noexcept {
			
				this->state=state;
				
				return SmartPointer<SendHandle>();
			
			}
			
			
			//	Recursion terminators
			static inline void atomic_helper () noexcept {	}
			static inline void atomic_helper (const SmartPointer<SendHandle> &) noexcept {	}
			static inline void atomic_helper (const Vector<SmartPointer<SendHandle>> &) noexcept {	}
			
			
			template <typename T, typename... Args>
			typename std::enable_if<
				!(
					std::is_same<
						typename std::decay<T>::type,
						SmartPointer<SendHandle>
					>::value ||
					std::is_same<
						typename std::decay<T>::type,
						Vector<SmartPointer<SendHandle>>
					>::value
				)
			>::type atomic_helper (T && t, Args &&... args) {
			
				atomic_perform(std::forward<T>(t));
				
				atomic_helper(std::forward<Args>(args)...);
			
			}
			
			
			template <typename T, typename... Args>
			void atomic_helper (SmartPointer<SendHandle> & handle, T && t, Args &&... args) {
			
				auto h=atomic_perform(std::forward<T>(t));
				
				if (!h.IsNull()) handle=std::move(h);
				
				atomic_helper(
					handle,
					std::forward<Args>(args)...
				);
			
			}
			
			
			template <typename T, typename... Args>
			void atomic_helper (Vector<SmartPointer<SendHandle>> & vec, T && t, Args &&... args) {
			
				auto h=atomic_perform(std::forward<T>(t));
				
				if (!h.IsNull()) vec.Add(std::move(h));
				
				atomic_helper(
					vec,
					std::forward<Args>(args)...
				);
			
			}
			
			
			template <typename... Args>
			static typename std::enable_if<
				ClientImpl::ContainsPacket<Args...>::Value==1,
				SmartPointer<SendHandle>
			>::type get_atomic () noexcept {
			
				return SmartPointer<SendHandle>();
			
			}
			
			
			template <typename... Args>
			static typename std::enable_if<
				ClientImpl::ContainsPacket<Args...>::Value!=1,
				Vector<SmartPointer<SendHandle>>
			>::type get_atomic () noexcept {
			
				return Vector<SmartPointer<SendHandle>>();
			
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
			 *
			 */
			template <typename... Args>
			typename std::enable_if<
				ClientImpl::ContainsPacket<Args...>::Value==0
			>::type Atomic (Args &&... args) {
			
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
				
					atomic_helper(std::forward<Args>(args)...);
				
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
			
			}
			/**
			 *
			 */
			template <typename... Args>
			typename std::enable_if<
				ClientImpl::ContainsPacket<Args...>::Value!=0,
				decltype(get_atomic<Args...>())
			>::type Atomic (Args &&... args) {
			
				auto retr=get_atomic<Args...>();
			
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
				
					atomic_helper(
						retr,
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
			SmartPointer<SendHandle> Send (Vector<Byte> buffer);
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
			SmartPointer<SendHandle> Send (const T & packet) {
			
				return read([&] () {	return send(packet);	});
			
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

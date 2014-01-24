/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <promise.hpp>
#include <socketpair.hpp>
#include <thread_pool.hpp>
#include <atomic>
#include <cstddef>
#include <memory>
#include <system_error>
#include <unordered_map>
#include <utility>
#ifdef linux
#include <sys/epoll.h>
#else

#endif


namespace MCPP {


	namespace NetworkImpl {
	
	
		//	The type of file descriptors on
		//	this platform
		typedef int FDType;
		//	The type of error codes on this
		//	platform
		typedef int ErrorType;
	
	
		//	Gets underlying OS errors in a way
		//	that is C++-friendly
		std::exception_ptr GetException () noexcept;
		[[noreturn]]
		void Raise ();
		//	Drop-in replacement for POSIX strerror_r
		//	since glibc decided to mess around with
		//	everything and make feature test macros
		//	REQUIRED to get POSIX functionality
		//
		//	Sometimes I think that despite all their
		//	preaching etc., the FOSS community is really
		//	no better than Microsoft, except instead
		//	of getting paid to throw a wrench into
		//	standardization efforts, they do it for
		//	free.
		//
		//	Not sure if that's better or worse...
		int strerror_r (int, char *, std::size_t);
		String GetErrorMessage ();
		bool WouldBlock () noexcept;
		bool WasInterrupted () noexcept;
		ErrorType GetSocketError (FDType);
		void SetError (ErrorType) noexcept;
		
		
		//	Miscellaneous socket operations
		void SetBlocking (FDType, bool);
		FDType GetSocket (bool);
		Endpoint GetEndpoint (const struct sockaddr_storage *) noexcept;
		
		
		//	When "activated" captures the
		//	current system error.  Cannot
		//	be activated multiple times
		class Error {
		
		
			public:
			
			
				mutable Mutex lock;
				bool set;
				std::exception_ptr Exception;
				Nullable<String> Message;
				
				
				Error () noexcept;
				
				
				operator bool () const noexcept;
				void Capture ();
				void Set (String) noexcept;
		
		
		};
		
		
		class Updater {
		
		
			public:
			
			
				virtual void Update (FDType) = 0;
		
		
		};
		
		
		class ChannelBase;
		
		
		class Channel {
		
		
			public:
			
			
				FDType FD;
				SmartPointer<ChannelBase> Impl;
		
		
		};
		
		
		//	An action which the worker should take
		//	after performing an action on a channel
		class FollowUp {
		
		
			public:
			
			
				typedef std::function<FollowUp (SmartPointer<ChannelBase>)> Type;
			
			
				//	Callbacks that the worker shall execute
				//	when this returns, and which may themselves
				//	return further follow ups
				Vector<Type> Action;
				//	Whether the channel that was invoked should
				//	be removed from the worker's consideration
				bool Remove;
				//	How many bytes the channel sent
				Word Sent;
				//	How many bytes the channel received
				Word Received;
				//	How many incoming connections the channel
				//	formed
				Word Incoming;
				//	How many outgoing connections the channel
				//	formed
				Word Outgoing;
				//	How many incoming connections the channel
				//	accepted
				Word Accepted;
				//	How many connections the channel closed
				Word Disconnected;
				//	A connection that should be added to the
				//	worker
				Channel Add;
				
				
				FollowUp () noexcept;
		
		
		};
		
		
		//	A single notification about a file descriptor,
		//	which specifies that it has an error, is readable,
		//	or is writeable
		class Notification {
		
		
			private:
			
			
				struct epoll_event event;
				
				
				bool impl (uint32_t) const noexcept;
				
				
			public:
			
			
				FDType FD () const noexcept;
				bool Readable () const noexcept;
				bool Writeable () const noexcept;
				bool Error () const noexcept;
				bool End () const noexcept;
		
		
		};
		
		
		//	Make sure Notification is the
		//	correct size
		static_assert(
			sizeof(
			#ifdef linux
			struct epoll_event
			#else
			
			#endif
			)==sizeof(Notification),
			"NetworkImpl::Notification layout incorrect"
		);
		
		
		//	A notifier which aggregates notifications
		//	from various file descriptors
		class Notifier {
		
		
			private:
			
			
				FDType handle;
				
				
				Word wait (void *, Word);
				
				
			public:
			
			
				Notifier ();
				~Notifier () noexcept;
				Notifier (const Notifier &) = delete;
				Notifier (Notifier &&) = delete;
				Notifier & operator = (const Notifier &) = delete;
				Notifier & operator = (Notifier &&) = delete;
				
				
				void Attach (FDType);
				void Update (FDType, bool read, bool write);
				void Detach (FDType);
				
				
				template <Word n>
				Word Wait (Notification (& arr) [n]) {
				
					return wait(arr,n);
				
				}
				Word Wait (Notification &);
		
		
		};
		
		
		class ChannelBase {
		
		
			public:
			
			
				virtual ~ChannelBase () noexcept;
				virtual FollowUp Perform (const Notification &) = 0;
				virtual void SetUpdater (Updater *) = 0;
				//	Returns true if the channel should be
				//	maintained, false otherwise
				virtual bool Update (Notifier &) = 0;
		
		
		};
	
	
	}
	
	
	class ListeningSocket : public NetworkImpl::ChannelBase {
	
		
		private:
		
		
			NetworkImpl::FDType socket;
			
			
			mutable Mutex lock;
			NetworkImpl::Updater * updater;
			
			
			std::atomic<bool> do_shutdown;
			bool detached;
			
			
			LocalEndpoint ep;
			
			
			virtual NetworkImpl::FollowUp Perform (const NetworkImpl::Notification &) override;
			virtual void SetUpdater (NetworkImpl::Updater *) override;
			virtual bool Update (NetworkImpl::Notifier &) override;
			
			
		public:
			
			
			ListeningSocket () = delete;
			ListeningSocket (const ListeningSocket &) = delete;
			ListeningSocket (ListeningSocket &&) = delete;
			ListeningSocket & operator = (const ListeningSocket &) = delete;
			ListeningSocket & operator = (ListeningSocket &&) = delete;
			
			
			ListeningSocket (NetworkImpl::FDType, LocalEndpoint) noexcept;
			~ListeningSocket () noexcept;
			
		
			void Shutdown () noexcept;
			
		
	};
	
	
	class Connection : public NetworkImpl::ChannelBase {
	
	
		private:
		
		
			class SendBuffer {
			
			
				public:
				
				
					//	The bytes to be sent
					Vector<Byte> Buffer;
					//	How many bytes have been
					//	sent
					Word Sent;
					//	The promise that will be
					//	fulfilled when this completes
					Promise<bool> Completion;
					
					
					SendBuffer () = delete;
					SendBuffer (Vector<Byte>) noexcept;
			
			
			};
		
		
			NetworkImpl::FDType socket;
			
			
			bool connecting;
			std::atomic<bool> connected;
			//	Protected by the send lock
			bool is_shutdown;
			
			
			NetworkImpl::Error error;
			
			
			//	The receieve buffer
			Vector<Byte> buffer;
			std::atomic<bool> pending_recv;
			
			
			//	Pending sends
			mutable Mutex lock;
			Vector<SendBuffer> sends;
			
			
			//	Callbacks
			DisconnectType disconnect;
			ReceiveType receive;
			ConnectType connect;
			
			
			NetworkImpl::Updater * updater;
			
			
			//	Statistics
			std::atomic<Word> sent;
			std::atomic<Word> received;
			
			
			//	Endpoints
			IPAddress local_ip;
			UInt16 local_port;
			IPAddress remote_ip;
			UInt16 remote_port;
			
			
			void shutdown (bool);
			void get_disconnect (NetworkImpl::FollowUp &);
			bool read (NetworkImpl::FollowUp &);
			void write (NetworkImpl::FollowUp &);
			bool get_local_endpoint () noexcept;
			void get_connect (NetworkImpl::FollowUp &);
			
			
			//	Interface implementation
			virtual NetworkImpl::FollowUp Perform (const NetworkImpl::Notification &) override;
			virtual void SetUpdater (NetworkImpl::Updater *) override;
			virtual bool Update (NetworkImpl::Notifier &) override;
			
			
		public:
		
		
			Connection () = delete;
			Connection (const Connection &) = delete;
			Connection (Connection &&) = delete;
			Connection & operator = (const Connection &) = delete;
			Connection & operator = (Connection &&) = delete;
			
			
			Connection (NetworkImpl::FDType, IPAddress, UInt16, IPAddress, UInt16, ReceiveType, DisconnectType) noexcept;
			Connection (NetworkImpl::FDType, RemoteEndpoint) noexcept;
			
			
			~Connection () noexcept;
			
			
			void Disconnect ();
			void Disconnect (String reason);
			
			
			Promise<bool> Send (Vector<Byte> buffer);
			
			
			IPAddress IP () const noexcept;
			UInt16 Port () const noexcept;
			Word Sent () const noexcept;
			Word Received () const noexcept;
	
	
	};
	
	
	class ConnectionHandler {
	
	
		public:
		
		
			typedef std::function<void (std::exception_ptr)> PanicType;
	
	
		private:
		
		
			enum class CommandType {
			
				Update,
				Add,
				Shutdown
			
			};
		
		
			//	A command to a worker thread
			class Command {
			
			
				public:
				
				
					Command () = delete;
					Command (CommandType, NetworkImpl::FDType fd=-1, SmartPointer<NetworkImpl::ChannelBase> impl=SmartPointer<NetworkImpl::ChannelBase>{}) noexcept;
				
				
					CommandType Type;
					NetworkImpl::FDType FD;
					SmartPointer<NetworkImpl::ChannelBase> Impl;
			
			
			};
			
			
			//	A communications channel to a worker
			//	thread
			class WorkerChannel {
			
			
				private:
				
				
					SocketPair pair;
					mutable Mutex lock;
					Vector<Command> commands;
					
					
				public:
				
				
					WorkerChannel ();
				
				
					void Attach (NetworkImpl::Notifier &);
					void Send (Command);
					Nullable<Command> Receive ();
					bool Is (NetworkImpl::FDType) const noexcept;
			
			
			};
		
		
			//	Worker thread block
			class Worker : public NetworkImpl::Updater {
			
			
				public:
				
				
					//	Notifier
					NetworkImpl::Notifier N;
					//	The actual underlying thread
					Thread T;
					//	Commands
					WorkerChannel Control;
					//	File descriptors managed
					std::unordered_map<
						NetworkImpl::FDType,
						SmartPointer<NetworkImpl::ChannelBase>
					> FDs;
					//	Approximate number of managed
					//	FDs
					std::atomic<Word> Count;
					
					
					Worker ();
					
					
					virtual void Update (NetworkImpl::FDType) override;
			
			
			};
			
			
			//	Thread pool
			ThreadPool & pool;
			
			
			//	Workers
			Vector<Worker> workers;
			
			
			//	Manages pending callbacks
			mutable Mutex lock;
			mutable CondVar wait;
			Word callbacks;
			
			
			//	Statistics
			std::atomic<Word> sent;
			std::atomic<Word> received;
			std::atomic<Word> incoming;
			std::atomic<Word> outgoing;
			std::atomic<Word> accepted;
			std::atomic<Word> disconnected;
			
			
			//	Startup control
			bool proceed;
			bool shutdown;
			
			
			//	Panic callback
			PanicType panic;
			
			
			[[noreturn]]
			void do_panic () noexcept;
			void complete_callback () noexcept;
			template <typename T, typename... Args>
			auto enqueue (T && callback, Args &&... args) -> Promise<decltype(callback(std::forward<Args>(args)...))>;
			void handle (const NetworkImpl::FollowUp &) noexcept;
			void add (NetworkImpl::FDType, SmartPointer<NetworkImpl::ChannelBase>);
			void handle (Worker &, NetworkImpl::FDType, SmartPointer<NetworkImpl::ChannelBase> &, NetworkImpl::FollowUp, bool synchronous=true);
			bool process_control (Worker &);
			bool process_notification (Worker &, NetworkImpl::Notification &);
			void worker (Worker &);
			void worker_func (Word) noexcept;
			
			
		public:
		
		
			ConnectionHandler (
				ThreadPool & pool,
				Nullable<Word> num_workers=Nullable<Word>{},
				PanicType panic=PanicType{}
			);
			
			
			~ConnectionHandler () noexcept;
			
			
			void Connect (RemoteEndpoint ep);
			
			
			SmartPointer<ListeningSocket> Listen (LocalEndpoint ep);
	
	
	};


}

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <cstddef>
#include <utility>
#ifdef ENVIRONMENT_WINDOWS
#include <windows.h>
#else
#include <sys/time.h>
#endif


namespace MCPP {


	/**
	 *	A pair of connected, full-duplex sockets.
	 */
	class SocketPair {


		public:
		
		
			/**
			 *	The type of a socket.
			 *
			 *	Different between Windows and Linux.
			 */
			typedef
			#ifdef ENVIRONMENT_WINDOWS
			SOCKET
			#else
			int
			#endif
			Type;


		private:
		
		
			Type pair [2];
			
			
		public:
		
		
			/**
			 *	Creates a pair of connected, full-duplex
			 *	sockets.
			 */
			SocketPair ();
			/**
			 *	Destroys the managed pair of sockets.
			 */
			~SocketPair () noexcept;
			
			
			/**
			 *	Retrieves one of the two sockets.
			 *
			 *	\param [in] i
			 *		The socket to retrieve.
			 *
			 *	\return
			 *		A reference to the \em ith socket.
			 */
			Type & operator [] (std::size_t i) noexcept;
			/**
			 *	Retrieves one of the two sockets.
			 *
			 *	\param [in] i
			 *		The socket to retrieve.
			 *
			 *	\return
			 *		A reference to the \em ith socket.
			 */
			const Type & operator [] (std::size_t i) const noexcept;


	};
	
	
	/**
	 *	A socket pair intended for use in controlling a worker
	 *	thread.
	 *
	 *	Data may be written to the master end, and read from
	 *	the worker end.
	 *
	 *	The worker socket may be seamlessly added to a fd_set
	 *	structure for use in select polling.
	 */
	class ControlSocket {
	
	
		private:
		
		
			SocketPair pair;
			
			
			void send (Byte);
			Nullable<Byte> recv ();
			
			
		public:
		
		
			/**
			 *	The type of a socket.
			 *
			 *	Different between Windows and Linux.
			 */
			typedef SocketPair::Type Type;
			
			
			ControlSocket ();
			
			
			/**
			 *	Adds the slave end of the control socket
			 *	to a fd_set structure.
			 *
			 *	\param [in,out] set
			 *		The fd_set structure which shall be passed
			 *		to select.  Should be the set which is being
			 *		tested for writeability.
			 *	\param [in] nfds
			 *		Ignored on Windows (since Windows implementation
			 *		of select ignores the nfds parameter).  Optional.
			 *		Will be used to determine what value should be
			 *		passed to select for the parameter nfds.  Should
			 *		be the highest file descriptor in all sets to be
			 *		selected on plus one.
			 *
			 *	\return
			 *		The value of nfds that should be passed to select.
			 */
			int Add (fd_set & set, int nfds=0) const noexcept;
			
			
			/**
			 *	Checks to see if the slave end of the control
			 *	socket is in a fd_set structure.
			 *
			 *	\param [in] set
			 *		The set to check.
			 *
			 *	\return
			 *		\em true if the slave end is in the given
			 *		fd_set structure, \em false otherwise.
			 */
			bool Is (const fd_set & set) const noexcept;
			
			
			/**
			 *	Removes the slave end of the control socket from
			 *	a fd_set structure.
			 *
			 *	\param [in,out] set
			 *		The set from which to remove the slave end.
			 */
			void Clear (fd_set & set) const noexcept;
			
			
			/**
			 *	Sends a message across the control socket.
			 *
			 *	\tparam T
			 *		The type of object that shall be sent.
			 *		Cannot exceed one byte.
			 *
			 *	\param [in] obj
			 *		The object to send.
			 */
			template <typename T>
			void Send (T obj) {
			
				static_assert(
					sizeof(obj)==1,
					"obj must be one byte"
				);
				
				union {
					T in;
					Byte out;
				};
				in=obj;
				
				send(out);
			
			}
			
			
			/**
			 *	Receives a message on the control socket.
			 *
			 *	\tparam T
			 *		The type of object that shall be
			 *		received.  Cannot exceed one byte.
			 *
			 *	\return
			 *		An object of type \em T if there was
			 *		data to be read from the socket.  Null
			 *		if there was no data to be read.
			 */
			template <typename T>
			Nullable<T> Receive () {
			
				static_assert(
					sizeof(T)==1,
					"obj must be one byte"
				);
				
				auto recvd=recv();
				
				if (recvd.IsNull()) return Nullable<T>{};
				
				union {
					T out;
					Byte in;
				};
				in=*recvd;
				
				return out;
			
			}
	
	
	};
	
	
}

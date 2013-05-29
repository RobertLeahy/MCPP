/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <connection_manager.hpp>
#include <unordered_map>
#include <utility>
#include <packet.hpp>


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
	enum class ClientState {
	
		Connected	/**<	Client is newly-connected, and hasn't entered any state yet.	*/
	
	};


	/**
	 *	Contains information about a connected
	 *	client.
	 */
	class Client {
	
	
		friend class ClientList;
	
	
		private:
		
		
			SmartPointer<Connection> conn;
			
			
		public:
		
		
			Packet InProgress;
		
		
			Client (SmartPointer<Connection> && conn) noexcept;
		
		
			SmartPointer<Connection> Conn () noexcept;
	
	
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
			 *	Removes all clients.
			 */
			void Clear () noexcept;
			
	
	};


}

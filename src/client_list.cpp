#include <client.hpp>


namespace MCPP {


	SmartPointer<Client> ClientList::operator [] (const Connection & conn) {
	
		Nullable<SmartPointer<Client>> client;
	
		map_lock.Read();
		
		try {
		
			client.Construct(map.at(&conn));
		
		} catch (...) {
		
			map_lock.CompleteRead();
			
			throw;
		
		}
		
		map_lock.CompleteRead();
		
		return *client;
	
	}
	
	
	void ClientList::Add (const SmartPointer<Client> & client) {
	
		map_lock.Write();
		
		try {
	
			map.insert(
				decltype(map)::value_type(
					static_cast<const Connection *>(client->conn),
					client
				)
			);
			
		} catch (...) {
		
			map_lock.CompleteWrite();
			
			throw;
		
		}
		
		map_lock.CompleteWrite();
	
	}
	
	
	void ClientList::Remove (const Connection & conn) {
	
		map_lock.Write();
		
		try {
		
			map.erase(&conn);
		
		} catch (...) {
		
			map_lock.CompleteWrite();
			
			throw;
		
		}
		
		map_lock.CompleteWrite();
	
	}
	
	
	Word ClientList::Count () const noexcept {
	
		map_lock.Read();
		
		Word count=map.size();
		
		map_lock.CompleteRead();
		
		return count;
	
	}
	
	
	void ClientList::Clear () noexcept {
	
		map_lock.Write();
		
		map.clear();
		
		map_lock.CompleteWrite();
	
	}


}

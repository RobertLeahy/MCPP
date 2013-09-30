#include <client.hpp>
#include <utility>


namespace MCPP {


	ClientList::ClientList () noexcept : iters(0) {	}


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
	
	
	void ClientList::Add (SmartPointer<Client> client) {
	
		map_lock.Write();
		
		try {
	
			map.insert(
				decltype(map)::value_type(
					static_cast<const Connection *>(client->conn),
					std::move(client)
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
	
	
	Word ClientList::AuthenticatedCount () const noexcept {
	
		Word count=0;
		
		map_lock.Read();
		
		for (const auto & pair : map) {
		
			if (pair.second->GetState()==ClientState::Authenticated) ++count;
		
		}
		
		map_lock.CompleteRead();
		
		return count;
	
	}
	
	
	void ClientList::Clear () noexcept {
	
		map_lock.Write();
		
		map.clear();
		
		map_lock.CompleteWrite();
	
	}
	
	
	inline void ClientList::acquire () noexcept {
	
		iters_lock.Acquire();
		if ((iters++)==0) map_lock.Read();
		iters_lock.Release();
	
	}
	
	
	ClientListIterator ClientList::begin () noexcept {
	
		acquire();
	
		return ClientListIterator(
			*this,
			map.begin()
		);
	
	}
	
	
	ClientListIterator ClientList::end () noexcept {
	
		acquire();
		
		return ClientListIterator(
			*this,
			map.end()
		);
	
	}


}

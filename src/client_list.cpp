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
		
			if (pair.second->GetState()==ProtocolState::Play) ++count;
		
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
	
	
	Vector<SmartPointer<Client>> ClientList::Get (const String & str, ClientSearch type) const {
	
		Vector<SmartPointer<Client>> retr;
		
		//	Choose the regular expression
		//	pattern that will be used
		String pattern;
		String regex_str(Regex::Escape(str));
		switch (type) {
		
			case ClientSearch::Exact:
			default:
				pattern=String::Format(
					"^{0}$",
					regex_str
				);
				break;
				
			case ClientSearch::Begin:
				pattern=String::Format(
					"^{0}",
					regex_str
				);
				break;
				
			case ClientSearch::End:
				pattern=String::Format(
					"{0}$",
					regex_str
				);
				break;
				
			case ClientSearch::Match:
				pattern=std::move(regex_str);
				break;
		
		}
		
		//	Create the regular expression
		Regex regex(
			pattern,
			RegexOptions().SetIgnoreCase()
		);
		
		//	Loop and search clients
		for (const auto & client : *const_cast<ClientList *>(this)) if (
			//	Only match authenticated clients
			//	(the only kind who have usernames
			//	anyway)
			(client->GetState()==ProtocolState::Play) &&
			//	Attempt to match against regular
			//	expression
			regex.IsMatch(client->GetUsername())
		) retr.Add(client);
		
		return retr;
	
	}


}

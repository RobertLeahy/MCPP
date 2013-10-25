#include <client.hpp>
#include <utility>


namespace MCPP {


	SmartPointer<Client> ClientList::operator [] (const Connection & conn) {
	
		return map_lock.Read([&] () mutable {	return map.at(&conn);	});
	
	}
	
	
	void ClientList::Add (SmartPointer<Client> client) {
	
		map_lock.Write([&] () mutable {
		
			auto conn=client->conn;
			
			map.emplace(
				conn,
				std::move(client)
			);
			
		});
	
	}
	
	
	void ClientList::Remove (const Connection & conn) {
	
		map_lock.Write([&] () mutable {	map.erase(&conn);	});
	
	}
	
	
	Word ClientList::Count () const noexcept {
	
		return map_lock.Read([&] () {	return map.size();	});
	
	}
	
	
	Word ClientList::AuthenticatedCount () const noexcept {
	
		return map_lock.Read([&] () {
		
			Word retr=0;
		
			for (const auto & pair : map) if (pair.second->GetState()==ProtocolState::Play) ++retr;
			
			return retr;
		
		});
	
	}
	
	
	void ClientList::Clear () noexcept {
	
		map_lock.Write([&] () mutable {	map.clear();	});
	
	}
	
	
	ClientListIterator ClientList::begin () noexcept {
	
		return ClientListIterator(
			this,
			map.begin()
		);
	
	}
	
	
	ClientListIterator ClientList::end () noexcept {
		
		return ClientListIterator(
			this,
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

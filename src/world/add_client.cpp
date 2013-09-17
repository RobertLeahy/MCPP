#include <world/world.hpp>


namespace MCPP {


	void WorldContainer::Add (SmartPointer<Client> client, ColumnID id) {
	
		auto column=get_column(id);
		
		try {
	
			//	Wait until column is populated, or
			//	see if column will eventually enter
			//	desired state
			if (!(
				async
					?	column->Check(ColumnState::Populated)
					:	column->WaitUntil(ColumnState::Populated)
			)) process(*column);
			
			clients_lock.Execute([&] () {
			
				auto iter=clients.find(client);
				
				if (iter==clients.end()) {
				
					auto pair=clients.emplace(
						client,
						std::unordered_set<ColumnID>()
					);
					
					try {
					
						pair.first->second.insert(id);
					
					} catch (...) {
					
						clients.erase(client);
						
						throw;
					
					}
				
				} else if (iter->second.count(id)!=0) {
				
					//	Client already has column, abort
					return;
				
				} else {
				
					iter->second.insert(id);
				
				}
				
				try {
				
					column->AddPlayer(client);
				
				} catch (...) {
				
					clients.find(client)->second.erase(id);
					
					throw;
				
				}
			
			});
			
		} catch (...) {
		
			column->EndInterest();
			
			throw;
		
		}
		
		column->EndInterest();
	
	}
	
	
	void WorldContainer::Remove (SmartPointer<Client> client, ColumnID id, bool force) {
	
		if (clients_lock.Execute([&] () {
		
			auto iter=clients.find(client);
			
			if (iter==clients.end()) return false;
			
			return iter->second.erase(id)!=0;
		
		})) {
		
			auto column=get_column(id);
			
			try {
			
				column->RemovePlayer(
					std::move(client),
					force
				);
			
			} catch (...) {
			
				column->EndInterest();
				
				throw;
			
			}
			
			column->EndInterest();
			
		}
	
	}
	
	
	void WorldContainer::Remove (SmartPointer<Client> client, bool force) {
	
		std::unordered_set<ColumnID> set;
		
		clients_lock.Execute([&] () {
		
			auto iter=clients.find(client);
			
			if (iter!=clients.end()) {
			
				set=std::move(iter->second);
			
				clients.erase(iter);
				
			}
		
		});
		
		for (auto id : set) {
		
			auto column=get_column(id);
			
			try {
		
				column->RemovePlayer(
					client,
					force
				);
				
			} catch (...) {
			
				column->EndInterest();
				
				throw;
			
			}
			
			column->EndInterest();
			
		}
	
	}


}

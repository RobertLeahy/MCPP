#include <client.hpp>


namespace MCPP {


	void ClientListIterator::destroy () noexcept {
	
		if (list!=nullptr) {
		
			list->map_lock.CompleteRead();
			
			list=nullptr;
		
		}
	
	}


	ClientListIterator::ClientListIterator (ClientList * list, iter_type iter) noexcept
		:	iter(std::move(iter)),
			list(list)
	{
	
		list->map_lock.Read();
	
	}
	
	
	ClientListIterator::ClientListIterator (const ClientListIterator & other) noexcept : iter(other.iter), list(other.list) {
	
		list->map_lock.Read();
	
	}
	
	
	ClientListIterator::ClientListIterator (ClientListIterator && other) noexcept : iter(std::move(other.iter)), list(other.list) {
	
		other.list=nullptr;
	
	}
	
	
	ClientListIterator & ClientListIterator::operator = (const ClientListIterator & other) noexcept {
	
		if (this!=&other) {
		
			destroy();
			
			iter=other.iter;
			list=other.list;
			
			list->map_lock.Read();
			
		}
	
		return *this;
	
	}
	
	
	ClientListIterator & ClientListIterator::operator = (ClientListIterator && other) noexcept {
	
		if (this!=&other) {
		
			destroy();
		
			iter=std::move(other.iter);
			list=other.list;
			other.list=nullptr;
		
		}
		
		return *this;
	
	}
	
	
	ClientListIterator::~ClientListIterator () noexcept {
	
		destroy();
	
	}
	
	
	SmartPointer<Client> & ClientListIterator::operator * () noexcept {
	
		return iter->second;
	
	}
	
	
	SmartPointer<Client> * ClientListIterator::operator -> () noexcept {
	
		return &(iter->second);
	
	}
	
	
	ClientListIterator & ClientListIterator::operator ++ () noexcept {
	
		++iter;
		
		return *this;
	
	}
	
	
	ClientListIterator ClientListIterator::operator ++ (int) noexcept {
	
		auto retr=*this;
		
		++iter;
		
		return retr;
	
	}
	
	
	bool ClientListIterator::operator == (const ClientListIterator & other) const noexcept {
	
		return iter==other.iter;
	
	}
	
	
	bool ClientListIterator::operator != (const ClientListIterator & other) const noexcept {
	
		return iter!=other.iter;
	
	}


}

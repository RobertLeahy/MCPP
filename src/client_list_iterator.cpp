#include <client.hpp>


namespace MCPP {


	ClientListIterator::ClientListIterator (ClientList * list, iter_type iter) noexcept
		:	iter(std::move(iter)),
			list(list)
	{
	
		list->map_lock.Read();
	
	}
	
	
	ClientListIterator::ClientListIterator (const ClientListIterator & other) noexcept : iter(other.iter), list(other.list) {
	
		list->map_lock.Read();
	
	}
	
	
	ClientListIterator & ClientListIterator::operator = (const ClientListIterator & other) noexcept {
	
		list->map_lock.CompleteRead();
		
		iter=other.iter;
		list=other.list;
		
		list->map_lock.Read();
	
		return *this;
	
	}
	
	
	ClientListIterator::~ClientListIterator () noexcept {
	
		list->map_lock.CompleteRead();
	
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

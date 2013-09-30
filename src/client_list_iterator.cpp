#include <client.hpp>


namespace MCPP {


	ClientListIterator::ClientListIterator (ClientList & list, iter_type iter) noexcept
		:	iter(std::move(iter)),
			list(list)
	{	}
	
	
	ClientListIterator::~ClientListIterator () noexcept {
	
		list.iters_lock.Acquire();
		if ((--list.iters)==0) list.map_lock.CompleteRead();
		list.iters_lock.Release();
	
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

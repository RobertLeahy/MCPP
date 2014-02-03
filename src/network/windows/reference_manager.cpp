#include <network.hpp>


namespace MCPP {


	namespace NetworkImpl {
	
	
		void ReferenceManagerHandle::destroy () noexcept {
		
			if (manager!=nullptr) {
			
				manager->End();
				
				manager=nullptr;
			
			}
		
		}
	
	
		ReferenceManagerHandle::ReferenceManagerHandle (ReferenceManager & manager) noexcept : manager(&manager) {
		
			manager.Begin();
		
		}
		
		
		ReferenceManagerHandle::ReferenceManagerHandle (const ReferenceManagerHandle & other) noexcept : manager(other.manager) {
		
			if (manager!=nullptr) manager->Begin();
		
		}
		
		
		ReferenceManagerHandle::ReferenceManagerHandle (ReferenceManagerHandle && other) noexcept : manager(other.manager) {
		
			other.manager=nullptr;
		
		}
		
		
		ReferenceManagerHandle & ReferenceManagerHandle::operator = (const ReferenceManagerHandle & other) noexcept {
		
			if (this!=&other) {
			
				destroy();
				
				manager=other.manager;
				if (manager!=nullptr) manager->Begin();
			
			}
			
			return *this;
		
		}
		
		
		ReferenceManagerHandle & ReferenceManagerHandle::operator = (ReferenceManagerHandle && other) noexcept {
		
			if (this!=&other) {
			
				destroy();
				
				manager=other.manager;
				other.manager=nullptr;
			
			}
		
			return *this;
		
		}
		
		
		ReferenceManagerHandle::~ReferenceManagerHandle () noexcept {
		
			destroy();
		
		}
	
	
		ReferenceManager::ReferenceManager () noexcept : count(0) {	}
		
		
		ReferenceManager::~ReferenceManager () noexcept {
		
			Wait();
		
		}
		
		
		void ReferenceManager::Begin () noexcept {
		
			lock.Execute([&] () mutable {	++count;	});
		
		}
		
		
		void ReferenceManager::End () noexcept {
		
			lock.Execute([&] () mutable {	if ((--count)==0) wait.WakeAll();	});
		
		}
		
		
		ReferenceManagerHandle ReferenceManager::Get () noexcept {
			
			return ReferenceManagerHandle(*this);
		
		}
		
		
		void ReferenceManager::Wait () const noexcept {
		
			lock.Execute([&] () mutable {	while (count!=0) wait.Sleep(lock);	});
		
		}
		
		
		void ReferenceManager::Reset () noexcept {
		
			lock.Execute([&] () mutable {	count=0;	});
		
		}
	
	
	}


}

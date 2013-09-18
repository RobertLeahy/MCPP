#include <concurrency_manager.hpp>


namespace MCPP {


	ConcurrencyManager::ConcurrencyManager (
		ThreadPool & pool,
		Word max,
		std::function<void ()> panic
	) noexcept : pool(pool), max(max), running(0), panic(std::move(panic)) {
	
		//	Sanity check
		if (this->max==0) this->max=1;
	
	}
	
	
	Word ConcurrencyManager::Maximum () const noexcept {
	
		return max;
	
	}


}

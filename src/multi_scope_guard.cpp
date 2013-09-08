#include <multi_scope_guard.hpp>
#include <utility>


namespace MCPP {


	MultiScopeGuardIndirect::MultiScopeGuardIndirect (
		std::function<void ()> all,
		std::function<void ()> each,
		std::function<void ()> panic
	) noexcept : ExitAll(std::move(all)), ExitEach(std::move(each)), Panic(std::move(panic)) {
	
		count=1;
	
	}
	
	
	void MultiScopeGuardIndirect::Acquire () noexcept {
	
		++count;
	
	}
	
	
	bool MultiScopeGuardIndirect::Release () noexcept {
	
		return (--count)==0;
	
	}
	
	
	inline void MultiScopeGuard::destroy () noexcept {
	
		if (indirect!=nullptr) {
		
			try {
			
				if (indirect->ExitEach) indirect->ExitEach();
			
			} catch (...) {
			
				try {
				
					if (indirect->Panic) indirect->Panic();
				
				} catch (...) {	}
			
			}
			
			if (indirect->Release()) {
			
				try {
				
					if (indirect->ExitAll) indirect->ExitAll();
				
				} catch (...) {
				
					try {
					
						if (indirect->Panic) indirect->Panic();
					
					} catch (...) {	}
				
				}
				
				delete indirect;
			
			}
		
		}
	
	}
	
	
	MultiScopeGuard::MultiScopeGuard () noexcept : indirect(nullptr) {	}
	
	
	MultiScopeGuard::MultiScopeGuard (std::function<void ()> all) : indirect(new MultiScopeGuardIndirect(
		std::move(all),
		std::function<void ()>(),
		std::function<void ()>()
	)) {	}
	
	
	MultiScopeGuard::MultiScopeGuard (
		std::function<void ()> all,
		std::function<void ()> each
	) : indirect(new MultiScopeGuardIndirect(
		std::move(all),
		std::move(each),
		std::function<void ()>()
	)) {	}
	
	
	MultiScopeGuard::MultiScopeGuard (
		std::function<void ()> all,
		std::function<void ()> each,
		std::function<void ()> panic
	) : indirect(new MultiScopeGuardIndirect(
		std::move(all),
		std::move(each),
		std::move(panic)
	)) {	}
	
	
	MultiScopeGuard::MultiScopeGuard (const MultiScopeGuard & other) noexcept : indirect(other.indirect) {
	
		if (indirect!=nullptr) indirect->Acquire();
	
	}
	
	
	MultiScopeGuard::MultiScopeGuard (MultiScopeGuard && other) noexcept : indirect(other.indirect) {
	
		other.indirect=nullptr;
	
	}
	
	
	MultiScopeGuard & MultiScopeGuard::operator = (const MultiScopeGuard & other) noexcept {
	
		if (&other!=this) {
		
			destroy();
			
			indirect=other.indirect;
			if (indirect!=nullptr) indirect->Acquire();
		
		}
		
		return *this;
	
	}
	
	
	MultiScopeGuard & MultiScopeGuard::operator = (MultiScopeGuard && other) noexcept {
	
		if (&other!=this) {
		
			destroy();
			
			indirect=other.indirect;
			other.indirect=nullptr;
		
		}
		
		return *this;
	
	}
	
	
	MultiScopeGuard::~MultiScopeGuard () noexcept {
	
		destroy();
	
	}


}

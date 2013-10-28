#include <network.hpp>
#include <utility>


using namespace MCPP::NetworkImpl;


namespace MCPP {


	SendHandle::SendHandle (Vector<Byte> buffer) noexcept : Buffer(std::move(buffer)), Sent(0), state(SendState::InProgress) {	}
	
	
	void SendHandle::Complete (ThreadPool & pool) {
	
		lock.Execute([&] () mutable {
		
			//	Set state
			state=SendState::Succeeded;
		
			//	Fire callbacks
			std::exception_ptr ex;
			for (auto & callback : callbacks) try {
			
				pool.Enqueue(
					std::move(callback),
					SendState::Succeeded
				);
			
			} catch (...) {
			
				//	We need to complete this operation,
				//	save this exception and rethrow
				//	later
				ex=std::current_exception();
			
			}
			
			//	Release waiting threads
			wait.WakeAll();
			
			//	Rethrow exception if necessary
			if (ex) std::rethrow_exception(ex);
		
		});
	
	}


}

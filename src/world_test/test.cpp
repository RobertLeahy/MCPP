#include <world/world.hpp>
#include <atomic>


Nullable<ThreadPool> pool;
Nullable<WorldLock> lock;
std::atomic<Word> num;
Mutex console_lock;


int main () {

	pool.Construct(7);
	lock.Construct(*pool);
	num=0;
	
	for (Word i=0;i<7;++i) {
	
		pool->Enqueue([] () {
		
			Word thread_id=++num;
			
			for (;;) {
			
				const void * handle;
			
				if (thread_id==6) {
			
					WorldLockRequest request(ColumnID{1,0,0});
					
					handle=lock->Acquire(std::move(request));
			
				} else if (thread_id==7) {
				
					WorldLockRequest request;
					
					handle=lock->Acquire(std::move(request));
				
				} else {
			
					WorldLockRequest request(ColumnID{0,0,0});
					
					handle=lock->Acquire(std::move(request));
					
				}
				
				console_lock.Execute([=] () {	StdOut << thread_id << Newline;	});
				
				Thread::Sleep(1000);
				
				lock->Release(handle);
			
			}
		
		});
	
	}
	
	for (;;) Thread::Sleep(1000);

	return 0;

}

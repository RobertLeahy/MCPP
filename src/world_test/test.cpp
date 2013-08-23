#include <world/world.hpp>
#include <atomic>


Nullable<ThreadPool> pool;
Nullable<WorldLock> lock;
Mutex console_lock;


int main () {

	pool.Construct(5);
	lock.Construct(*pool);
	
	WorldLockRequest request(
		ColumnID{
			0,
			0,
			1
		}
	);
	
	lock->Enqueue(
		std::move(request),
		[] (const void * handle) {
		
			console_lock.Execute([] () {	StdOut << "Have the lock on 0,0,1" << Newline;	});
			
			Thread::Sleep(2500);
			
			console_lock.Execute([] () {	StdOut << "Releasing..." << Newline;	});
			
			lock->Release(handle);
		
		}
	);
	
	WorldLockRequest request2(
		ColumnID{
			1,
			0,
			1
		}
	);
	
	lock->Enqueue(
		std::move(request),
		[] (const void * handle) {
		
			console_lock.Execute([] () {	StdOut << "Have the lock on 1,0,1" << Newline;	});
			
			WorldLockRequest request(
				ColumnID{
					1,
					1,
					1
				}
			);
			
			lock->Upgrade(
				handle,
				request,
				[] (const void * handle) {
				
					console_lock.Execute([] () {	StdOut << "Have the lock on 1,1,1" << Newline;	});
					
					WorldLockRequest request(
						ColumnID{
							0,
							0,
							1
						}
					);
					
					lock->Upgrade(
						handle,
						request,
						[] (const void * handle) {
						
							console_lock.Execute([] () {	StdOut << "Have the lock on 0,0,1" << Newline;	});
							
							lock->Release(handle);
						
						}
					);
				
				}
			);
		
		}
	);
	
	for (;;) Thread::Sleep(1000);

	return 0;

}

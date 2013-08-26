#include <world/world.hpp>
#include <atomic>


Nullable<ThreadPool> pool;
Nullable<WorldLock> lock;
Mutex console_lock;
static const Word iterations=1000000;


int main () {

	pool.Construct(5);
	lock.Construct(*pool);

	Vector<Thread> threads(5);

	Timer timer=Timer::CreateAndStart();
	
	threads.Add(
		Thread(
			[] () {
			
				WorldLockRequest request(
					ColumnID{
						0,
						0,
						0
					}
				);
				
				for (Word i=0;i<iterations;++i) {
				
					lock->Release(
						lock->Acquire(
							request
						)
					);
				
				}
			
			}
		)
	);
	
	threads.Add(
		Thread(
			[] () {
			
				WorldLockRequest request(
					BlockID{
						32,
						0,
						0,
						0
					}
				);
				
				for (Word i=0;i<iterations;++i) {
				
					lock->Release(
						lock->Acquire(
							request
						)
					);
				
				}
			
			}
		)
	);
	
	threads.Add(
		Thread(
			[] () {
		
				WorldLockRequest request;
				
				for (Word i=0;i<iterations;++i) {
				
					lock->Release(
						lock->Acquire(
							request
						)
					);
				
				}
		
			}
		)
	);
	
	for (auto & t : threads) t.Join();
	
	timer.Stop();
	
	StdOut	<< "Time elapsed: "
			<< timer.ElapsedNanoseconds()
			<< "ns"
			<< Newline
			<< "Time per acquire/release: "
			<< (timer.ElapsedNanoseconds()/(iterations*3))
			<< "ns"
			<< Newline;

}

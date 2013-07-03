#include <network.hpp>
#include <thread_pool.hpp>
#include <rleahylib/main.hpp>
#include <unordered_map>
#include <world/world.hpp>
#include <atomic>


using namespace MCPP;



int Main (const Vector<const String> & args) {

	std::atomic<bool> stop;
	std::atomic<Word> count;
	stop=false;
	count=0;

	ThreadPool pool(10);
	
	WorldLock lock(pool);
	
	Mutex console_lock;

	Thread t([&] () {
	
		while (!stop) {
	
			auto id=lock.Acquire(ColumnID{0,0,0});
			
			++count;
			
			//console_lock.Execute([] () {	StdOut << "Column lock acquired" << Newline;	});
			
			//Thread::Sleep(1000);
			
			//console_lock.Execute([] () {	StdOut << "Column lock released" << Newline;	});
			
			lock.Release(id);
			
		}
	
	});
	
	Thread t1([&] () {
	
		while (!stop) {
		
			auto id=lock.Acquire(BlockID{0,0,0,0});
			
			++count;
			
			//console_lock.Execute([] () {	StdOut << "Acquired!" << Newline;	});
			
			//Thread::Sleep(1000);
			
			//console_lock.Execute([] () {	StdOut << "Released!" << Newline;	});
			
			lock.Release(id);
		
		}
	
	});
	
	Thread t2([&] () {
	
		while (!stop) {
		
			auto id=lock.Acquire(BlockID{1,0,0,0});
			
			++count;
			
			//console_lock.Execute([] () {	StdOut << "Acquired!" << Newline;	});
			
			//Thread::Sleep(1000);
			
			//console_lock.Execute([] () {	StdOut << "Released!" << Newline;	});
			
			lock.Release(id);
		
		}
	
	});
	
	Thread t3([&] () {
	
		while (!stop) {
		
			auto id=lock.Acquire();
			
			++count;
			
			//StdOut << "Exclusive lock acquired!" << Newline;
			
			//Thread::Sleep(1000);
			
			//StdOut << "Exclusive lock released!" << Newline;
			
			lock.Release(id);
		
		}
	
	});
	
	Thread t4([&] () {
	
		while (!stop) {
		
			auto id=lock.Acquire(BlockID{16,0,0,0});
			
			++count;
			
			//console_lock.Execute([] () {	StdOut << "Unrelated acquired!" << Newline;	});
			
			//Thread::Sleep(1000);
			
			//console_lock.Execute([] () {	StdOut << "Unrelated released!" << Newline;	});
			
			lock.Release(id);
		
		}
	
	});
	
	//for (;;) Thread::Sleep(1000);
	//StdIn.ReadLine();
	Thread::Sleep(1000);
	
	stop=true;
	
	t.Join();
	t1.Join();
	t2.Join();
	t4.Join();
	
	StdOut << Word(count) << Newline;

	/*auto lambda=[&] () {
		
		while (!stop) {
		
			lock.Release(lock.Acquire({0,0,0,0}));
			
			++count;
		
		}
		
	};
	
	auto lambda_2=[&] () {
	
		while (!stop) {
		
			lock.Release(lock.Acquire({1,0,0,0}));
			
			++count;
		
		}
	
	};
	
	Thread t(lambda);
	Thread t2(lambda);
	Thread t3(lambda);
	Thread t4(lambda_2);
	
	Thread::Sleep(1000);
	
	stop=true;
	
	t.Join();
	t2.Join();
	t3.Join();
	t4.Join();
	
	StdOut << Word(count) << Newline;*/

	return EXIT_SUCCESS;

}

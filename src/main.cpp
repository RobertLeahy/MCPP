#include <common.hpp>
#include <rleahylib/main.hpp>
#include <iterator>
#include <limits>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <iostream>
#include <type_traits>


static const Int32 x_limit=10;
static const Int32 y_limit=16;
static const Int32 z_limit=10;


int Main (const Vector<const String> & args) {

	DataProvider * dp=DataProvider::GetDataProvider();
	
	try {
	
		Barrier barrier(2);
		
		std::atomic<Word> finished;
		
		finished=0;
	
		StdOut << "Generating " << x_limit << "x" << z_limit << " area centered at the origin with all-ascending bytes..." << Newline;
	
		Timer timer=Timer::CreateAndStart();
		
		Vector<Vector<Byte>> chunks(x_limit*y_limit*z_limit);
		
		for (Word i=0;i<chunks.Capacity();++i) {
		
			chunks.EmplaceBack(16*16*16);
			
			Byte b=0;
			
			for (Word n=0;n<chunks[i].Capacity();++n) {
			
				chunks[i].SetCount(n+1);
				
				chunks[i][n]=b;
				
				if (b==std::numeric_limits<Byte>::max()) b=0;
				else ++b;
			
			}
		
		}
		
		timer.Stop();
		
		StdOut << "Done.  Took " << timer.ElapsedNanoseconds() << "ns" << Newline;
		
		StdOut << Newline << "Saving chunks..." << Newline;
		
		timer=Timer::CreateAndStart();
		
		for (Word x=0;x<x_limit;++x) for (Word y=0;y<y_limit;y++) for (Word z=0;z<z_limit;++z) {
		
			dp->SaveChunk(x,y,z,0,0,chunks[(x*y)+z].begin(),chunks[(x*y)+z].end(),ChunkSaveBegin(),[&] (Int32,Int32,Int32,SByte,bool) {
			
				if (++finished==x_limit*y_limit*z_limit) barrier.Enter();
			
			});
		
		}
		
		barrier.Enter();
		
		timer.Stop();
		
		StdOut << "Done.  Took " << timer.ElapsedNanoseconds() << "ns" << Newline;
	
	} catch (...) {
	
		delete dp;
		
		throw;
	
	}
	
	delete dp;
	
	return EXIT_SUCCESS;

}

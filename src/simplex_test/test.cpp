#include <rleahylib/rleahylib.hpp>
#include <simplex.hpp>
#include <unordered_map>


using namespace MCPP;


UInt64 seed=11653;


int main () {
	
	auto noise=MakeConvert<Word>(
		MakeScale(
			MakeGain(
				MakeOctave(
					Simplex(seed),
					1,
					0.25,
					0.015
				),
				0.95
			),
			64,
			256
		)
	);
	
	for (Word i=0;;++i) {
	
		Timer timer=Timer::CreateAndStart();
	
		auto result=noise(0,i);
		
		timer.Stop();
		
		StdOut << result << " took " << timer.ElapsedNanoseconds() << "ns" << Newline;
		
		Thread::Sleep(75);
	
	}

	return 0;

}

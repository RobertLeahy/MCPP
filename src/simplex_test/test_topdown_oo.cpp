#include <rleahylib/rleahylib.hpp>
#include <noise.hpp>
#include <random.hpp>
#include <cstdio>
#include <stdexcept>
#include <limits>
#include <cmath>
#include <random>
#include <utility>
#include <cstring>
#include <atomic>
#include "terrain_generator.cpp"


using namespace MCPP;


const Word width=25000;
const Word depth=256;
//const Word height=1024;
const Word height=depth;
const SWord z_slice_pos=0;
const Word sample_freq=1;
const Word num_threads=4;
const SWord start_x=-512;
const SWord start_z=-512;
const Nullable<std::mt19937_64::result_type> seed=14454031532601581723ULL;
const String filename("../simplex");


void write_file (FILE * handle, const String & s) {

	auto c_string=s.ToCString();

	if (
		(fputs(
			static_cast<ASCIIChar *>(c_string),
			handle
		)<0) ||
		(fputc(
			'\0',
			handle
		)<0)
	) throw std::runtime_error("Error writing to file");

}


void write_file (FILE * handle, Byte c) {

	if (fputc(
		c,
		handle
	)<0) throw std::runtime_error("Error writing to file");

}


inline Double normalize (Double lo, Double hi, Double val) noexcept {

	if (lo==hi) {
	
		if (val==lo) return 0.5;
		if (val>lo) return 1;
		return 0;
	
	}
	
	bool complement;
	if (lo>hi) {
	
		complement=true;
		std::swap(lo,hi);
	
	} else {
	
		complement=false;
	
	}
	
	Double difference=val-lo;
	Double range=hi-lo;
	
	if (difference<0) return complement ? 1 : 0;
	if (difference>range) return complement ? 0 : 1;
	
	Double result=difference/range;
	
	return complement ? 1-result : result;

}


void execute (std::mt19937_64::result_type seed) {

	std::mt19937_64 gen;
	gen.seed(seed);
	StdOut << "Seed: " << seed << Newline;

	auto c_filename=Path::Combine(
		Path::GetPath(
			File::GetCurrentExecutableFileName()
		),
		filename+"_"+String(seed)+".txt"
	).ToCString();
	
	FILE * fhandle=fopen(
		static_cast<ASCIIChar *>(c_filename),
		"w"
	);
	
	if (fhandle==nullptr) {
	
		StdOut << "Could not open " << filename << " for writing" << Newline;
	
		return;
	
	}
	
	try {
		
		
		TerrainGenerator terrain(gen);
		
		write_file(
			fhandle,
			String(seed)
		);
		write_file(
			fhandle,
			String(sample_freq)
		);
		write_file(
			fhandle,
			String(start_x)
		);
		write_file(
			fhandle,
			String(0)
		);
		write_file(
			fhandle,
			String(width)
		);
		write_file(
			fhandle,
			String(height)
		);
		
		Timer timer(Timer::CreateAndStart());
		Word blocks=0;
		Vector<Byte> buffer(width*depth);
		for (Word x=0;x<width;++x) {
		
			SWord real_x=static_cast<Word>(x*sample_freq)+start_x;
			
			for (Word y=0;y<depth;++y) {
			
				auto noise=terrain(real_x,y,z_slice_pos);
				
				buffer.Add(
					(noise==0)
						?	255
						:	(
								(noise==1)
									?	127
									:	0
							)
				);
				
				++blocks;
			
			}
		
		}
		
		timer.Stop();
		
		StdOut	<< "====GENERATION COMPLETE===="
				<< Newline
				<< "Blocks placed: "
				<< static_cast<Word>(blocks)
				<< Newline
				<< "Elapsed time: "
				<< timer.ElapsedNanoseconds()
				<< "ns"
				<< Newline
				<< "Average time per block: "
				<< (timer.ElapsedNanoseconds()/blocks)
				<< "ns"
				<< Newline;
		
		for (Byte b : buffer) write_file(fhandle,b);
		
	} catch (...) {
	
		fclose(fhandle);
		
		throw;
	
	}
	
	fclose(fhandle);

}


int main () {

	do {
	
		std::mt19937_64::result_type curr_seed=seed.IsNull() ? CryptoRandom<std::mt19937_64::result_type>() : *seed;
	
		execute(curr_seed);
		
		if (seed.IsNull()) {
		
			StdOut << "Waiting for CPU to cool down..." << Newline;
			Thread::Sleep(10000);
		
		}
	
	} while (seed.IsNull());

	return 0;

}

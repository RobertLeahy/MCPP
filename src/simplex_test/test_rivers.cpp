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


using namespace MCPP;


const Word width=4096;
const Word depth=256;
const Word height=1024;
const Word sample_freq=16;
const Word num_threads=4;
const SWord start_x=-512;
const SWord start_z=-512;
const Nullable<std::mt19937_64::result_type> seed=13615544769841768814ULL;
const String filename("../simplex");


enum class Biome : Byte {

	Ocean=0,
	Plains=1,
	Desert=2,
	ExtremeHills=3,
	Forest=4,
	Taiga=5,
	Swamp=6,
	River=7,
	Hell=8,
	Sky=9,
	FrozenOcean=10,
	FrozenRiver=11,
	IcePlains=12,
	IceMountains=13,
	MushroomIsland=14,
	MushroomIslandShore=15,
	Beach=16,
	DesertHills=17,
	ForestHills=18,
	TaigaHills=19,
	ExtremeHillsEdge=20,
	Jungle=21,
	JungleHills=22

};


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

	auto gen=std::mt19937_64();
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
		
		auto noise=MakeConvert<Byte>(
			MakeScale(
				[&gen] (Double x, Double y) -> Double {
				
					static auto noise=MakeRidged(
						MakeOctave(
							Simplex(gen),
							4,
							0.8,//0.7
							0.0001
						)
					);
					
					auto val=noise(x,y);
					
					if (val>=0.9) return 1;
					
					return -1;
				
				},
				2,
				51
			)
		);
		
		String width_str(width);
		String height_str(height);
		
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
			String(start_z)
		);
		write_file(
			fhandle,
			String(width_str)
		);
		write_file(
			fhandle,
			String(height_str)
		);
		
		Vector<Byte> buffer(width*height);
		buffer.SetCount(width*height);
		
		Timer timer(Timer::CreateAndStart());
		
		std::atomic<Word> blocks;
		blocks=0;
		Word offset=0;
		for (Word x=0;x<width;++x) {
		
			SWord real_x=static_cast<SWord>(x*sample_freq)+start_x;
			
			for (Word z=0;z<height;++z) {
			
				buffer[offset++]=noise(
					real_x,
					static_cast<SWord>(z*sample_freq)+start_z
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
		
		offset=0;
		for (Word x=0;x<width;++x)
		for (Word z=0;z<height;++z)
		write_file(fhandle,buffer[(x*height)+z]);
		
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
	
	} while (seed.IsNull());

	return 0;

}

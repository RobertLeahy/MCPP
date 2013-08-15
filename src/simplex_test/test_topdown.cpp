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


const Word width=1024;
const Word depth=256;
const Word height=1024;
const Word sample_freq=16;
const Word num_threads=4;
const SWord start_x=-512;
const SWord start_z=-512;
const Nullable<std::mt19937_64::result_type> seed;//=10564365128032029849ULL;
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
		
		auto ocean=MakeOctave(
			Simplex(gen),
			4,
			0.7,
			0.00005
		);
		
		auto max=MakeScale(
			MakeOctave(
				Simplex(gen),
				/*8,
				0.2,
				0.0005*/
				4,
				0.9,
				0.00005
			),
			0,
			1
		);
		
		auto min=MakeScale(
			MakeBias(
				MakeOctave(
					Simplex(gen),
					4,
					0.5,
					0.00005
				),
				0.25
			),
			0,
			1
		);
		
		auto heightmap=MakeScale(
			MakeOctave(
				Simplex(gen),
				/*8,
				0.3,
				0.005*/
				4,
				0.7,
				0.0005
			),
			0,
			1
		);
		
		auto noise=MakeConvert<Byte>(
			MakePerturbateDomain<1>(
				MakePerturbateDomain<4>(
					[&] (Double x, Double y, Double z) {
						
						auto ocean_val=ocean(x,z);
						
						if (ocean_val<-0.0125) {
						
							//	Ocean
							
							//	To enable a smooth transition
							//	between sea and land, we have a
							//	"continental shelf" which slowly
							//	approaches beach level (i.e. 64)
							//	as we move towards land
							Double dampen=(ocean_val<-0.025) ? 0 : normalize(
								-0.025,
								-0.0125,
								ocean_val
							);
							
							//	Determine the "ceiling"
							//	and "floor" values
							Double ceiling=fma(
								min(x,z),
								16,
								68
							);
							Double floor=fma(
								max(x,z),
								-16,
								48
							);
							if (floor>ceiling) std::swap(floor,ceiling);
							
							//	If we're dampening, apply it
							if (dampen!=0) {
							
								floor+=dampen*(64-floor);
								ceiling+=dampen*(64-ceiling);
							
							}
							
							//	Determine current height
							return (
								(y>fma(
									heightmap(x,z),
									ceiling-floor,
									floor
								))
									//	Air or water?
									?	(
											(y>64)
												//	Air
												?	255
												//	Water
												:	127
										)
									//	Solid
									:	0
							);
						
						} else if (ocean_val<0) {
						
							//	Beach
							
							//	All beaches are at sea level
							return (y>64) ? 255 : 0;
						
						} else {
						
							//	Land
							
							//	We need to dampen the height so
							//	that everything falls off near the
							//	ocean to the beach
							Double dampen=(ocean_val<0.0125) ? normalize(
								0.0125,
								0,
								ocean_val
							) : 0;
							
							//	Determine the "ceiling" and
							//	"floor" values
							Double max_val=max(x,z);
							Double ceiling=(
								(max_val<0.3)
									?	72
									:	(
											(max_val<0.8)
												?	fma(
														normalize(
															0.3,
															0.8,
															max_val
														),
														256-72,
														72
													)
												:	256
										)
							);
							Double min_val=min(x,z);
							Double floor=(
								(min_val<0.2)
									?	fma(
											normalize(
												0,
												0.2,
												min_val
											),
											52-64,
											64
										)
									:	(
											(min_val<0.5)
												?	64
												:	(
														(min_val<0.9)
															?	fma(
																	normalize(
																		0.5,
																		0.9,
																		min_val
																	),
																	128-64,
																	64
																)
															:	128
													)
										)
							);
							if (floor>ceiling) std::swap(floor,ceiling);
							
							//	If we're dampening, apply it
							if (dampen!=0) {
							
								floor+=dampen*(64-floor);
								ceiling+=dampen*(64-ceiling);
							
							}
							
							//	Determine current height
							return (
								(y>fma(
									heightmap(x,z),
									ceiling-floor,
									floor
								))
									//	Air
									?	255
									//	Solid
									:	0
							);
						
						}
					
					},
					MakeScale(
						MakeOctave(
							MakeGain(
								Simplex(gen),
								0.25
							),
							3,
							0.4,
							0.01
						),
						-20,
						20
					)
				),
				MakeScale(
					MakeOctave(
						MakeGain(
							Simplex(gen),
							0.25
						),
						3,
						0.4,
						0.01
					),
					-20,
					20
				)
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
		
		Vector<Byte> buffer(width*height*depth);
		buffer.SetCount(width*height*depth);
		
		Timer timer(Timer::CreateAndStart());
		
		std::atomic<Word> blocks;
		blocks=0;
		Vector<Thread> threads(num_threads);
		for (Word i=0;i<num_threads;++i) {
		
			threads.EmplaceBack(
				[&] (Word num) {
				
					Word lower=(width/num_threads)*num;
					Word upper=(
						(num==(num_threads-1))
							?	width
							:	(width/num_threads)*(num+1)
					);
					
					for (Word x=lower;x<upper;++x) {
					
						SWord real_x=static_cast<SWord>(x*sample_freq)+start_x;
						
						for (Word z=0;z<height;++z) {
						
							SWord real_z=static_cast<SWord>(z*sample_freq)+start_z;
							Word offset=(x*height*256)+(z*256);
							
							for (Word y=256;(y--)>0;) {
							
								auto val=noise(real_x,y,real_z);
								
								buffer[offset+y]=val;
								
								++blocks;
								
								if (val!=255) break;
							
							}
						
						}
					
					}
				
				},
				i
			);
		
		}
		
		for (auto & t : threads) t.Join();
		
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
				
		for (Word x=0;x<width;++x)
		for (Word z=0;z<height;++z)
		for (Word y=255;;--y) {
		
			if (y==0) {
			
				write_file(fhandle,1);
			
				break;
			
			}
			
			Byte val=buffer[(x*height*depth)+(z*depth)+y];
			
			if (val==0) {
			
				write_file(fhandle,static_cast<Byte>(y));
				
				break;
			
			}
			
			if (val==127) {
			
				write_file(fhandle,0);
				
				break;
			
			}
		
		}
		
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

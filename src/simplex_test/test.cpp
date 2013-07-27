#include <rleahylib/rleahylib.hpp>
#include <simplex.hpp>
#include <cstdio>
#include <stdexcept>


using namespace MCPP;


Word width=1024;
Word height=256;
String filename("../simplex.txt");


void write_file (FILE * handle, const String & s) {

	if (fputs(
		static_cast<ASCIIChar *>(s.ToCString()),
		handle
	)<0) throw std::runtime_error("Error writing to file");

}


int main () {

	/*Gradient<2> grad(0,75,0,125);
	for (Word i=0;i<=200;++i) {
	
		StdOut << i << " - " << grad(0,i) << Newline;
	
	}

	return 0;*/

	auto c_filename=Path::Combine(
		Path::GetPath(
			File::GetCurrentExecutableFileName()
		),
		filename	
	).ToCString();
	
	FILE * fhandle=fopen(
		static_cast<ASCIIChar *>(
			Path::Combine(
				Path::GetPath(
					File::GetCurrentExecutableFileName()
				),
				filename	
			).ToCString()
		),
		"w"
	);
	
	if (fhandle==nullptr) {
	
		StdOut << "Could not open " << filename << " for writing" << Newline;
	
		return 1;
	
	}
	
	try {
	
		Gradient<2> grad(0,height/2);
		
		
		Simplex simplex;
		auto noise=MakeConvert<bool>(
			MakeCombine(
				MakeScale(
					MakeRange(
						MakeRidged(
							MakeBias(
								MakeCombine(
									MakeScaleDomain<1>(
										MakeScaleDomain<~static_cast<Word>(0)>(
											simplex,
											0.05
										),
										0.5
									),
									Gradient<2>(0,128,0,32),
									[] (Double x, Double y) -> Double {
									
										return fma(
											fma(
												x,
												0.5,
												0.5
											)*fma(
												y,
												0.5,
												0.5
											),
											2,
											-1
										);
										
									}
								),
								0.4
							)
						),
						[] (Double input) -> Double {
						
							if (input>0.3) return 1;
							
							return -1;
						
						}
					),
					0,
					1
				),
				MakeScale(
					MakePerturbateDomain<1>(
						MakePerturbateDomain<2>(
							MakeRange(
								Gradient<2>(0,height),
								[] (Double input) -> Double {
								
									if (input>0) return 1;
									
									return -1;
								
								}
							),
							MakeScaleDomain<2>(
								MakeScale(
									MakeOctave(
										MakeGain(
											simplex,
											0.4
										),
										8,
										0.3,
										0.005
									),
									-static_cast<Double>(height/2),
									height/2
								),
								0
							)
						),
						MakeScale(
							MakeOctave(
								MakeGain(
									simplex,
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
					0,
					1
				),
				[] (Double x, Double y) -> Double {
				
					return ((x==1) || (y==1)) ? 1 : 0;
				
				}
			)
		);
	
		/*auto noise=MakeConvert<bool>(
			MakeScale(
				MakePerturbateDomain<1>(
					MakePerturbateDomain<2>(
						MakeRange(
							Gradient<2>(0,height),
							[] (Double input) -> Double {
							
								if (input>0) return 1;
								
								return -1;
							
							}
						),
						MakeScaleDomain<2>(
							MakeScale(
								MakeOctave(
									MakeGain(
										simplex,
										0.4
									),
									8,
									0.3,
									0.005
								),
								-static_cast<Double>(height/2),
								height/2
							),
							0
						)
					),
					MakeScale(
						MakeOctave(
							MakeGain(
								simplex,
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
				0,
				1
			)
		);*/
		
		String width_str(width);
		String height_str(height);
		
		write_file(
			fhandle,
			String(width_str.Size())
		);
		write_file(
			fhandle,
			width_str
		);
		write_file(
			fhandle,
			String(height_str.Size())
		);
		write_file(
			fhandle,
			height_str
		);
		
		for (Word x=0;x<width;++x) for (Word y=0;y<height;++y) {
		
			write_file(
				fhandle,
				noise(x,y) ? "F" : "T"
			);
		
		}
		
	} catch (...) {
	
		fclose(fhandle);
		
		throw;
	
	}
	
	fclose(fhandle);

	return 0;

}

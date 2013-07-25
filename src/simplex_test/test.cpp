#include <rleahylib/rleahylib.hpp>
#include <simplex.hpp>
#include <cstdio>
#include <stdexcept>


using namespace MCPP;


Word width=25000;
Word height=256;
String filename("../simplex.txt");


void write_file (FILE * handle, const String & s) {

	if (fputs(
		static_cast<ASCIIChar *>(s.ToCString()),
		handle
	)<0) throw std::runtime_error("Error writing to file");

}


int main () {

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
	
		Simplex simplex;
		auto noise=MakeConvert<bool>(
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
		);
		
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

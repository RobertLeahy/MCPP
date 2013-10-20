#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <cstring>
#include <fstream>


using namespace MCPP;


static const String name("Favicon Loader");
static const Word priority=1;
static const String filename("favicon.png");
static const String no_favicon("{0} could not be opened");
static const String invalid_format("{0} is not a valid PNG file");
static const String wrong_size("{0} must be {1}x{2}");
static const String favicon_key("favicon");
static const Byte png_header []={137,80,78,71,13,10,26,10};
static const Word height=64;
static const Word width=64;


class FaviconLoader : public Module {


	private:
	
	
		enum class Reason {
		
			Success,
			CouldNotOpen,
			Format,
			Size
		
		};
		
		
		static void cleanup () {
		
			Server::Get().Data().DeleteBinary(favicon_key);
		
		}
		
		
		static bool verify (const Byte * begin, const Byte * end) {
		
			for (auto b : png_header) {
			
				if (b!=*begin) return false;
				
				++begin;
			
			}
			
			return true;
		
		}
		
		
		static UInt32 get (const Byte * begin, const Byte *) {
		
			union {
				UInt32 retr;
				Byte buffer [sizeof(UInt32)];
			};
			
			if (Endianness::IsBigEndian<UInt32>()) {
			
				std::memcpy(buffer,begin,sizeof(UInt32));
			
			} else {
			
				for (Word i=0;i<sizeof(UInt32);++i) buffer[i]=begin[sizeof(UInt32)-i-1];
			
			}
			
			return retr;
		
		}
	
	
		//	Reloads favicon.png
		static Reason reload () {
		
			//	Get full path to favicon
			auto c_filename=Path::Combine(
				Path::GetPath(
					File::GetCurrentExecutableFileName()
				),
				filename
			).ToCString();
			
			//	Open a binary stream to read
			//	the favicon
			std::fstream stream(
				static_cast<char *>(c_filename),
				std::ios::in|std::ios::binary
			);
			
			auto & data=Server::Get().Data();
			
			//	If the favicon file could not
			//	be opened, fail out
			if (!stream) {
			
				cleanup();
			
				return Reason::CouldNotOpen;
				
			}
			
			//	This buffer holds the binary
			//	contents of the file
			//
			//	We read -- or try to read -- at
			//	least 24 bytes so we can verify:
			//
			//	1.	That it's a PNG file (from the
			//		first 8 bytes).
			//	2.	Its dimensions (which means we
			//		need to read the length header
			//		for the IHDR chunk, then the
			//		chunk type for the IHDR chunk,
			//		and then the first 8 bytes of
			//		the IHDR chunk).
			Vector<Byte> buffer(24);
			
			//	Read in enough information that we
			//	can determine the size of the image
			stream.read(
				reinterpret_cast<char *>(buffer.end()),
				24
			);
			
			//	If we couldn't read 24 bytes, there's
			//	no way it's a valid PNG file, fail out
			if (stream.gcount()!=24) {
			
				cleanup();
				
				return Reason::Format;
			
			}
			
			buffer.SetCount(24);
			
			//	Verify the header
			if (!verify(buffer.begin(),buffer.begin()+8)) {
			
				cleanup();
				
				return Reason::Format;
			
			}
			
			//	Verify sizes
			if (
				(get(
					buffer.begin()+16,
					buffer.begin()+20
				)!=width) ||
				(get(
					buffer.begin()+20,
					buffer.end()
				)!=height)
			) {
			
				cleanup();
				
				return Reason::Size;
			
			}
			
			//	Extract the entire file
			do {
			
				buffer.SetCapacity();
				
				stream.read(
					reinterpret_cast<char *>(buffer.end()),
					int(SafeWord(buffer.Capacity()-buffer.Count()))
				);
				
				buffer.SetCount(buffer.Count()+stream.gcount());
			
			} while (stream);
			
			//	Store the image in the backing
			//	store
			data.SaveBinary(
				favicon_key,
				buffer.begin(),
				buffer.Count()
			);
			
			//	Success
			return Reason::Success;
		
		}


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			switch (reload()) {
			
				case Reason::Success:
				default:break;
				
				case Reason::CouldNotOpen:
					Server::Get().WriteLog(
						String::Format(
							no_favicon,
							filename
						),
						//	This is just informative, perhaps
						//	the user does not actually want
						//	a favicon
						Service::LogType::Information
					);
					break;
					
				case Reason::Format:
					Server::Get().WriteLog(
						String::Format(
							invalid_format,
							filename
						),
						//	This is an error.  The user
						//	clearly attempted to provide a
						//	favicon, but it's invalid
						Service::LogType::Error
					);
					break;
					
				case Reason::Size:
					Server::Get().WriteLog(
						String::Format(
							wrong_size,
							filename,
							width,
							height
						),
						//	This is an error.  The user
						//	clearly attempted to provide a
						//	favicon, but it's the wrong
						//	size
						Service::LogType::Error
					);
					break;
			
			}
		
		}


};


INSTALL_MODULE(FaviconLoader)

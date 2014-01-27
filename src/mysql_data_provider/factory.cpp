#include <mysql_data_provider/mysql_data_provider.hpp>
#include <fstream>


namespace MCPP {


	//	Name of the configuration file
	static const String filename("mysql.ini");


	static String get_path () {
	
		return Path::Combine(
			Path::GetPath(
				File::GetCurrentExecutableFileName()
			),
			filename
		);
	
	}
	
	
	static Nullable<String> get_file_contents () {
	
		//	Get the filename
		auto c_str=get_path().ToCString();
		
		//	Open a binary stream to read the
		//	file in
		std::fstream stream(c_str.begin(),std::ios::in|std::ios::binary);
		
		Nullable<String> retr;
		
		//	If the file could not be opened, fail
		//	out
		if (!stream) return retr;
		
		//	Extract the entire contents of the file
		Vector<Byte> buffer;
		do {
		
			buffer.SetCapacity();
			
			stream.read(
				reinterpret_cast<char *>(buffer.end()),
				buffer.Capacity()-buffer.Count()
			);
			
			buffer.SetCount(
				static_cast<Word>(
					SafeWord(buffer.Count())+
					SafeWord(stream.gcount())
				)
			);
		
		} while (buffer.Capacity()==buffer.Count());
		
		//	If there was nothing, don't even bother
		if (buffer.Count()==0) return retr;
		
		//	Decode
		retr.Construct(
			UTF8().Decode(
				buffer.begin(),
				buffer.end()
			)
		);
		
		return retr;
	
	}
	
	
	//	Default maximum number of connections
	//	in the connection pool -- 0 which is
	//	unlimited
	static const Word default_pool_max=0;
	
	
	DataProvider * DataProvider::GetDataProvider () {
	
		//	Setup defaults, they'll get replaced
		//	as we parse
		Nullable<String> host;
		Nullable<String> username;
		Nullable<String> password;
		Nullable<String> database;
		Nullable<UInt16> port;
		Word max=default_pool_max;
	
		auto contents=get_file_contents();
		
		if (!contents.IsNull()) {
		
			//	Grab each line
			auto matches=Regex("^.*$",RegexOptions().SetMultiline()).Matches(*contents);
			
			//	Separate out each line, skipping
			//	lines that are comments
			Regex regex("^([^#=][^=]*)=(.*)$");
			
			for (auto & m : matches) {
			
				auto match=regex.Match(m.Value());
				if (match.Success()) {
				
					auto key=match[1].Value().Trim();
					auto value=match[2].Value().Trim();
					
					if (key=="host") host=value;
					else if (key=="username") username=value;
					else if (key=="password") password=value;
					else if (key=="database") database=value;
					else if (key=="port") {
					
						UInt16 temp;
						if (value.ToInteger(&temp)) port=temp;
					
					} else if (key=="pool_max") value.ToInteger(&max);
				
				}
			
			}
		
		}
		
		return new MySQL::DataProvider(
			std::move(host),
			std::move(username),
			std::move(password),
			std::move(database),
			std::move(port),
			max
		);
	
	}


}

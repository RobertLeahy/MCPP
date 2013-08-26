#include <mysql_data_provider/mysql_data_provider.hpp>
#include <cstdio>


namespace MCPP {


	//	Name of the configuration file
	static const String filename("mysql.ini");


	DataProvider * DataProvider::GetDataProvider () {
	
		Nullable<String> username;
		Nullable<String> password;
		Nullable<String> database;
		Nullable<String> host;
		Nullable<UInt16> port;
		
		//	Get absolute path to the configuration
		//	file in a format acceptable for use with
		//	the C Standard Library
		auto c_filename=Path::Combine(
			//	Folder that the current executable
			//	is in
			Path::GetPath(
				File::GetCurrentExecutableFileName()
			),
			//	Filename within that folder of
			//	the configuration file
			filename
		).ToCString();
		
		auto handle=fopen(
			static_cast<ASCIIChar *>(c_filename),
			"r"
		);
		
		if (handle!=nullptr) {
		
			//	File opened successfully
			
			Vector<Byte> buffer;
			
			try {
			
				//	Read file contents into memory
				for (
					int c=fgetc(handle);
					feof(handle)==0;
					c=fgetc(handle)
				) buffer.Add(static_cast<Byte>(c));
				
				//	Grab each line
				auto matches=Regex("^.*$",RegexOptions().SetMultiline()).Matches(
					UTF8().Decode(
						buffer.begin(),
						buffer.end()
					)
				);
				
				//	Separate each line into key
				//	and value, skipping lines that
				//	are comments
				Regex regex("^([^#=][^=]*)=(.*)$");
				
				for (auto & m : matches) {
				
					auto match=regex.Match(m.Value());
					
					if (match.Success()) {
					
						auto key=match[1].Value().Trim();
						auto value=match[2].Value().Trim();
						
						if (key=="username") username=value;
						else if (key=="password") password=value;
						else if (key=="database") database=value;
						else if (key=="host") host=value;
						else if (key=="port") {
						
							UInt16 temp;
							if (value.ToInteger(&temp)) port=temp;
						
						}
					
					}
				
				}
				
			} catch (...) {
			
				fclose(handle);
			
				throw;
			
			}
			
			//	Close the file
			fclose(handle);
		
		}
		
		//	Create MySQLDataProvider and
		//	return
		return new MySQLDataProvider(
			host,
			std::move(port),
			username,
			password,
			database
		);
	
	}


}

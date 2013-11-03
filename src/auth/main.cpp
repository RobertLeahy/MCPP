#include <auth/auth.hpp>
#include <server.hpp>
#include <singleton.hpp>


using namespace MCPP;


namespace MCPP {


	//	Length of Minecraft verify tokens
	static const Word verify_token_length=4;
	//	Key which must be set to enable debugging
	static const String debug_key("auth");
	//	Debug logging templates
	static const String http_request("HTTP GET request => {0}");
	static const String http_response("HTTP response <= {0} - Status {1} - Time elapsed {2}ns - Body: {3}");
	static const String http_response_failed("HTTP response <= {0} internal error after {1}ns");
	//	Module information
	static const Word priority=1;
	static const String name("Authentication API");
	
	
	static bool debug () {
	
		return Server::Get().IsVerbose(debug_key);
	
	}


	Authentication::Authentication () : id_len_generator(15,20), id_generator('!','~') {
	
		ygg.SetDebug(
			[this] (Yggdrasil::Request request) mutable {
			
				if (debug()) Server::Get().WriteLog(
					String::Format(
						http_request,
						request.URL
					),
					Service::LogType::Debug
				);
			
			},
			[this] (Yggdrasil::Response response) mutable {
			
				if (debug()) {
				
					auto & server=Server::Get();
				
					if (response.Status==0) server.WriteLog(
						String::Format(
							http_response_failed,
							response.URL,
							response.Elapsed
						),
						Service::LogType::Debug
					);
					else server.WriteLog(
						String::Format(
							http_response,
							response.URL,
							response.Status,
							response.Elapsed,
							response.Body
						),
						Service::LogType::Debug
					);
				
				}
			
			}
		);
	
	}
	
	
	const RSAKey & Authentication::GetKey () const noexcept {
	
		return key;
	
	}
	
	
	Yggdrasil::Client & Authentication::GetClient () noexcept {
	
		return ygg;
	
	}
	
	
	Vector<Byte> Authentication::GetVerifyToken () {
	
		Vector<Byte> retr(verify_token_length);
		
		for (Word i=0;i<verify_token_length;++i) retr.Add(token_generator());
		
		return retr;
	
	}
	
	
	String Authentication::GetServerID () {
	
		//	Determine how long this
		//	server ID string will be
		Word len=id_len_generator();
		
		//	Generate random ASCII characters
		Vector<Byte> ascii(len);
		
		for (Word i=0;i<len;++i) ascii.Add(id_generator());
		
		//	Decode and return
		return ASCII().Decode(ascii.begin(),ascii.end());
	
	}
	
	
	const String & Authentication::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Authentication::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	//	Does nothing
	void Authentication::Install () {	}
	
	
	static Singleton<Authentication> singleton;
	
	
	Authentication & Authentication::Get () {
	
		return singleton.Get();
	
	}
	
	
	bool Authentication::VerifyTokens (const Vector<Byte> & a, const Vector<Byte> & b) noexcept {
	
		//	Both tokens must be the appropriate
		//	length for Minecraft verify tokens
		if (!(
			(a.Count()==verify_token_length) &&
			(b.Count()==verify_token_length)
		)) return false;
		
		//	Compare each byte of the verify
		//	tokens
		for (Word i=0;i<verify_token_length;++i) if (a[i]!=b[i]) return false;
		
		return true;
	
	}


}


extern "C" {


	Module * Load () {
	
		try {
		
			//	Attempt to retrieve
			return &(Authentication::Get());
		
		} catch (...) {	}
		
		//	Failed
		return nullptr;
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

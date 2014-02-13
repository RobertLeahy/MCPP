#include <auth/auth.hpp>
#include <random_device.hpp>
#include <seed_sequence.hpp>
#include <server.hpp>
#include <singleton.hpp>


using namespace MCPP;


namespace MCPP {


	//	Length of Minecraft verify tokens
	static const Word verify_token_length=4;
	//	Key which must be set to enable debugging
	static const String debug_key("auth");
	//	Debug logging templates
	static const String http_request("HTTP request => {0}");
	static const String http_response("HTTP response <= {0} - Status {1} - Time elapsed {2}ns - Body: {3}");
	static const String http_request_failed("HTTP request => {0} failed after {1}ns");
	static const String http_request_failed_reason(" with reason \"{0}\"");
	//	Module information
	static const Word priority=1;
	static const String name("Authentication API");
	
	
	/*static bool debug () {
	
		return Server::Get().IsVerbose(debug_key);
	
	}*/
	
	
	static std::mt19937 get_mt19937 () {
	
		SeedSequence seq;
		return std::mt19937(seq);
	
	}


	Authentication::Authentication ()
		:	gen(get_mt19937()),
			id_len(UniformIntDistribution<Word>(15,20)),
			id_dist(UniformIntDistribution<Byte>('!','~'))
	{
	
		/*ygg.SetDebug(
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
					
					if (response.Status==0) {
						
						String log(
							String::Format(
								http_request_failed,
								response.URL,
								response.Elapsed
							)
						);
						if (response.Body.Size()!=0) log << String::Format(
							http_request_failed_reason,
							response.Body
						);
						
						server.WriteLog(
							log,
							Service::LogType::Debug
						);
					
					} else {
				
						server.WriteLog(
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
			
			}
		);*/
	
	}
	
	
	const RSAKey & Authentication::GetKey () const noexcept {
	
		return key;
	
	}
	
	
	Yggdrasil::Client & Authentication::GetClient () noexcept {
	
		return ygg;
	
	}
	
	
	Vector<Byte> Authentication::GetVerifyToken () {
	
		static_assert(
			sizeof(std::mt19937::result_type)==verify_token_length,
			"std::mt19937 does not return exactly 4 bytes"
		);
		
		union {
			std::mt19937::result_type in;
			Byte out [verify_token_length];
		};
		in=gen();
		
		Vector<Byte> retr(verify_token_length);
		for (Word i=0;i<verify_token_length;++i) retr.Add(out[i]);
		
		return retr;
	
	}
	
	
	String Authentication::GetServerID () {
	
		//	Determine how long this
		//	server ID string will be
		auto len=id_len(gen);
		
		//	Generate random ASCII characters
		Vector<CodePoint> ascii(len);
		
		for (Word i=0;i<len;++i) ascii.Add(
			static_cast<CodePoint>(
				id_dist(gen)
			)
		);
		
		return ascii;
	
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

#include <sha1.hpp>
#include <url.hpp>
#include <yggdrasil.hpp>
#include <limits>
#include <stdexcept>
#include <utility>


namespace Yggdrasil {


	Error::Error (Word status, String type, String message, Nullable<String> cause) noexcept
		:	Status(status),
			Type(std::move(type)),
			Message(std::move(message)),
			Cause(std::move(cause))
	{	}
	
	
	static const char * ygg_error="Yggdrasil error";
	const char * Error::what () const noexcept {
	
		return ygg_error;
	
	}


	//	Throws an error appropriate for a malformed
	//	response
	static const char * malformed_response="Malformed response";
	[[noreturn]]
	static void malformed () {
	
		throw std::runtime_error(malformed_response);
	
	}
	
	
	//	Gets a value from a JSON object.
	//
	//	Makes no requirements about the value's presence
	//	or type.
	template <typename T>
	Nullable<T> get_opt_value (JSON::Object & obj, const String & key) {
	
		Nullable<T> retr;
	
		if (obj.IsNull()) return retr;
		
		auto & pairs=*obj.Pairs;
		
		auto iter=pairs.find(key);
		if (iter==pairs.end()) return retr;
		
		auto & value=iter->second;
		
		if (!value.Is<T>()) return retr;
		
		retr.Construct(
			std::move(value.Get<T>())
		);
		
		return retr;
	
	}
	
	
	template <>
	Nullable<Word> get_opt_value<Word> (JSON::Object & obj, const String & key) {
	
		auto dbl=get_opt_value<Double>(obj,key);
		
		Nullable<Word> retr;
		
		if (dbl.IsNull()) return retr;
		
		auto wrd=static_cast<Word>(*dbl);
		
		if (static_cast<Double>(wrd)!=dbl) return retr;
		
		retr.Construct(wrd);
		
		return retr;
	
	}
	
	
	//	Gets a value from a JSON object.
	//
	//	Requires that the key be not only defined, but
	//	but of the appropriate type.
	template <typename T>
	T get_value (JSON::Object & obj, const String & key) {
	
		auto retr=get_opt_value<T>(obj,key);
		
		if (retr.IsNull()) malformed();
		
		return std::move(*retr);
	
	}
	
	
	//	Unpacks a JSON object from a JSON value
	static JSON::Object unpack (JSON::Value value) {
	
		if (!value.Is<JSON::Object>()) malformed();
		
		return std::move(value.Get<JSON::Object>());
	
	}
	
	
	//	Extracts a JSON object from an HTTP response
	static const Word max_json_depth=5;
	static JSON::Object get_json (const MCPP::HTTPResponse & response) {
	
		return unpack(
			JSON::Parse(
				response.GetBody(),
				max_json_depth
			)
		);
	
	}
	
	
	//	If a response is a Yggdrasil error, throws,
	//	otherwise returns
	static const String error_key("error");
	static const String error_message_key("errorMessage");
	static const String cause_key("cause");
	static void check (const MCPP::HTTPResponse & response) {
	
		//	2xx is not a Yggdrasil error
		if ((response.Status/100)==2) return;
		
		//	It's a Yggdrasil error, parse
		//	and throw
		auto obj=get_json(response);
		throw Error(
			response.Status,
			get_value<String>(
				obj,
				error_key
			),
			get_value<String>(
				obj,
				error_message_key
			),
			get_opt_value<String>(
				obj,
				cause_key
			)
		);
	
	}
	
	
	//	Converts an integer type to a double
	static const char * int_to_dbl_error="Integer value not representable as a double";
	template <typename T>
	T to_dbl (T in) {
	
		auto dbl=static_cast<Double>(in);
		
		if (static_cast<T>(dbl)!=in) throw std::domain_error(int_to_dbl_error);
		
		return dbl;
	
	}
	
	
	//	Gets an HTTP request
	static const Word max_header_bytes=1024;
	static const Word max_body_bytes=65536;
	static MCPP::HTTPRequest get_request () {
	
		MCPP::HTTPRequest retr;
		retr.RequireSSL=true;
		retr.MaxHeaderBytes=max_header_bytes;
		retr.MaxBodyBytes=max_body_bytes;
		retr.FollowRedirects=false;
		
		return retr;
	
	}
	
	
	static const String content_type_key("Content-Type");
	static const String content_type_value("application/json");
	static MCPP::HTTPRequest get_request (const JSON::Value & value) {
	
		auto retr=get_request();
		retr.Body=UTF8().Encode(
			JSON::Serialize(value)
		);
		retr.Verb=MCPP::HTTPVerb::POST;
		retr.Headers.Add({
			content_type_key,
			content_type_value
		});
		
		return retr;
	
	}
	
	
	static const String url("https://authserver.mojang.com/{0}");
	static MCPP::HTTPRequest get_request (const String & endpoint, const JSON::Value & value) {
	
		auto retr=get_request(value);
		retr.URL=String::Format(
			url,
			endpoint
		);
		
		return retr;
	
	}
	
	
	static const String agent_name_key("name");
	static const String agent_version_key("version");
	JSON::Object Agent::ToJSON () {
	
		JSON::Object retr;
		retr.Add(
			agent_name_key,
			std::move(Name),
			agent_version_key,
			to_dbl(Version)
		);
		
		return retr;
	
	}
	
	
	static const String profile_id_key("id");
	static const String profile_name_key("name");
	Profile::Profile (JSON::Object obj)
		:	ID(get_value<String>(obj,profile_id_key)),
			Name(get_value<String>(obj,profile_name_key))
	{	}
	
	
	JSON::Object Profile::ToJSON () {
	
		JSON::Object retr;
		retr.Add(
			profile_id_key,
			std::move(ID),
			profile_name_key,
			std::move(Name)
		);
		
		return retr;
	
	}
	
	
	static const String auth_a_token_key("accessToken");
	static const String auth_c_token_key("clientToken");
	static const String auth_profiles_key("availableProfiles");
	static const String auth_profile_key("selectedProfile");
	AuthenticateResult::AuthenticateResult (JSON::Object obj)
		:	AccessToken(get_value<String>(obj,auth_a_token_key)),
			ClientToken(get_value<String>(obj,auth_c_token_key))
	{
	
		auto avail=get_opt_value<JSON::Array>(obj,auth_profiles_key);
		if (!avail.IsNull()) {
		
			AvailableProfiles.Construct(
				avail->Values.Count()
			);
			
			for (auto & p : avail->Values) AvailableProfiles->EmplaceBack(
				unpack(std::move(p))
			);
		
		}
		
		auto selected=get_opt_value<JSON::Object>(obj,auth_profile_key);
		if (!selected.IsNull()) SelectedProfile.Construct(
			std::move(*selected)
		);
	
	}
	
	
	static const String refresh_a_token_key("accessToken");
	static const String refresh_c_token_key("clientToken");
	static const String refresh_profile_key("selectedProfile");
	RefreshResult::RefreshResult (JSON::Object obj)
		:	AccessToken(get_value<String>(obj,refresh_a_token_key)),
			ClientToken(get_value<String>(obj,refresh_c_token_key))
	{
	
		auto profile=get_opt_value<JSON::Object>(obj,refresh_profile_key);
		if (!profile.IsNull()) profile.Construct(std::move(*profile));
	
	}
	
	
	//	Dispatches HTTP requests
	template <typename T, typename Callback, typename... Args>
	typename std::enable_if<
		!std::is_same<
			typename std::decay<Callback>::type,
			JSON::Object
		>::value,
		Promise<T>
	>::type Client::dispatch (MCPP::HTTPRequest request, Callback && callback, Args &&... args) {
	
		return http.Execute(std::move(request)).Then(
			std::forward<Callback>(callback),
			std::forward<Args>(args)...
		);
	
	}
	
	
	template <typename T>
	Promise<T> Client::dispatch (MCPP::HTTPRequest request) {
	
		return dispatch<T>(
			std::move(request),
			[] (Promise<MCPP::HTTPResponse> p) {
		
				auto response=p.Get();
				check(response);
				
				return T(get_json(response));
				
			}
		);
	
	}
	
	
	template <>
	Promise<void> Client::dispatch (MCPP::HTTPRequest request) {
	
		return dispatch<void>(
			std::move(request),
			[] (Promise<MCPP::HTTPResponse> p) {
			
				auto response=p.Get();
				check(response);
			
			}
		);
	
	}
	
	
	template <typename T>
	Promise<T> Client::dispatch (const String & endpoint, const JSON::Value & value) {
	
		return dispatch<T>(get_request(endpoint,value));
	
	}
	
	
	static const String auth_username_key("username");
	static const String auth_password_key("password");
	static const String auth_client_token_key("clientToken");
	static const String auth_agent_key("agent");
	static const String auth_endpoint("authenticate");
	Promise<AuthenticateResult> Client::Authenticate (
		String username,
		String password,
		Nullable<String> client_token,
		Nullable<Agent> agent
	) {
	
		//	Create JSON request
		JSON::Object obj;
		obj.Add(
			auth_username_key,
			std::move(username),
			auth_password_key,
			std::move(password)
		);
		if (!client_token.IsNull()) obj.Add(
			auth_client_token_key,
			std::move(*client_token)
		);
		if (!agent.IsNull()) obj.Add(
			auth_agent_key,
			agent->ToJSON()
		);
		
		//	Dispatch request
		return dispatch<AuthenticateResult>(
			auth_endpoint,
			std::move(obj)
		);
	
	}
	
	
	static const String refresh_token_key("accessToken");
	static const String refresh_client_token_key("clientToken");
	static const String refresh_endpoint("refresh");
	Promise<RefreshResult> Client::Refresh (
		String access_token,
		String client_token,
		Nullable<Profile> profile
	) {
	
		//	Create JSON request
		JSON::Object obj;
		obj.Add(
			refresh_token_key,
			std::move(access_token),
			refresh_client_token_key,
			std::move(client_token)
		);
		if (!profile.IsNull()) obj.Add(
			refresh_profile_key,
			profile->ToJSON()
		);
		
		//	Dispatch request
		return dispatch<RefreshResult>(
			refresh_endpoint,
			std::move(obj)
		);
	
	}
	
	
	static const String validate_token_key("accessToken");
	static const String validate_endpoint("validate");
	//	How stupid is this?  I'm doing string comparisons
	//	on error messages intended to be INFORMATIVE
	//	to see if some absolutely expected condition
	//	occurred.  Someone needs to give Mojang a lesson
	//	on exceptions/errors and how to use them.  An
	//	invalid token is NOT an error when calling an
	//	endpoint whose raison d'etre is TO CHECK WHETHER
	//	THINGS ARE VALID OR NOT.
	static const String invalid_token_type("ForbiddenOperationException");
	static const String invalid_token_message("Invalid token");
	Promise<bool> Client::Validate (String access_token) {
	
		//	Create JSON request
		JSON::Object obj;
		obj.Add(
			validate_token_key,
			std::move(access_token)
		);
		
		//	Dispatch request
		return http.Execute(
			get_request(
				validate_endpoint,
				std::move(obj)
			)
		).Then([] (Promise<MCPP::HTTPResponse> p) {
		
			auto response=p.Get();
			try {
			
				check(response);
			
			//	We need to catch the ERROR to see whether
			//	it's JUST RETURNING FALSE ARE YOU KIDDING
			//	ME?
			} catch (const Error & e) {
			
				if (
					(e.Type==invalid_token_type) &&
					(e.Message==invalid_token_message)
				) return false;
				
				throw;
			
			}
			
			return true;
		
		});
	
	}
	
	
	static const String signout_username_key("username");
	static const String signout_password_key("password");
	static const String signout_endpoint("signout");
	Promise<void> Client::Signout (String username, String password) {
	
		//	Create JSON request
		JSON::Object obj;
		obj.Add(
			signout_username_key,
			std::move(username),
			signout_password_key,
			std::move(password)
		);
		
		//	Dispatch
		return dispatch<void>(
			signout_endpoint,
			std::move(obj)
		);
	
	}
	
	
	static const String invalidate_token_key("accessToken");
	static const String invalidate_client_token_key("clientToken");
	static const String invalidate_endpoint("invalidate");
	Promise<void> Client::Invalidate (String access_token, String client_token) {
	
		//	Create JSON request
		JSON::Object obj;
		obj.Add(
			invalidate_token_key,
			std::move(access_token),
			invalidate_client_token_key,
			std::move(client_token)
		);
		
		//	Dispatch
		return dispatch<void>(
			invalidate_endpoint,
			std::move(obj)
		);
	
	}
	
	
	static String get_hash (const String & server_id, const Vector<Byte> & secret, const Vector<Byte> & public_key) {
	
		MCPP::SHA1 hash;
		hash.Update(ASCII().Encode(server_id));
		hash.Update(secret);
		hash.Update(public_key);
		
		return hash.HexDigest();
	
	}
	
	
	static const String session_url("https://sessionserver.mojang.com/session/minecraft/{0}");
	
	
	static const String session_token_key("accessToken");
	static const String session_profile_key("selectedProfile");
	static const String session_server_id_key("serverId");
	static const String client_session_endpoint("join");
	Promise<void> Client::ClientSession (
		String access_token,
		String profile_id,
		String server_id,
		const Vector<Byte> & secret,
		const Vector<Byte> & public_key
	) {
		
		//	Create JSON request
		JSON::Object obj;
		obj.Add(
			session_token_key,
			std::move(access_token),
			session_profile_key,
			std::move(profile_id),
			session_server_id_key,
			get_hash(server_id,secret,public_key)
		);
		
		//	We have to use a custom URL
		auto request=get_request(std::move(obj));
		request.URL=String::Format(
			session_url,
			client_session_endpoint
		);
		
		//	Dispatch
		return dispatch<void>(std::move(request));
	
	}
	
	
	static const String session_id_key("id");
	static const String server_endpoint("hasJoined?username={0}&serverId={1}");
	static const char * session_fail("Session verification failed");
	Promise<String> Client::ServerSession (
		const String & username,
		const String & server_id,
		const Vector<Byte> & secret,
		const Vector<Byte> & public_key
	) {
	
		//	Building a custom GET request
		auto request=get_request();
		request.URL=String::Format(
			session_url,
			String::Format(
				server_endpoint,
				MCPP::URL::Encode(username),
				MCPP::URL::Encode(
					get_hash(server_id,secret,public_key)
				)
			)
		);
		
		//	Dispatch
		return http.Execute(
			std::move(request)
		).Then([] (Promise<MCPP::HTTPResponse> p) {
		
			auto response=p.Get();
			check(response);
			
			//	If the server returned nothing,
			//	that's its way of letting us now
			//	we failed
			if (response.Body.Count()==0) throw std::runtime_error(session_fail);
			
			auto obj=get_json(response);
			return get_value<String>(
				obj,
				session_id_key
			);
		
		});
	
	}


}

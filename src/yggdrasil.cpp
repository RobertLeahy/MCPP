#include <json.hpp>
#include <sha1.hpp>
#include <url.hpp>
#include <yggdrasil.hpp>
#include <utility>


using namespace MCPP;


static const String content_type("application/json");
static const String url("https://authserver.mojang.com/");
static const String client_url("http://session.minecraft.net/game/joinserver.jsp?user={0}&sessionId={1}&serverId={2}");
static const String client_good("OK");
static const String server_url("https://sessionserver.mojang.com/session/minecraft/hasJoined?username={0}&serverId={1}");
static const String token_template("token:{0}:{1}");
static const String auth_ep("authenticate");
static const String refresh_ep("refresh");
static const String validate_ep("validate");
static const String signout_ep("signout");
static const String invalidate_ep("invalidate");
static const Word max_recurse=5;
static const Word max_payload=1024;


namespace Yggdrasil {


	Client::Client () : handler(max_payload) {	}
	
	
	void Client::send_post (String url, String body, std::function<void (Word, String)> callback) {
	
		Request event{
			true,
			std::move(url),
			std::move(body)
		};
		
		if (request) try {
		
			request(event);
		
		} catch (...) {	}
		
		Timer timer(Timer::CreateAndStart());
		
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		handler.Post(
			event.URL,
			content_type,
			*event.Body,
			[this,callback=std::move(callback),url=event.URL,timer] (Word status, String response) mutable {
			
				auto elapsed=timer.ElapsedNanoseconds();
			
				Response event{
					status,
					std::move(url),
					std::move(response),
					elapsed
				};
				
				if (this->response) try {
				
					this->response(event);
				
				} catch (...) {	}
				
				callback(
					status,
					std::move(event.Body)
				);
			
			}
		);
		#pragma GCC diagnostic pop
	
	}
	
	
	void Client::send_get (String url, std::function<void (Word, String)> callback) {
	
		Request event{
			false,
			std::move(url),
			Nullable<String>()
		};
		
		if (request) try {
		
			request(event);
		
		} catch (...) {	}
		
		Timer timer(Timer::CreateAndStart());
		
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		handler.Get(
			event.URL,
			[this,callback=std::move(callback),url=event.URL,timer] (Word status, String response) mutable {
			
				auto elapsed=timer.ElapsedNanoseconds();
			
				Response event{
					status,
					std::move(url),
					std::move(response),
					elapsed
				};
				
				if (this->response) try {
				
					this->response(event);
				
				} catch (...) {	}
				
				callback(
					status,
					std::move(event.Body)
				);
			
			}
		);
		#pragma GCC diagnostic pop
	
	}


	[[noreturn]]
	static void raise () {
	
		throw 0;
	
	}
	
	
	static void check_obj (const JSON::Object & obj) {
	
		if (!obj.Pairs) raise();
	
	}
	
	
	template <typename T>
	T get (const String & key, const JSON::Object & obj) {
	
		auto iter=obj.Pairs->find(key);
		if (
			(iter==obj.Pairs->end()) ||
			(!iter->second.Is<T>())
		) raise();
		
		return std::move(iter->second.Get<T>());
	
	}
	
	
	template <>
	JSON::Object get<JSON::Object> (const String & key, const JSON::Object & obj) {
	
		auto iter=obj.Pairs->find(key);
		if (
			(iter==obj.Pairs->end()) ||
			(!iter->second.Is<JSON::Object>())
		) raise();
		
		auto & retr=iter->second.Get<JSON::Object>();
		
		check_obj(retr);
		
		return std::move(retr);
	
	}


	static JSON::Object parse (const String & json) {
	
		auto retr=JSON::Parse(json,max_recurse);
		
		if (!retr.Is<JSON::Object>()) raise();
		
		auto & obj=retr.Get<JSON::Object>();
		
		check_obj(obj);
		
		return std::move(obj);
	
	}
	
	
	static Error parse_error (Word status, const String & json) {
	
		auto obj=parse(json);
		
		Error retr;
		retr.Status=status;
		retr.Error=get<String>("error",obj);
		retr.ErrorMessage=get<String>("errorMessage",obj);
		
		auto cause=obj.Pairs->find("cause");
		if (cause!=obj.Pairs->end()) {
		
			if (!cause->second.Is<String>()) raise();
			
			retr.Cause.Construct(
				std::move(cause->second.Get<String>())
			);
		
		}
		
		return retr;
	
	}
	
	
	static Profile get_profile (const JSON::Object & obj) {
	
		Profile retr;
		retr.ID=get<String>("id",obj);
		retr.Name=get<String>("name",obj);
		
		return retr;
	
	}
	
	
	static AuthenticateResult parse_authenticate (const String & json) {
	
		auto obj=parse(json);
		
		AuthenticateResult retr;
		retr.AccessToken=get<String>("accessToken",obj);
		retr.ClientToken=get<String>("clientToken",obj);
		
		auto profiles=obj.Pairs->find("availableProfiles");
		if (profiles!=obj.Pairs->end()) {
		
			if (!profiles->second.Is<JSON::Array>()) raise();
			
			auto & arr=profiles->second.Get<JSON::Array>().Values;
			
			retr.AvailableProfiles.Construct(arr.Count());
			
			for (auto & value : arr) {
			
				if (!value.Is<JSON::Object>()) raise();
				
				auto & obj=value.Get<JSON::Object>();
				
				check_obj(obj);
				
				retr.AvailableProfiles->Add(get_profile(obj));
			
			}
		
		}
		
		auto selected=obj.Pairs->find("selectedProfile");
		if (selected!=obj.Pairs->end()) {
		
			if (!selected->second.Is<JSON::Object>()) raise();
			
			retr.SelectedProfile.Construct(
				get_profile(selected->second.Get<JSON::Object>())
			);
		
		}
		
		return retr;
	
	}


	void Client::Authenticate (
		String username,
		String password,
		AuthenticateCallback callback,
		Nullable<String> client_token,
		Nullable<Agent> agent
	) {
	
		//	Create JSON request
		
		JSON::Object root;
		
		//	Add username and password
		root.Add(
			"username",std::move(username),
			"password",std::move(password)
		);
		
		//	Add agent (if supplied)
		if (!agent.IsNull()) {
		
			JSON::Object a;
			a.Add(
				"name",std::move(agent->Name),
				"version",agent->Version
			);
			
			root.Add("agent",std::move(a));
		
		}
		
		//	Add client taken (if supplied)
		if (!client_token.IsNull()) root.Add(
			"clientToken",std::move(*client_token)
		);
		
		//	Make request
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		send_post(
			url+auth_ep,
			JSON::Serialize(std::move(root)),
			[callback=std::move(callback)] (Word status, String response) {
			
				AuthenticateType result;
			
				//	Attempt to parse JSON if it's
				//	not a complete failure
				if (status!=0) try {
				
					if (status==200) result=parse_authenticate(response);
					else result=parse_error(status,response);
				
				} catch (...) {	}
				
				callback(std::move(result));
				
			}
		);
		#pragma GCC diagnostic pop
	
	}
	
	
	static RefreshResult parse_refresh (const String & json) {
	
		auto obj=parse(json);
		
		return RefreshResult{
			get<String>("accessToken",obj),
			get<String>("clientToken",obj),
			get_profile(obj)
		};
	
	}
	
	
	void Client::Refresh (
		String access_token,
		String client_token,
		RefreshCallback callback,
		Nullable<Profile> selected_profile
	) {
	
		//	Create JSON request
		
		JSON::Object root;
		
		//	Add tokens
		root.Add(
			"accessToken",std::move(access_token),
			"clientToken",std::move(client_token)
		);
		
		//	Optionally add the selected
		//	profile
		if (!selected_profile.IsNull()) {
		
			JSON::Object sp;
			sp.Add(
				"id",std::move(selected_profile->ID),
				"name",std::move(selected_profile->Name)
			);
			
			root.Add(
				"selectedProfile",std::move(sp)
			);
		
		}
		
		//	Make request
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		send_post(
			url+refresh_ep,
			JSON::Serialize(std::move(root)),
			[callback=std::move(callback)] (Word status, String response) {
			
				RefreshType result;
				
				//	Attempt to parse JSON if the
				//	request didn't completely fail
				if (status!=0) try {
				
					if (status==200) result=parse_refresh(response);
					else result=parse_error(status,response);
				
				} catch (...) {	}
				
				callback(std::move(result));
			
			}
		);
		#pragma GCC diagnostic pop
	
	}
	
	
	void Client::Validate (String access_token, ValidateCallback callback) {
	
		//	Create JSON request
		
		JSON::Object root;
		
		root.Add(
			"accessToken",std::move(access_token)
		);
		
		//	Make request
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		send_post(
			url+validate_ep,
			JSON::Serialize(std::move(root)),
			[callback=std::move(callback)] (Word status, String response) {
			
				BooleanType result;
				
				//	Attempt to process response if
				//	the request didn't hard fail
				if (status!=0) try {
				
					if (status==200) {
					
						if (response.Size()==0) result=true;
					
					} else {
					
						result=parse_error(status,response);
					
					}
				
				} catch (...) {	}
				
				callback(std::move(result));
			
			}
		);
		#pragma GCC diagnostic pop
	
	}
	
	
	void Client::Signout (String username, String password, SignoutCallback callback) {
	
		//	Create JSON request
		
		JSON::Object root;
		
		root.Add(
			"username",std::move(username),
			"password",std::move(password)
		);
		
		//	Make request
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		send_post(
			url+signout_ep,
			JSON::Serialize(std::move(root)),
			[callback=std::move(callback)] (Word status, String response) {
			
				BooleanType result;
				
				//	Attempt to process response if
				//	the request didn't hard fail
				if (status!=0) try {
				
					if (status==200) {
					
						if (response.Size()==0) result=true;
					
					} else {
					
						result=parse_error(status,response);
					
					}
				
				} catch (...) {	}
				
				callback(std::move(result));
			
			}
		);
		#pragma GCC diagnostic pop
	
	}
	
	
	void Client::Invalidate (String access_token, String client_token, InvalidateCallback callback) {
	
		//	Create JSON request
		
		JSON::Object root;
		
		root.Add(
			"accessToken",std::move(access_token),
			"clientToken",std::move(client_token)
		);
		
		//	Make request
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		send_post(
			url+invalidate_ep,
			JSON::Serialize(std::move(root)),
			[callback=std::move(callback)] (Word status, String response) {
			
				BooleanType result;
				
				//	Attempt to process response if
				//	the request didn't hard fail
				if (status!=0) try {
				
					if (status==200) {
					
						if (response.Size()==0) result=true;
					
					} else {
					
						result=parse_error(status,response);
					
					}
				
				} catch (...) {	}
				
				callback(std::move(result));
			
			}
		);
		#pragma GCC diagnostic pop
	
	}
	
	
	static String get_hash (const String & server_id, const Vector<Byte> & secret, const Vector<Byte> & public_key) {
	
		SHA1 hash;
		hash.Update(ASCII().Encode(server_id));
		hash.Update(secret);
		hash.Update(public_key);
		
		return hash.HexDigest();
	
	}
	
	
	static String get_token (const String & access_token, const String & profile_id) {
	
		return String::Format(
			token_template,
			access_token,
			profile_id
		);
	
	}
	
	
	void Client::ClientSession (
		const String & username,
		const String & access_token,
		const String & profile_id,
		const String & server_id,
		const Vector<Byte> & secret,
		const Vector<Byte> & public_key,
		ClientSessionCallback callback
	) {
	
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		send_get(
			String::Format(
				client_url,
				URL::Encode(username),
				URL::Encode(get_token(access_token,profile_id)),
				URL::Encode(get_hash(server_id,secret,public_key))
			),
			[callback=std::move(callback)] (Word status, String response) {
			
				bool result=false;
			
				try {
				
					if ((status==200) && (response==client_good)) result=true;
				
				} catch (...) {	}
				
				callback(result);
			
			}
		);
		#pragma GCC diagnostic pop
	
	}
	
	
	void Client::ServerSession (
		const String & username,
		const String & server_id,
		const Vector<Byte> & secret,
		const Vector<Byte> & public_key,
		ServerSessionCallback callback
	) {
	
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		send_get(
			String::Format(
				server_url,
				URL::Encode(username),
				URL::Encode(get_hash(server_id,secret,public_key))
			),
			[callback=std::move(callback)] (Word status, String response) {
			
				Nullable<String> result;
				
				try {
				
					if (status==200) {
					
						//	Parse JSON
						auto obj=parse(response);
						
						//	Ensure JSON structure is valid and
						//	extract user's UUID
						if (obj.Pairs) {
						
							auto iter=obj.Pairs->find("id");
							
							if (
								(iter!=obj.Pairs->end()) &&
								(iter->second.Is<String>())
							) result.Construct(
								std::move(iter->second.Get<String>())
							);
						
						}
					
					}
				
				} catch (...) {	}
				
				callback(std::move(result));
			
			}
		);
		#pragma GCC diagnostic pop
	
	}
	
	
	void Client::SetDebug (RequestCallback request, ResponseCallback response) {
		
		this->request=std::move(request);
		this->response=std::move(response);
	
	}


}

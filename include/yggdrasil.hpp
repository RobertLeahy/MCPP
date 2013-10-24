/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <http_handler.hpp>
#include <variant.hpp>
#include <functional>


namespace Yggdrasil {


	/**
	 *	\cond
	 */
	 
	 
	template <typename... Args>
	using Variant=MCPP::Variant<Args...>;
	 
	 
	/**
	 *	\endcond
	 */


	/**
	 *	Information about a profile.
	 */
	class Profile {
	
	
		public:
		
		
			/**
			 *	The profile's ID.
			 */
			String ID;
			/**
			 *	The profile's name.
			 */
			String Name;
	
	
	};
	
	
	/**
	 *	The result of a successful authentication.
	 */
	class AuthenticateResult {
	
	
		public:
		
		
			/**
			 *	The access token.
			 */
			String AccessToken;
			/**
			 *	The client token.
			 */
			String ClientToken;
			/**
			 *	An optional collection of profiles
			 *	associated with the authenticated
			 *	account.
			 */
			Nullable<Vector<Profile>> AvailableProfiles;
			/**
			 *	An optional profile selected during
			 *	the authentication process.
			 */
			Nullable<Profile> SelectedProfile;
	
	
	};
	
	
	/**
	 *	The result of successfully refreshing an
	 *	access token.
	 */
	class RefreshResult {
	
	
		public:
		
		
			/**
			 *	The access token.
			 */
			String AccessToken;
			/**
			 *	The client token.
			 */
			String ClientToken;
			/**
			 *	The selected profile.
			 */
			Profile SelectedProfile;
	
	
	};
	
	
	/**
	 *	Represents an agent.
	 */
	class Agent {
	
	
		public:
		
		
			/**
			 *	The name of the agent.
			 */
			String Name;
			/**
			 *	The agent's version.
			 */
			Word Version;
	
	
	};
	
	
	/**
	 *	Encapsulates an error returned by
	 *	the Yggdrasil API.
	 */
	class Error {
	
	
		public:
		
		
			/**
			 *	The HTTP status code associated
			 *	with this error.
			 */
			Word Status;
			/**
			 *	A string which describes this error.
			 */
			String Error;
			/**
			 *	A human readable string which describes
			 *	this error.
			 */
			String ErrorMessage;
			/**
			 *	An optional string which identifies
			 *	the cause of this error.
			 */
			Nullable<String> Cause;
	
	
	};
	 
	 
	class Request {
	
	
		public:
		
		
			bool Post;
			String URL;
			Nullable<String> Body;
	
	
	};
	
	
	class Response {
	
	
		public:
		
		
			Word Status;
			String URL;
			String Body;
			UInt64 Elapsed;
	
	
	};
	
	
	/**
	 *	\cond
	 */
	 
	 
	template <typename T>
	using Type=Variant<T,Error>;
	
	
	/**
	 *	\endcond
	 */
	
	
	typedef Type<AuthenticateResult> AuthenticateType;
	typedef Type<RefreshResult> RefreshType;
	typedef Type<bool> BooleanType;
	typedef std::function<void (AuthenticateType)> AuthenticateCallback;
	typedef std::function<void (RefreshType)> RefreshCallback;
	typedef std::function<void (BooleanType)> ValidateCallback;
	typedef std::function<void (BooleanType)> SignoutCallback;
	typedef std::function<void (BooleanType)> InvalidateCallback;
	typedef std::function<void (bool)> SessionCallback;
	typedef std::function<void (const Request &)> RequestCallback;
	typedef std::function<void (const Response &)> ResponseCallback;


	/**
	 *	Makes requests of the Yggdrasil and Minecraft
	 *	session APIs.
	 */
	class Client {
	
	
		private:
		
		
			MCPP::HTTPHandler handler;
			RequestCallback request;
			ResponseCallback response;
			
			
			void send_post (String, String, std::function<void (Word, String)>);
			void send_get (String, std::function<void (Word, String)>);
			
			
		public:
		
		
			/**
			 *	Creates a new Yggdrasil client.
			 */
			Client ();
		
		
			/**
			 *	Authenticates a user with Yggdrasil.
			 *
			 *	\param [in] username
			 *		The username.
			 *	\param [in] password
			 *		The password.
			 *	\param [in] callback
			 *		The function which shall be asynchronously
			 *		invoked when the request completes.
			 *	\param [in] client_token
			 *		The client token to supply for the request.
			 *		Defaults to null.
			 *	\param [in] agent
			 *		The agent making this request.  Defaults to
			 *		null.
			 */
			void Authenticate (
				String username,
				String password,
				AuthenticateCallback callback,
				Nullable<String> client_token=Nullable<String>(),
				Nullable<Agent> agent=Nullable<Agent>()
			);
			
			
			/**
			 *	Refreshes a Yggdrasil session.
			 *
			 *	\param [in] access_token
			 *		The access token to refresh.
			 *	\param [in] client_token
			 *		The client token to refresh.
			 *	\param [in] callback
			 *		The function which shall be asynchronously
			 *		invoked when the request completes.
			 *	\param [in] selected_profile
			 *		The profile to supply for this request.
			 *		Defaults to null.
			 */
			void Refresh (
				String access_token,
				String client_token,
				RefreshCallback callback,
				Nullable<Profile> selected_profile=Nullable<Profile>()
			);
			
			
			/**
			 *	Checks to see if an access token is
			 *	valid.
			 *
			 *	\param [in] access_token
			 *		The access token to check.
			 *	\param [in] callback
			 *		The function which shall be asynchronously
			 *		invoked when the request completes.
			 */
			void Validate (String access_token, ValidateCallback callback);
			
			
			/**
			 *	Signs a user out, invalidating all access
			 *	tokens associated with them.
			 *
			 *	\param [in] username
			 *		The username.
			 *	\param [in] password
			 *		The password.
			 *	\param [in] callback
			 *		The function which shall be asynchronously
			 *		invoked when the request completes.
			 */
			void Signout (String username, String password, SignoutCallback callback);
			
			
			/**
			 *	Invalidates an access token.
			 *
			 *	\param [in] access_token
			 *		The access token.
			 *	\param [in] client_token
			 *		The client token.
			 *	\param [in] callback
			 *		The function which shall be asynchronously
			 *		invoked when the request completes.
			 */
			void Invalidate (String access_token, String client_token, InvalidateCallback callback);
			
			
			/**
			 *	Creates a session for a client logging into
			 *	a server.
			 *
			 *	\param [in] username
			 *		The username.
			 *	\param [in] access_token
			 *		The access token.
			 *	\param [in] profile_id
			 *		The profile ID.
			 *	\param [in] server_id
			 *		The server ID string supplied by the
			 *		server.  Must be ASCII representable.
			 *	\param [in] secret
			 *		The shared secret.
			 *	\param [in] public_key
			 *		The ASN.1 DER-encoded representation
			 *		of the server's public key.
			 *	\param [in] callback
			 *		The callback which shall be asynchronously
			 *		invoked when the request completes.
			 */
			void ClientSession (
				const String & username,
				const String & access_token,
				const String & profile_id,
				const String & server_id,
				const Vector<Byte> & secret,
				const Vector<Byte> & public_key,
				SessionCallback callback
			);
			
			
			/**
			 *	Validates a user logging into a server.
			 *
			 *	\param [in] username
			 *		The username of the user logging in.
			 *	\param [in] server_id
			 *		The server ID string.
			 *	\param [in] secret
			 *		The shared secret.
			 *	\param [in] secret
			 *		The ASN.1 DER-encoded representation
			 *		of the server's public key.
			 *	\param [in] callback
			 *		The callback which shall be asynchronously
			 *		invoked when the request completes.
			 */
			void ServerSession (
				const String & username,
				const String & server_id,
				const Vector<Byte> & secret,
				const Vector<Byte> & public_key,
				SessionCallback callback
			);
			
			
			/**
			 *	Set debugging functions which will be invoked
			 *	before each request is made, and after its
			 *	response is received or it fails.
			 *
			 *	\param [in] request
			 *		The callback to invoke when a request is
			 *		sent.
			 *	\param [in] response
			 *		The callback to invoke when a response is
			 *		received or a request fails.
			 */
			void SetDebug (RequestCallback request=RequestCallback(), ResponseCallback response=ResponseCallback());
	
	
	};


}

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <http_handler.hpp>
#include <json.hpp>
#include <promise.hpp>
#include <exception>
#include <type_traits>


namespace Yggdrasil {


	template <typename T>
	using Promise=MCPP::Promise<T>;
	
	
	/**
	 *	Sent during the authentication process to tell
	 *	Mojang which kind of agent is logging in on
	 *	the user's behalf.
	 */
	class Agent {
	
	
		public:
		
		
			/**
			 *	The name of this agent.
			 *
			 *	Only known valid value is \"Minecraft\".
			 */
			String Name;
			/**
			 *	The version of this agent.
			 *
			 *	Only known valid value is 1.
			 */
			Word Version;
			
			
			/**
			 *	\cond
			 */
			
			
			JSON::Object ToJSON ();
			
			
			/**
			 *	\endcond
			 */
		
	
	};
	
	
	/**
	 *	A profile associated with a user's account.
	 *
	 *	Received from Mojang when authentication is
	 *	successful.
	 */
	class Profile {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			Profile (JSON::Object);
			JSON::Object ToJSON ();
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	The ID of this profile.
			 */
			String ID;
			/**
			 *	The name of this profile.
			 */
			String Name;
	
	
	};
	
	
	/**
	 *	The result of authenticating through Yggdrasil.
	 */
	class AuthenticateResult {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			AuthenticateResult (JSON::Object);
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	The access token.
			 */
			String AccessToken;
			/**
			 *	The client token.  This is the same
			 *	token that was sent, if one was sent.
			 */
			String ClientToken;
			/**
			 *	A list of the user's available profiles,
			 *	if received from Mojang.
			 */
			Nullable<Vector<Profile>> AvailableProfiles;
			/**
			 *	The profile that has been selected, if
			 *	any.
			 */
			Nullable<Profile> SelectedProfile;
	
	
	};
	
	
	/**
	 *	A Yggdrasil API error.
	 */
	class Error : public std::exception {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			Error (Word, String, String, Nullable<String>) noexcept;
			
			
			/**
			 *	\endcond
			 */
		
		
			/**
			 *	The HTTP status code that Yggdrasil returned.
			 */
			Word Status;
			/**
			 *	The type of error that was encountered.  Maps
			 *	exactly to the name of a Java class which extends
			 *	Exception.
			 */
			String Type;
			/**
			 *	A descriptive message describing the error.
			 */
			String Message;
			/**
			 *	The cause of the error, if received.
			 */
			Nullable<String> Cause;
			
			
			virtual const char * what () const noexcept override;
	
	
	};
	
	
	/**
	 *	The result of refreshing an access token.
	 */
	class RefreshResult {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			RefreshResult (JSON::Object);
			
			
			/**
			 *	\endcond
			 */
			
			
			/**
			 *	The access token which was refreshed.
			 */
			String AccessToken;
			/**
			 *	The associated client token.
			 */
			String ClientToken;
			/**
			 *	The selected profile, if any.
			 */
			Nullable<Profile> SelectedProfile;
	
	
	};


	/**
	 *	Processes asynchronous requests to the Yggdrasil
	 *	HTTP API.
	 */
	class Client {
	
	
		private:
		
		
			MCPP::HTTPHandler http;
			
			
			template <typename T, typename Callback, typename... Args>
			typename std::enable_if<
				!std::is_same<
					typename std::decay<Callback>::type,
					JSON::Object
				>::value,
				Promise<T>
			>::type dispatch (MCPP::HTTPRequest, Callback &&, Args &&...);
			template <typename T>
			Promise<T> dispatch (MCPP::HTTPRequest);
			template <typename T>
			Promise<T> dispatch (const String &, const JSON::Value &);
			
			
		public:
			
			
			/**
			 *	Authenticates a username and password through
			 *	Yggdrasil.
			 *
			 *	\param [in] username
			 *		The username to authenticate.
			 *	\param [in] password
			 *		The password to authenticate.
			 *	\param [in] client_token
			 *		Defaults to null.  The client token that the
			 *		caller wishes to supply to the Yggdrasil API,
			 *		if any.
			 *	\param [in] agent
			 *		Defaults to null.  The agent that the caller
			 *		wishes to supply to the Yggdrasil API, if
			 *		any.
			 *
			 *	\return
			 *		A promise of the future result of the
			 *		authentication request.
			 */
			Promise<AuthenticateResult> Authenticate (
				String username,
				String password,
				Nullable<String> client_token=Nullable<String>{},
				Nullable<Agent> agent=Nullable<Agent>{}
			);
			
			
			/**
			 *	Refreshes an access token through Yggdrasil.
			 *
			 *	\param [in] access_token
			 *		The access token previously obtained from
			 *		Yggdrasil that shall be refreshed.
			 *	\param [in] client_token
			 *		The client token used to initially obtain
			 *		\em access_token.
			 *	\param [in] profile
			 *		Defaults to null.  The profile that was
			 *		selected when \em access_token was initially
			 *		obtained.
			 *
			 *	\return
			 *		A promise of the future result of the refresh
			 *		request.
			 */
			Promise<RefreshResult> Refresh (
				String access_token,
				String client_token,
				Nullable<Profile> profile=Nullable<Profile>{}
			);
			
			
			/**
			 *	Queries Yggdrasil to determine if an access
			 *	token is valid.
			 *
			 *	\param [in] access_token
			 *		The access token to query.
			 *
			 *	\return
			 *		A promise of the future result of whether
			 *		\em access_token is valid or not.
			 */
			Promise<bool> Validate (String access_token);
			
			
			/**
			 *	Signs a user out through Yggdrasil.
			 *
			 *	\param [in] username
			 *		The username.
			 *	\param [in] password
			 *		The password.
			 *
			 *	\return
			 *		A promise of the result of the operation.
			 */
			Promise<void> Signout (String username, String password);
			
			
			/**
			 *	Invalidates a token.
			 *
			 *	\param [in] access_token
			 *		The access token to invalidate.
			 *	\param [in] client_token
			 *		The client token used to initially obtain
			 *		\em access_token.
			 *
			 *	\return
			 *		A promise of the result of the invalidation.
			 */
			Promise<void> Invalidate (String access_token, String client_token);
			
			
			/**
			 *	Begins a Minecraft game session login.
			 *
			 *	\param [in] access_token
			 *		The client's access token.
			 *	\param [in] profile_id
			 *		The ID of the client's currently selected
			 *		profile.
			 *	\param [in] server_id
			 *		The server ID the server sent.
			 *	\param [in] secret
			 *		The shared secret.
			 *	\param [in] public_key
			 *		The server's PublicKeyInfo structure.
			 *
			 *	\return
			 *		A promise of the result.
			 */
			Promise<void> ClientSession (
				String access_token,
				String profile_id,
				String server_id,
				const Vector<Byte> & secret,
				const Vector<Byte> & public_key
			);
			
			
			/**
			 *	Completes a Minecraft game session login.
			 *
			 *	\param [in] username
			 *		The username the client sent.
			 *	\param [in] server_id
			 *		The server ID string sent to the client.
			 *	\param [in] secret
			 *		The shared secret the client sent.
			 *	\param [in] pulic_key
			 *		The PublicKeyInfo used to complete the
			 *		encryption handshake with the client.
			 *
			 *	\return
			 *		A promise of the client's UUID from
			 *		Mojang.
			 */
			Promise<String> ServerSession (
				const String & username,
				const String & server_id,
				const Vector<Byte> & secret,
				const Vector<Byte> & public_key
			);
	
	
	};


}

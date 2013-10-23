#include <rleahylib/rleahylib.hpp>
#include <hash.hpp>
#include <http_handler.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <random.hpp>
#include <rsa_key.hpp>
#include <server.hpp>
#include <sha1.hpp>
#include <url.hpp>
#include <memory>
#include <unordered_map>
#include <utility>


using namespace MCPP;


static const String login_url("http://session.minecraft.net/game/checkserver.jsp?user={0}&serverId={1}");
static const String verbose_key("auth");
static const String http_request("HTTP request => {0}");
static const String http_reply_success("HTTP response <= {0} - Status: 200 - Time elapsed {1}ns - \"{2}\"");
static const String http_reply_error("HTTP response <= {0} - Status: {1} - Time elapsed {2}ns");
static const String http_reply_internal_error("HTTP response <= {0} internal error after {1}ns");
static const String auth_callback_error("Error during authentication");
static const String protocol_error("Protocol error");
static const String encryption_error("Encryption error");
static const String logged_in("{0}:{1} logged in as {2}");
static const String yes("yes");
static const String authentication_failed("Authentication failed");
//	minecraft.net sends "YES" or "NO", so maximum
//	bytes is 3
static const Word max_http_bytes=3;
static const Word verify_token_length=4;
static const Word priority=1;
static const String name("Minecraft.net Authentication Support");


enum class AuthenticationState {

	Waiting,
	ResponseSent,
	Authenticate

};


//	Data about each client
class ClientData {


	public:
	
	
		//	Guards this structure against
		//	concurrent access
		Mutex Lock;
		//	The state the associated client
		//	is in
		AuthenticationState State;
		//	"Verify token"
		Vector<Byte> VerifyToken;
		//	Shared secret
		Vector<Byte> Secret;
		//	Randomly-generated server ID
		//	string sent to this client
		String ServerID;
		
		
		ClientData () noexcept : State(AuthenticationState::Waiting) {	}


};


class Authentication : public Module {


	private:
	
	
		enum class Status {
		
			Success,
			Gone,
			ProtocolError,
			EncryptionError
		
		};
	
	
		//	Client sends this to begin
		//	the login process
		typedef Packets::Login::Serverbound::LoginStart start;
		//	Server responds with this
		//	to request a shared secret
		//	from the client
		typedef Packets::Login::Clientbound::EncryptionResponse key_request;
		//	Client sends shared secret
		//	and encrypted verify token
		//	in this packet
		typedef Packets::Login::Serverbound::EncryptionRequest key_response;
		//	After authenticating and enabling
		//	encryption, server confirms login
		//	successful by sending this
		typedef Packets::Login::Clientbound::LoginSuccess success;
	
	
		//	Master server encryption key
		RSAKey key;
		//	Asynchronous HTTP handler which
		//	makes requests to minecraft.net
		HTTPHandler http;
		//	Associates clients with data
		//	about them
		std::unordered_map<
			SmartPointer<Client>,
			SmartPointer<ClientData>
		> map;
		RWLock map_lock;
		//	Client is going to need
		//	random bytes
		Random<Byte> token_generator;
		Random<Word> id_len_generator;
		Random<Byte> id_generator;
		
		
		SmartPointer<ClientData> get (const SmartPointer<Client> client) {
		
			return map_lock.Read([&] () mutable {
			
				auto iter=map.find(client);
				
				return (iter==map.end()) ? SmartPointer<ClientData>() : iter->second;
			
			});
		
		}
		
		
		void handle (Status status, SmartPointer<Client> & client) {
		
			switch (status) {
			
				case Status::Success:
				case Status::Gone:
				default:break;
				
				case Status::ProtocolError:
					client->Disconnect(protocol_error);
					break;
					
				case Status::EncryptionError:
					client->Disconnect(encryption_error);
					break;
			
			}
		
		}
		
		
		Status login_start (ReceiveEvent & event) {
		
			auto data=get(event.From);
			
			//	Client has disconnected, do
			//	not do any further processing
			if (data.IsNull()) return Status::Gone;
			
			return data->Lock.Execute([&] () mutable {
			
				//	Verify client's authentication
				//	state
				if (data->State!=AuthenticationState::Waiting) return Status::ProtocolError;
				
				//	Set username
				event.From->SetUsername(
					std::move(
						event.Data.Get<start>().Name
					)
				);
				
				//	We respond with EncryptionResponse
				key_request reply;
				
				//	Generate a random server ID
				Word id_len=id_len_generator();
				Vector<Byte> id(id_len);
				for (Word i=0;i<id_len;++i) id.Add(id_generator());
				reply.ServerID=ASCII().Decode(id.begin(),id.end());
				data->ServerID=reply.ServerID;
				
				//	Provide our public key
				reply.PublicKey=key.PublicKey();
				
				//	Provide four random bytes -- a
				//	"verify token"
				reply.VerifyToken=Vector<Byte>(verify_token_length);
				for (Word i=0;i<verify_token_length;++i) reply.VerifyToken.Add(token_generator());
				data->VerifyToken=reply.VerifyToken;
				
				//	Send
				event.From->Send(reply);
				
				//	Advance client's authentication
				//	state
				data->State=AuthenticationState::ResponseSent;
				
				return Status::Success;
			
			});
		
		}
		
		
		void authenticate (SmartPointer<Client> & client, SmartPointer<ClientData> & data) {
		
			auto & server=Server::Get();
		
			//	Prepare a response
			success packet;
			packet.Username=client->GetUsername();
			
			data->Lock.Execute([&] () mutable {
			
				//	Atomically enable encryption,
				//	send response, and fire on login
				//	handler
				client->Atomic(
					Tuple<Vector<Byte>,Vector<Byte>>(
						data->Secret,
						data->Secret
					),
					packet,
					ProtocolState::Play,
					[&] () {	server.OnLogin(client);	}
				);
			
			});
			
			//	Client is authenticated, log
			server.WriteLog(
				String::Format(
					logged_in,
					client->IP(),
					client->Port(),
					client->GetUsername()
				),
				Service::LogType::Information
			);
		
		}
		
		
		Status encryption_request (ReceiveEvent & event) {
		
			auto data=get(event.From);
			
			//	Make sure client hasn't gone
			//	away
			if (data.IsNull()) return Status::Gone;
			
			return data->Lock.Execute([&] () mutable {
			
				//	Verify client's authentication
				//	state
				if (data->State!=AuthenticationState::ResponseSent) return Status::ProtocolError;
				
				auto & packet=event.Data.Get<key_response>();
				
				//	Match up verify token
				auto verify_token=key.PrivateDecrypt(packet.VerifyToken);
				if (verify_token.Count()!=data->VerifyToken.Count()) return Status::ProtocolError;
				for (Word i=0;i<verify_token.Count();++i) if (verify_token[i]!=data->VerifyToken[i]) return Status::EncryptionError;
				
				//	Decrypt the shared secret
				data->Secret=key.PrivateDecrypt(packet.Secret);
				
				//	Verify secret length (128 bits)
				if (data->Secret.Count()!=(128/BitsPerByte())) return Status::ProtocolError;
				
				//	Prepare a SHA-1 hash
				SHA1 hash;
				hash.Update(ASCII().Encode(data->ServerID));
				hash.Update(data->Secret);
				hash.Update(key.PublicKey());
				
				//	Get the URL to verify login
				//	against minecraft.net
				String url(
					String::Format(
						login_url,
						URL::Encode(event.From->GetUsername()),
						URL::Encode(hash.HexDigest())
					)
				);
				
				//	Debug log if requested
				auto & server=Server::Get();
				
				if (server.IsVerbose(verbose_key)) server.WriteLog(
					String::Format(
						http_request,
						url
					),
					Service::LogType::Debug
				);
				
				//	Advance client's state
				data->State=AuthenticationState::Authenticate;
				
				//	Start a timer to measure how
				//	long the HTTP request takes
				Timer timer(Timer::CreateAndStart());
				
				//	Dispatch HTTP request
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wpedantic"
				http.Get(
					url,
					[=,&server,client=event.From] (Word status, String response) mutable {
					
						try {
						
							auto elapsed=timer.ElapsedNanoseconds();
							
							auto data=get(client);
							
							if (data.IsNull()) return;
							
							if (status==200) {
							
								//	SUCCESS
								
								if (server.IsVerbose(verbose_key)) server.WriteLog(
									String::Format(
										http_reply_success,
										url,
										elapsed,
										response
									),
									Service::LogType::Debug
								);
								
								if (response.ToLower()==yes) authenticate(client,data);
								else client->Disconnect(authentication_failed);
							
							} else if (status==0) {
							
								//	cURL failure
							
								server.WriteLog(
									String::Format(
										http_reply_internal_error,
										url,
										elapsed
									),
									Service::LogType::Error
								);
							
							} else {
							
								//	HTTP failure
								
								server.WriteLog(
									String::Format(
										http_reply_error,
										url,
										status,
										elapsed
									),
									Service::LogType::Error
								);
							
							}
						
						} catch (...) {
						
							//	This shouldn't happen, but
							//	if it does we don't have anything
							//	higher up in the stack to
							//	deal with this for us
							
							client->Disconnect(auth_callback_error);
							
							throw;
						
						}
					
					}
				);
				#pragma GCC diagnostic pop
				
				return Status::Success;
			
			});
		
		}
		
		
	public:
	
	
		Authentication () : http(max_http_bytes), id_len_generator(15,20), id_generator('!','~') {	}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			auto & server=Server::Get();
			
			//	Install connect/disconnect handlers
			
			server.OnConnect.Add([this] (SmartPointer<Client> client) mutable {
			
				map_lock.Write([&] () {	map.emplace(std::move(client),SmartPointer<ClientData>::Make());	});
			
			});
			
			server.OnDisconnect.Add([this] (SmartPointer<Client> client, const String &) mutable {
			
				map_lock.Write([&] () {	map.erase(client);	});
			
			});
			
			//	Install packet handlers
			
			server.Router(
				start::PacketID,
				ProtocolState::Login
			)=[this] (ReceiveEvent event) mutable {	handle(login_start(event),event.From);	};
			
			server.Router(
				key_response::PacketID,
				ProtocolState::Login
			)=[this] (ReceiveEvent event) mutable {	handle(encryption_request(event),event.From);	};
		
		}


};


INSTALL_MODULE(Authentication)

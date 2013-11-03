#include <rleahylib/rleahylib.hpp>
#include <auth/auth.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <server.hpp>
#include <memory>
#include <unordered_map>
#include <utility>


using namespace MCPP;


static const String auth_callback_error("Error during authentication");
static const String protocol_error("Protocol error");
static const String encryption_error("Encryption error");
static const String logged_in("{0}:{1} logged in as {2}");
static const String authentication_failed("Authentication failed");
//	minecraft.net sends "YES" or "NO", so maximum
//	bytes is 3
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


class VanillaAuth : public Module {


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
	
	
		//	Associates clients with data
		//	about them
		std::unordered_map<
			SmartPointer<Client>,
			SmartPointer<ClientData>
		> map;
		RWLock map_lock;
		
		
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
				
				auto & auth=Authentication::Get();
				
				//	Server ID
				reply.ServerID=auth.GetServerID();
				data->ServerID=reply.ServerID;
				
				//	Public Key
				reply.PublicKey=auth.GetKey().PublicKey();
				
				//	Verify token
				reply.VerifyToken=auth.GetVerifyToken();
				data->VerifyToken=reply.VerifyToken;
				
				//	Send our reply
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
				
				auto & auth=Authentication::Get();
				auto & key=auth.GetKey();
				
				//	Match up verify token
				auto verify_token=key.PrivateDecrypt(packet.VerifyToken);
				if (!Authentication::VerifyTokens(
					verify_token,
					data->VerifyToken
				)) return Status::EncryptionError;
				
				//	Decrypt the shared secret
				data->Secret=key.PrivateDecrypt(packet.Secret);
				
				//	Verify secret length (128 bits)
				if (data->Secret.Count()!=(128/BitsPerByte())) return Status::ProtocolError;
				
				//	Advance client's state
				data->State=AuthenticationState::Authenticate;
				
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wpedantic"
				Authentication::Get().GetClient().ServerSession(
					event.From->GetUsername(),
					data->ServerID,
					data->Secret,
					key.PublicKey(),
					[=,client=event.From] (bool result) mutable {
					
						try {
							
							auto data=get(client);
							
							//	If client no longer exists, bail
							//	out
							if (data.IsNull()) return;
							
							if (result) authenticate(client,data);
							else client->Disconnect(authentication_failed);
						
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


INSTALL_MODULE(VanillaAuth)

#include <common.hpp>
#include <unordered_map>
#include <utility>


static const String protocol_error("Protocol error");
static const String out_of_date_error("Protocol version mismatch");
static const String name("Authentication Support");
static const String verify_token_mismatch("Verify token does not match");
static const String login_template("http://session.minecraft.net/game/checkserver.jsp?user={0}&serverId={1}");
static const String login_error("Error communicating with minecraft.net");
static const String login_error_log("Error communicating with minecraft.net, HTTP status code {0}");
static const String auth_fail("Failed to verify username");
static const String auth_fail_log("Failed to verify username, minecraft.net says \"{0}\"");
static const String auth_callback_error("Error during authentication");
static const String logged_in("{0}:{1} logged in as {2}");
static const String pa_banner("====PROTOCOL ANALYSIS====");
static const String http_request_template("HTTP request ==> {0}");
static const String http_success_template("HTTP response <== {0} - Status: 200 - Time elapsed: {1}ns - Response body ({2} grapheme{3}, {4} code point{5}):");
static const String http_fail_template("HTTP response <== {0} - Status: {1} - Time elapsed: {2}ns");
static const String http_error_template("HTTP response <== {0} - ERROR - Time elapsed: {1}ns");
static const Word priority=1;
//	Minecraft.net sends "YES" (3 bytes) or "NO" (2 bytes)
static const Word max_http_bytes=3;
static const Word num_random_bytes=4;


enum class AuthenticationState {

	//	Initial state -- waiting on
	//	0x02 with username and protocol
	//	version
	WaitingForHandshake,
	//	0x02 received and checked out,
	//	server has sent public key
	//	and random bytes to the client
	EncryptionRequestSent,
	//	0xFC received and checked out,
	//	server has shared secret, waiting
	//	for minecrafte.net authentication
	//	confirmation
	WaitingForAuthentication,
	//	Encryption has been enabled,
	//	waiting on 0xCD
	EncryptionEnabled
};


class AuthenticationClient {


	public:
	
	
		AuthenticationClient () noexcept
			:	Lock(SmartPointer<Mutex>::Make()),
				State(AuthenticationState::WaitingForHandshake)
		{	}
	
	
		//	This lock is used to guard
		//	against the client sending
		//	the same packet multiple
		//	times.
		//
		//	Has to be a pointer because
		//	the implementation of std::unordered_map
		//	is braindead and I don't feel
		//	like rewriting it at the moment.
		SmartPointer<Mutex> Lock;
		//	Records the client's progress
		//	through the authentication
		//	process.
		AuthenticationState State;
		//	Random bytes we generated
		//	for the encryption handshake.
		Vector<Byte> Bytes;
		//	Shared secret client generated
		Vector<Byte> Secret;
		//	Server ID this client was sent
		String ServerID;


};


class Authentication : public Module {


	private:
	
	
		//	Master server encryption
		//	key
		RSAKey key;
		//	Makes authentication requests
		//	to Minecraft.NET
		HTTPHandler http;
		//	Maps clients in the process
		//	of authenticating to data
		//	about their authentication
		std::unordered_map<
			const Connection *,
			AuthenticationClient
		> map;
		RWLock map_lock;
		//	Client is going to want
		//	random bytes
		Random<Byte> token_generator;
		Random<Word> id_len_generator;
		Random<Byte> id_generator;
	
	
	public:
	
	
		Authentication ()
			:	http(max_http_bytes),
				id_len_generator(
					15,	//	Shortest string observed from Notchian server
					20	//	Longest string possible (confirmed by exception thrown in client on longer strings)
				),
				id_generator('!','~')	//	Non-printable characters seem to make authentication choke
		{	}
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
	
	
		virtual void Install () override {
		
			//	We need to be notified of connect/
			//	disconnect events so we can
			//	manage our data structures
			
			RunningServer->OnConnect.Add([&] (SmartPointer<Client> client) {
			
				//	Add this client to the map
				map_lock.Write([&] () {	map.emplace(client->GetConn(),AuthenticationClient());	});
			
			});
			
			RunningServer->OnDisconnect.Add([&] (SmartPointer<Client> client, const String &) {
			
				//	Remove this client from the map
				//	if they're in the map
				map_lock.Write([&] () {	map.erase(client->GetConn());	});
			
			});
		
			//	We need to handle the following
			//	packets:
			//
			//	-	0x02 Handshake
			//	-	0xFC Encryption Response
			//	-	0xCD Client Statuses
			
			//	Extract the previous 0x02
			//	handler (if any)
			PacketHandler prev(
				std::move(
					RunningServer->Router[0x02]
				)
			);
			
			//	Install our own handler
			RunningServer->Router[0x02]=[=] (SmartPointer<Client> client, Packet packet) {
			
				//	We're only interested in newly-connected
				//	clients
				if (client->GetState()!=ClientState::Connected) {
				
					//	Maybe the handler we replaced is
					//	interested in handling this
					if (prev) prev(
						std::move(client),
						std::move(packet)
					);
					//	Else there's nothing to chain
					//	through to, kill the client
					else client->Disconnect(protocol_error);
					
					//	Short-circuit out
					return;
				
				}
				
				//	If the client makes a protocol
				//	error, this is set
				bool kill=false;
				//	If the client has been sent a reason,
				//	this is unset
				bool error=true;
				
				//	We need to read from the map
				map_lock.Read([&] () {
				
					//	Retrieve information about
					//	this client
					
					auto loc=map.find(client->GetConn());
					if (loc==map.end()) {
					
						//	The client has been disconnected
						kill=true;
						error=false;
						
						return;
					
					}
					
					auto & data=loc->second;
					
					//	Acquire the lock on this
					//	client
					data.Lock->Execute([&] () {
					
						//	Check client's state
						if (data.State!=AuthenticationState::WaitingForHandshake) {
						
							//	Wrong state, ERROR
							kill=true;
							
							return;
						
						}
						
						//	Verify client's protocol version
						if (packet.Retrieve<Byte>(0)!=ProtocolVersion) {
						
							//	Wrong version, KILL
							client->Disconnect(out_of_date_error);
							
							kill=true;
							error=false;	//	We've already set the reason
							
							return;
						
						}
						
						//	Retrieve client's username
						//	from the packet
						client->SetUsername(packet.Retrieve<String>(1));
						
						//	Prepare a response
						Packet reply;
						reply.SetType<PacketTypeMap<0xFD>>();
						
						//	Server ID
						Word num_ascii=id_len_generator();
						Vector<Byte> ascii(num_ascii);
						for (Word i=0;i<num_ascii;++i) ascii.Add(id_generator());
						
						data.ServerID=ASCII().Decode(ascii.begin(),ascii.end());

						reply.Retrieve<String>(0)=data.ServerID;
						
						//	Our public key
						reply.Retrieve<Vector<Byte>>(1)=key.PublicKey();
						
						//	Client is going to need four
						//	random bytes
						for (Word i=0;i<num_random_bytes;++i) data.Bytes.Add(token_generator());
						
						reply.Retrieve<Vector<Byte>>(2)=data.Bytes;
						
						//	Send our response
						client->Send(reply);
						
						//	Change client's state
						data.State=AuthenticationState::EncryptionRequestSent;
					
					});
				
				});
				
				if (kill) {
				
					if (error) client->Disconnect(protocol_error);
					
					//	Don't chain
					return;
				
				}
				
				//	Chain to previous callback
				//	if it exists
				if (prev) prev(
					std::move(client),
					std::move(packet)
				);
			
			};
			
			//	Extract previous 0xFC handler
			//	(if any)
			prev=std::move(RunningServer->Router[0xFC]);
			
			//	Install our own handler
			RunningServer->Router[0xFC]=[=] (SmartPointer<Client> client, Packet packet) {
			
				//	We're only interested in newly-connected
				//	clients
				if (client->GetState()!=ClientState::Connected) {
				
					//	Maybe the handler we replaced is
					//	interested in handling this
					if (prev) prev(
						std::move(client),
						std::move(packet)
					);
					//	Else there's nothing to chain
					//	through to, kill the client
					else client->Disconnect(protocol_error);
					
					//	Short-circuit out
					return;
				
				}
				
				//	If the client makes a protocol error,
				//	this is set
				bool kill=false;
				//	If the client has been sent a reason,
				//	this is unset
				bool error=true;
				
				//	We need to look the client up
				map_lock.Read([&] () {
				
					//	Retrieve information about
					//	this client
					auto loc=map.find(client->GetConn());
					if (loc==map.end()) {
					
						//	The client has been disconnected
						kill=true;
						error=false;
						
						return;
					
					}
					
					auto & data=loc->second;
					
					//	Acquire the lock on this
					//	client
					data.Lock->Execute([&] () {
					
						//	Verify client's state
						if (data.State!=AuthenticationState::EncryptionRequestSent) {
						
							//	Wrong state, ERROR
							kill=true;
							
							return;
						
						}
						
						//	Make sure client received our
						//	encryption key and is using
						//	it properly by retrieving the
						//	random bytes we generated
						//	(encrypted), decrypting them,
						//	and verifying
						Vector<Byte> plaintext_bytes(
							key.PrivateDecrypt(
								packet.Retrieve<Vector<Byte>>(1)
							)
						);
						
						bool bytes_match=true;
						
						if (data.Bytes.Count()==plaintext_bytes.Count()) {
						
							for (Word i=0;i<plaintext_bytes.Count();++i) {
							
								if (plaintext_bytes[i]!=data.Bytes[i]) {
								
									bytes_match=false;
									
									break;
								
								}
							
							}
						
						} else {
						
							bytes_match=false;
						
						}
						
						if (!bytes_match) {
						
							//	Client failed at encrypting
							client->Disconnect(verify_token_mismatch);
							
							error=false;
							kill=true;
							
							return;
						
						}
						
						//	Fetch shared secret
						data.Secret=key.PrivateDecrypt(
							packet.Retrieve<Vector<Byte>>(0)
						);
						
						//	Verify private key is 128 bits
						if (data.Secret.Count()!=(128/BitsPerByte())) {
						
							kill=true;
							
							return;
						
						}
						
						//	Prepare a hash
						SHA1 hash;
						//	Add ASCII encoding
						//	of server ID string
						//	to the hash
						hash.Update(ASCII().Encode(data.ServerID));
						//	Add shared secret
						//	to the hash
						hash.Update(data.Secret);
						//	Add ASN.1 representation
						//	of public key to the
						//	hash
						hash.Update(key.PublicKey());
						
						//	Prepare request to Minecraft.NET
						String request_url(
							String::Format(
								login_template,
								URL::Encode(client->GetUsername()),
								URL::Encode(hash.HexDigest())
							)
						);
						
						//	Protocol analysis
						if (RunningServer->ProtocolAnalysis) {
						
							String log(pa_banner);
							log << Newline << String::Format(
								http_request_template,
								request_url
							);
							
							RunningServer->WriteLog(
								log,
								Service::LogType::Information
							);
						
						}
						
						//	Advance client's state
						data.State=AuthenticationState::WaitingForAuthentication;
						
						//	Time for protocol analysis (if applicable)
						Timer timer=Timer::CreateAndStart();
						
						//	Fire off verify request
						//	to minecraft.net
						http.Get(
							request_url,
							[=] (Word status_code, String response) mutable {
							
								try {
								
									timer.Stop();
								
									//	Protocol analysis
									if (RunningServer->ProtocolAnalysis) {
									
										String log(pa_banner);
										log << Newline;
										
										//	cURL reports an error
										if (status_code==0) log << String::Format(
											http_error_template,
											request_url,
											timer.ElapsedNanoseconds()
										);
										//	HTTP request succeeded
										else if (status_code==200) log << String::Format(
											http_success_template,
											request_url,
											timer.ElapsedNanoseconds(),
											response.Count(),
											(response.Count()==1) ? "" : "s",
											response.Size(),
											(response.Size()==1) ? "" : "s"
										) << Newline << response;
										//	HTTP request failed
										else log << String::Format(
											http_fail_template,
											request_url,
											status_code,
											timer.ElapsedNanoseconds()
										);
										
										RunningServer->WriteLog(
											log,
											Service::LogType::Information
										);
									
									}
								
									//	Client's getting a packet
									//	one way or another
									Packet reply;
							
									//	Handle instances wherein
									//	client shall not be authenticated
									if (!(
										(status_code==200) &&
										(response.Trim().ToLower()=="yes")
									)) {
									
										//	Packet to client will
										//	be a disconnect/kick
										reply.SetType<PacketTypeMap<0xFF>>();
									
										//	Descriptive error string
										//	to log
										String log_error;
										
										if (status_code!=200) {
										
											reply.Retrieve<String>(0)=login_error;
											log_error=String::Format(
												login_error_log,
												status_code
											);
										
										} else {
										
											reply.Retrieve<String>(0)=auth_fail;
											log_error=String::Format(
												auth_fail_log,
												response
											);
										
										}
										
										//	Send, wait, disconnect
										client->Send(reply)->AddCallback([=] (SendState) mutable {
										
											client->Disconnect(log_error);
											
										});
									
										return;
									
									}
									
									//	Enable encryption
									reply.SetType<PacketTypeMap<0xFC>>();
									//	Retrieving default constructs
									//	and we want zero-length arrays
									reply.Retrieve<Vector<Byte>>(0);
									reply.Retrieve<Vector<Byte>>(1);
									
									//	Go to map to get secret
									if (!map_lock.Read([&] () {
									
										auto loc=map.find(client->GetConn());
										if (loc==map.end()) {
										
											//	The client has been disconnected
											client->Disconnect();	//	Just make sure, doesn't hurt
											
											return false;
										
										}
										
										auto & data=loc->second;
										
										//	Simultaneously send the
										//	encryption response and
										//	enable encryption on
										//	the client's connection
										client->Send(
											reply,
											data.Secret,
											data.Secret
										);
										
										//	Advance state
										data.State=AuthenticationState::EncryptionEnabled;
										
										return true;
									
									})) return;
									
									//	Client is authenticated, log
									RunningServer->WriteLog(
										String::Format(
											logged_in,
											client->IP(),
											client->Port(),
											client->GetUsername()
										),
										Service::LogType::Information
									);
									
								} catch (...) {
								
									//	This shouldn't happen
									//
									//	Kill the client
									client->Disconnect(auth_callback_error);
								
								}
							
							}
						);
					
					});
				
				});
			
			};
			
			//	Extract the previous 0xCD
			//	handler (if any)
			//
			//	Chaining is very important
			//	as this packet can have
			//	different payloads and be
			//	interpreted in different ways
			//
			//	We're only interested in
			//	handling it right after
			//	authentication, but some
			//	other module might want to
			//	handle it when it's sent when
			//	the player dies and is then
			//	ready to respawn
			prev=std::move(RunningServer->Router[0xCD]);
			
			//	Install our own handler
			RunningServer->Router[0xCD]=[=] (SmartPointer<Client> client, Packet packet) {
			
				//	We're only interested in newly-connection
				//	clients, and clients sending 0 as the
				//	payload
				if (!(
					(packet.Retrieve<SByte>(0)==0) ||
					(client->GetState()==ClientState::Connected)
				)) {
				
					//	Chain
					if (prev) prev(
						std::move(client),
						std::move(packet)
					);
					//	If there's nothing to chain to,
					//	kill the client with a protocol
					//	error
					else client->Disconnect(protocol_error);
					
					//	Don't proceed
					return;
				
				}
				
				//	Client made an error, and
				//	is to be disconnected
				bool kill=false;
				//	We already handled the error,
				//	don't set a new reason, just
				//	disconnect
				bool error=true;
				
				//	We need to fetch our information about
				//	this client
				map_lock.Read([&] () {
				
					//	Grab data from the map
					
					auto loc=map.find(client->GetConn());
					if (loc==map.end()) {
					
						//	The client has been
						//	disconnected
						kill=true;
						error=false;
						
						return;
					
					}
					
					auto & data=loc->second;
					
					//	Lock this client's data
					data.Lock->Execute([&] () {
					
						//	Verify client's state
						if (data.State!=AuthenticationState::EncryptionEnabled) {
						
							//	Wrong state, ERROR
							kill=true;
							
							return;
						
						}
						
						//	Hit the client with a 0x01
						Packet reply;
						reply.SetType<PacketTypeMap<0x01>>();
						
						//	TODO: Populate this data
						//	properly
						reply.Retrieve<Int32>(0)=1;
						reply.Retrieve<String>(1)="default";
						reply.Retrieve<SByte>(2)=0;
						reply.Retrieve<SByte>(3)=0;
						reply.Retrieve<SByte>(4)=0;
						reply.Retrieve<SByte>(5)=0;
						reply.Retrieve<SByte>(6)=20;
						
						//	Atomically authenticate the
						//	player, send this packet
						//	to them, and fire the
						//	authenticate event
						client->Send(
							reply,
							ClientState::Authenticated,
							[&] (Client &, SmartPointer<SendHandle>) {
							
								RunningServer->OnLogin(client);
							
							}
						);
					
					});
				
				});
				
				//	Disconnect if necessary
				if (kill) {
				
					if (error) client->Disconnect(protocol_error);
					
					//	Don't chain
					return;
				
				}
				
				//	Chain (if applicable)
				if (prev) prev(
					std::move(client),
					std::move(packet)
				);
			
			};
		
		}


};


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		try {
		
			if (mod_ptr==nullptr) mod_ptr=new Authentication();
		
		} catch (...) {	}
		
		return mod_ptr;
	
	}
	
	
	void Unload () {
	
		if (mod_ptr!=nullptr) {
		
			delete mod_ptr;
			
			mod_ptr=nullptr;
		
		}
	
	}


}

#include <common.hpp>
#include <utility>
#include <unordered_map>
#include <functional>


static const String name ("Keep Alive, Latency, and Timeout Support");
static const String timeout("Timeout of {0} ms exceeded (inactive for {1}ms)");
static const String pa_banner("====PROTOCOL ANALYSIS====");
static const String pa_inactive_template("{0}:{1} inactive for {2}ms: {3}");
static const String pa_ping("{0}:{1} replied to keep alive, latency is {2}ms");
static const String pa_terminating("Terminating connection");
static const String pa_will_not_terminate("Will not terminate");
static const Word priority=1;
static const Word timeout_milliseconds=10000;
static const Word keep_alive_milliseconds=5000;
static const String protocol_error("Protocol error");
static const String ping_timed_out("Ping timed out ({0}ms)");
static const String keep_alive_pa_key("keepalive");


class KeepAliveInfo {


	public:
	
	
		//	Time since the last keep alive
		//	was sent
		Timer Latency;
		//	ID that was last sent
		Int32 ID;
		//	Are we waiting on a keep alive?
		bool Waiting;
		
		
		KeepAliveInfo () noexcept : Waiting(false) {	}


};


class KeepAlive : public Module {


	private:


		std::unordered_map<const Connection *,KeepAliveInfo> map;
		Mutex map_lock;
		Random<Int32> generator;
		std::function<void ()> callback;


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	We need to handle keep alives
			
			//	Old keep alive handler
			PacketHandler prev(std::move(Server::Get().Router[0x00]));
			
			//	Install our handler
			Server::Get().Router[0x00]=[=] (SmartPointer<Client> client, Packet packet) {
			
				typedef PacketTypeMap<0x00> pt;
			
				//	Check the ID
				//
				//	If it's zero this is the client
				//	pinging us, just send that right
				//	back at them.
				if (packet.Retrieve<pt,0>()==0) {
				
					Packet reply;
					reply.SetType<pt>();
					reply.Retrieve<pt,0>()=0;
					
					client->Send(reply);
				
				//	If it's not zero this is the client
				//	responding to us
				} else {
				
					map_lock.Execute([&] () {
					
						//	Look up our data about the client
						auto & data=map[client->GetConn()];
						
						//	Compare IDs
						if (packet.Retrieve<pt,0>()==data.ID) {
						
							//	They match, complete keep alive
							
							data.Latency.Stop();
							
							//	Set ping
							client->Ping=data.Latency.ElapsedMilliseconds();
							
							//	Protocol analysis
							if (Server::Get().IsVerbose(keep_alive_pa_key)) {
							
								String log(pa_banner);
								log << Newline << String::Format(
									pa_ping,
									client->IP(),
									client->Port(),
									static_cast<Word>(client->Ping)
								);
								
								Server::Get().WriteLog(
									log,
									Service::LogType::Debug
								);
							
							}
							
							//	No longer waiting for a reply
							//	to that keep alive
							data.Waiting=false;
						
						//	That's a protocol error
						} else {
						
							client->Disconnect(protocol_error);
						
						}
					
					});
				
				}
				
				//	Chain
				if (prev) prev(
					std::move(client),
					std::move(packet)
				);
			
			};
			
			//	Start the loop of callbacks to
			//	ping the client and see if they
			//	should be disconnected for
			//	inactivity
			
			//	Kind of a little hack here to
			//	get a lambda to capture itself...
			callback=[=] () {
			
				//	Loop for all clients
				for (auto & client : Server::Get().Clients) {
				
					//	How long have they been inactive?
					Word inactive=client->Inactive();
					
					//	Did they time out?
					bool timed_out=inactive>timeout_milliseconds;
					
					//	Protocol analysis
					if (Server::Get().IsVerbose(keep_alive_pa_key)) {
					
						String log(pa_banner);
						log << Newline << String::Format(
							pa_inactive_template,
							client->IP(),
							client->Port(),
							inactive,
							timed_out ? pa_terminating : pa_will_not_terminate
						);
						
						Server::Get().WriteLog(
							log,
							Service::LogType::Debug
						);
					
					}
					
					//	Kill if they've timed out
					if (timed_out) {
					
						client->Disconnect(
							String::Format(
								timeout,
								timeout_milliseconds,
								inactive
							)
						);
					
					//	Send them a ping
					//	but only if they've
					//	logged in
					} else if (client->GetState()==ClientState::Authenticated) {
					
						//	Generate a random ID that isn't
						//	zero
						Int32 id=generator();
						//	We could loop, but just make it
						//	not-zero by adding 1
						if (id==0) ++id;
					
						map_lock.Execute([&] () {
						
							auto & data=map[client->GetConn()];
							
							//	Are we waiting on a ping
							//	back?
							if (data.Waiting) {
							
								//	Kill them
								client->Disconnect(
									String::Format(
										ping_timed_out,
										data.Latency.ElapsedMilliseconds()
									)
								);
							
							//	We can send them a new
							//	keep alive
							} else {
							
								typedef PacketTypeMap<0x00> pt;
								
								Packet ping;
								ping.SetType<pt>();
								ping.Retrieve<pt,0>()=id;
								
								data.ID=id;
								
								data.Latency.Reset();
								data.Latency.Start();
								
								client->Send(ping);
							
							}
						
						});
					
					}
				
				}
				
				//	Enqueue this task again
				Server::Get().Pool().Enqueue(
					(timeout_milliseconds>keep_alive_milliseconds)
						?	keep_alive_milliseconds
						:	timeout_milliseconds,
					callback
				);
			
			};
			//	Enqueue
			Server::Get().Pool().Enqueue(
				(timeout_milliseconds>keep_alive_milliseconds)
					?	keep_alive_milliseconds
					:	timeout_milliseconds,
				callback
			);
			
			//	We need to hook into connect/disconnect
			//	to maintain our data structure
			Server::Get().OnConnect.Add([=] (SmartPointer<Client> client) {
			
				map_lock.Execute([&] () {	map.emplace(client->GetConn(),KeepAliveInfo());	});
			
			});
			
			Server::Get().OnDisconnect.Add([=] (SmartPointer<Client> client, const String &) {
			
				map_lock.Execute([&] () {	map.erase(client->GetConn());	});
			
			});
		
		}


};


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		if (mod_ptr==nullptr) try {
		
			mod_ptr=new KeepAlive();
		
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

#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <seed_sequence.hpp>
#include <server.hpp>
#include <synchronized_random.hpp>
#include <uniform_int_distribution.hpp>
#include <exception>
#include <random>
#include <unordered_map>
#include <utility>


using namespace MCPP;


static const String name("Keep Alive, Latency, and Timeout Support");
static const Word priority=1;
static const String debug_key("keepalive");
static const String timeout_setting("timeout");
static const Word timeout_default=10000;
static const String keep_alive_setting("keep_alive_frequency");
static const Word keep_alive_default=5000;


//	Disconnect messages
static const String protocol_error("Unexpected keep alive packet");
static const String ping_timed_out("Ping timed out ({0}ms)");
static const String timed_out("Timeout of {0}ms exceeded (inactive for {1}ms)");


//	Log templates
static const String log_ping("{0}:{1} replied to keep alive, latency is {2}ms");
static const String log_inactive("{0}:{1} inactive for {2}ms: {3}");
static const String log_action_terminate("Terminating connection");
static const String log_action_keep("Will not terminate");


class KeepAliveInfo {


	public:
	
	
		//	Time since last keep alive was
		//	sent
		Timer Latency;
		//	ID that was last sent
		Int32 ID;
		//	Are we waiting on a keep alive?
		bool Waiting;
		
		
		KeepAliveInfo () noexcept : Waiting(false) {	}


};


class KeepAlive : public Module {


	private:
	
	
		//	Type of packet we'll be sending
		typedef Packets::Play::Clientbound::KeepAlive send;
		//	Type of packet we'll be receiving
		typedef Packets::Play::Serverbound::KeepAlive recv;
	
	
		//	Maps clients to information about
		//	them
		std::unordered_map<SmartPointer<Client>,KeepAliveInfo> map;
		Mutex lock;
		
		
		//	Generates random IDs for the keep
		//	alive packets
		SynchronizedRandom<UniformIntDistribution<Int32>> dist;
		std::mt19937 gen;
		
		
		//	Settings
		
		//	How long constitutes a timeout
		Word timeout;
		//	How often keep alive packets should
		//	be sent
		Word keep_alive;
		
		
		//	Handles incoming packets
		void handler (PacketEvent event) {
		
			auto & packet=event.Data.Get<recv>();
			
			//	Check the ID, if it's zero then
			//	it's a client-initiated keep alive
			//	and we just reply in kind
			if (packet.KeepAliveID==0) {
			
				send reply;
				reply.KeepAliveID=0;
				
				event.From->Send(reply);
			
			//	Otherwise we have to check to make
			//	sure this is the right ID etc.
			} else {
			
				UInt64 elapsed;
				if (lock.Execute([&] () mutable {
				
					//	Look up data about this
					//	client
					auto iter=map.find(event.From);
					//	If the client's data can't be
					//	found, just stop processing
					if (iter==map.end()) return false;
					
					auto & data=iter->second;
					
					//	Stop the timer at once
					elapsed=data.Latency.ElapsedMilliseconds();
					
					//	If we're not actually
					//	waiting for a keep alive
					//	from this client, or this
					//	client sent the wrong ID,
					//	we kill them with a protocol
					//	error
					if (!(
						data.Waiting &&
						(data.ID==packet.KeepAliveID)
					)) {
					
						event.From->Disconnect(protocol_error);
						
						return false;
					
					}
					
					//	Everything is good, update
					//	the client's latency
					event.From->Ping=elapsed;
					//	We're no longer waiting
					data.Waiting=false;
					
					return true;
					
				})) {
				
					//	Log if applicable
					auto & server=Server::Get();
					if (server.IsVerbose(debug_key)) server.WriteLog(
						String::Format(
							log_ping,
							event.From->IP(),
							event.From->Port(),
							elapsed
						),
						Service::LogType::Debug
					);
				
				}
			
			}
		
		}
		
		
		Word get_callback_freq () const noexcept {
		
			return (timeout>keep_alive) ? keep_alive : timeout;
		
		}
		
		
		//	Handles timed callbacks
		void callback () {
		
			auto & server=Server::Get();
		
			try {
				
				//	Cache whether or not we're doing
				//	verbose logging
				bool is_verbose=server.IsVerbose(debug_key);
			
				//	Loop for all clients
				for (auto & client : server.Clients) {
				
					//	Determine how long this client has been
					//	inactive
					auto inactive=client->Inactive();
					
					//	Did the client time out?
					bool timed_out=inactive>timeout;
					
					//	Debug logging if applicable
					if (is_verbose) server.WriteLog(
						String::Format(
							log_inactive,
							client->IP(),
							client->Port(),
							inactive,
							timed_out ? log_action_terminate : log_action_keep
						),
						Service::LogType::Debug
					);
					
					//	Kill if client has timed out
					if (timed_out) {
					
						client->Disconnect(
							String::Format(
								::timed_out,
								timeout,
								inactive
							)
						);
					
					//	Send them a ping, but only if they're
					//	in the correct state
					} else if (client->GetState()==ProtocolState::Play) {
					
						//	Generate a random ID that isn't zero
						//	(since zero is reserved for the client's
						//	use)
						auto id=dist(gen);
						if (id==0) ++id;
						
						lock.Execute([&] () mutable {
						
							//	Look up client's data
							auto iter=map.find(client);
							//	If the data could not be found,
							//	just abort
							if (iter==map.end()) return;
							
							auto & data=iter->second;
							
							//	Are we still waiting on a previous
							//	ping?
							if (data.Waiting) {
							
								//	Kill the client
								
								client->Disconnect(
									String::Format(
										ping_timed_out,
										data.Latency.ElapsedMilliseconds()
									)
								);
								
								return;
								
							}
							
							//	Send a new keep alive
							send packet;
							packet.KeepAliveID=id;
							
							client->Send(packet);
							
							//	We're now waiting
							data.Waiting=true;
							data.ID=id;
						
						});
					
					}
				
				}
				
				//	Queue up next iteration
				server.Pool().Enqueue(
					get_callback_freq(),
					[this] () mutable {	callback();	}
				);
				
			} catch (...) {
			
				//	Panic
				try {	server.Panic(std::current_exception());	} catch (...) {	}
				
				throw;
			
			}
		
		}
		
		
		static std::mt19937 get_mt19937 () {
		
			SeedSequence seq;
			return std::mt19937(seq);
		
		}
		
		
	public:
	
	
		KeepAlive () : gen(get_mt19937()) {	}
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			auto & server=Server::Get();
			
			//	Get settings
			auto & data=server.Data();
			timeout=data.GetSetting(timeout_setting,timeout_default);
			keep_alive=data.GetSetting(keep_alive_setting,keep_alive_default);
			
			//	Install handler for incoming keep alives
			server.Router(
				recv::PacketID,
				recv::State
			)=[this] (PacketEvent event) mutable {	handler(std::move(event));	};
			
			//	Queue up timed callback
			server.Pool().Enqueue(
				get_callback_freq(),
				[this] () mutable {	callback();	}
			);
			
			//	Attach to connect/disconnect events
			//	to add/remove data structures
			server.OnConnect.Add([this] (SmartPointer<Client> client) mutable {
			
				lock.Execute([&] () mutable {	map.emplace(std::move(client),KeepAliveInfo());	});
			
			});
			server.OnDisconnect.Add([this] (SmartPointer<Client> client, const String &) mutable {
			
				lock.Execute([&] () mutable {	map.erase(client);	});
			
			});
		
		}


};


INSTALL_MODULE(KeepAlive)

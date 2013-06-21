#include <common.hpp>
#include <utility>
#include <unordered_map>
#include <functional>


//	The frequency with which every player's
//	list is updated to propagate ping changes
//	et cetera
static const Word update_freq=30000;
static const String name("Player List Support");
static const Word priority=1;


class PlayerListInfo {


	public:
	
	
		//	The client
		SmartPointer<Client> Conn;
		//	Whether or not other clients
		//	have been notified that this
		//	client logged in.
		bool Sent;
	
	
		PlayerListInfo (SmartPointer<Client> client) noexcept : Conn(std::move(client)), Sent(false) {	}
		PlayerListInfo () = default;


};


class PlayerList : public Module {


	private:
	
	
		//	The packet we'll be dealing with
		typedef PacketTypeMap<0xC9> pt;
		
		
		//	Periodic callback to keep pings in the
		//	clients' lists up to date
		std::function<void ()> callback;
	
	
		//	The onlogin event (which we use to trigger
		//	player list updates) is not guaranteed to
		//	fire with any particular ordering with respect
		//	to the ondisconnect event (which we also use
		//	to send player list updates).
		//
		//	The Notchian server gets around this issue
		//	in two ways:
		//
		//	1.	It's a single-threaded piece of garbage.
		//	2.	It sends one packet per user to every
		//		user per tick.
		//
		//	Both of these design choices are moronic, like
		//	so much else about the raw implementation of
		//	vanilla Minecraft.
		//
		//	The way we will reconcile this issue is by
		//	maintaining our own map of connected users.
		//
		//	Whenever the module is ready to generate some
		//	event, it'll lock the map and deal with a snapshot
		//	of the data, thereby insuring consistency.
		//
		//	It will set the flag in each user's data structure
		//	when it sends an initial list update with that
		//	user flagged as connected.  This tells
		//	the ondisconnect event that it can send
		//	player list updates disconnecting that user.
		//
		//	When a player list update is sent specifying
		//	that a user has gone offline, that user shall
		//	be removed from this data structure, which means
		//	that if the onlogin event is not synchronized
		//	it shall just be ignored and no updates
		//	for that player will ever be sent.
		//
		//	Similarly, the disconnect handler will avoid
		//	firing off updates if no client was ever
		//	notified that the client logged in in the
		//	first place.
		//
		//	Moreover, the server will periodically update
		//	all players (just not once every tick) so that
		//	ping information in each client is up-to-date.
		std::unordered_map<const Connection *,PlayerListInfo> map;
		Mutex map_lock;
		
		
	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	We need to handle connect
			//	events to keep the set
			//	up to date
			RunningServer->OnConnect.Add([=] (SmartPointer<Client> client) {
			
				map_lock.Execute([&] () {
				
					auto ptr=client->GetConn();
				
					map.emplace(
						ptr,
						PlayerListInfo(std::move(client))
					);
				
				});
				
			});
			
			//	We need to handle disconnect
			//	events to keep the set up
			//	to date and to send messages
			//	to the other connected clients
			RunningServer->OnDisconnect.Add([=] (SmartPointer<Client> client, const String &) {
			
				//	Get consistent state
				map_lock.Execute([&] () {
				
					auto conn=client->GetConn();
					
					//	Fetch data
					auto data=std::move(map[conn]);
					
					//	Remove
					map.erase(conn);
					
					//	Only proceed if an update
					//	for this client was sent
					if (data.Sent) {
					
						//	Prepare a packet
						Packet packet;
						packet.SetType<pt>();
						
						packet.Retrieve<pt,0>()=client->GetUsername();
						packet.Retrieve<pt,1>()=false;
						packet.Retrieve<pt,2>()=static_cast<pt::RetrieveType<2>::Type>(client->Ping);
						
						//	Send to all other clients
						//	who have received their
						//	initial updates
						for (auto & kvp : map) {
						
							if (
								//	This client has had their
								//	listing populated
								kvp.second.Sent &&
								//	This client is not the client
								//	we're disconnecting
								(kvp.second.Conn->GetConn()!=client->GetConn())
							) kvp.second.Conn->Send(packet);
						
						}
					
					}
				
				});
				
			});
			
			//	We need to handle login to populate both
			//	the player logging in, and to keep
			//	all the other players up-to-date
			RunningServer->OnLogin.Add([=] (SmartPointer<Client> client) {
			
				//	Get consistent state
				map_lock.Execute([&] () {
				
					//	Has this player been disconnected?
					auto loc=map.find(client->GetConn());
					
					//	No, proceed
					if (loc!=map.end()) {
					
						auto & data=loc->second;
						
						//	Mark this player as having
						//	been processed
						data.Sent=true;
						
						//	Generate an update packet about
						//	this user
						Packet packet;
						packet.SetType<pt>();
						packet.Retrieve<pt,0>()=client->GetUsername();
						packet.Retrieve<pt,1>()=true;
						packet.Retrieve<pt,2>()=client->Ping;
					
						//	Scan the list of users
						//	
						//	Give this user information on
						//	all connected users (including
						//	himself)
						//
						//	Give all other users information
						//	on this user.
						for (auto & kvp : map) {
						
							if (kvp.second.Sent) {
							
								auto & conn=kvp.second.Conn;
								
								//	It's the user who's logging in
								if (conn->GetConn()==client->GetConn()) {
								
									//	Send a packet about himself
									client->Send(packet);
								
								//	It's a different user
								} else {
								
									//	Create and send a packet about
									//	the user who's logging in to this
									//	user
									Packet curr;
									curr.SetType<pt>();
									curr.Retrieve<pt,0>()=conn->GetUsername();
									curr.Retrieve<pt,1>()=true;
									curr.Retrieve<pt,2>()=conn->Ping;
									
									client->Send(curr);
									
									//	Send a packet to this user
									//	about the user logging in
									conn->Send(packet);
								
								}
							
							}
						
						}
					
					}
				
				});
			
			});
			
			//	Prepare a periodic callback which
			//	will update the listings of all
			//	connected users who have received
			//	their initial update.
			callback=[=] () {
			
				//	Get consistent state
				map_lock.Execute([&] () {
				
					//	Create the packets that will
					//	be sent to each user
					Vector<Packet> packets;
					for (auto & kvp : map) {
					
						if (kvp.second.Sent) {
						
							auto & client=kvp.second.Conn;
							
							Packet packet;
							packet.SetType<pt>();
							packet.Retrieve<pt,0>()=client->GetUsername();
							packet.Retrieve<pt,1>()=true;
							packet.Retrieve<pt,2>()=client->Ping;
						
						}
					
					}
					
					//	Now send all those packets
					for (auto & kvp : map) {
					
						if (kvp.second.Sent) {
						
							for (auto & packet : packets) kvp.second.Conn->Send(packet);
						
						}
					
					}
				
				});
				
				//	Schedule this callback
				//	again
				RunningServer->Pool().Enqueue(
					update_freq,
					callback
				);
			
			};
			//	Schedule this callback to run
			//	initially
			RunningServer->Pool().Enqueue(
				update_freq,
				callback
			);
		
		}


};


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		if (mod_ptr==nullptr) try {
		
			mod_ptr=new PlayerList();
		
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

#include <rleahylib/rleahylib.hpp>
#include <client.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <server.hpp>
#include <limits>
#include <unordered_map>
#include <utility>


using namespace MCPP;


//	The frequency with which every player's
//	list is updated to propagate ping changes
//	et cetera
static const Word update_freq=30000;
static const String name("Player List Support");
static const Word priority=1;


class PlayerList : public Module {


	private:
	
	
		typedef Packets::Play::Clientbound::PlayerListItem packet_type;
	
	
		std::unordered_map<
			SmartPointer<Client>,
			//	This boolean value indicates
			//	whether disconnect processing
			//	has occurred
			bool
		> map;
		Mutex lock;
		
		
		typedef decltype(packet_type::Ping) ping_type;
		
		
		static ping_type get_ping (Word ping) noexcept {
		
			constexpr auto ping_max=std::numeric_limits<ping_type>::max();
			
			return (ping>ping_max) ? ping_max : static_cast<ping_type>(ping);
		
		}
		
		
		static packet_type get_packet (const SmartPointer<Client> & client, bool online) {
		
			packet_type retr;
			retr.Name=client->GetUsername();
			retr.Online=online;
			retr.Ping=get_ping(client->Ping);
			
			return retr;
		
		}
		
		
		Vector<packet_type> get_packets () {
		
			Vector<packet_type> retr(map.size());
			
			for (auto & pair : map) if (!pair.second) retr.Add(
				get_packet(
					pair.first,
					true
				)
			);
			
			return retr;
		
		}
		
		
		void login (SmartPointer<Client> client) {
		
			lock.Execute([&] () mutable {
			
				//	Attempt to find this client
				//	in the map
				auto loc=map.find(client);
				
				//	If there's an entry, it means
				//	that disconnect processing has occurred,
				//	delete and return
				if (loc!=map.end()) {
				
					map.erase(loc);
					
					return;
				
				}
				
				//	Otherwise disconnect processing has not
				//	occurred, and we may proceed
				
				//	Send a full update to this connecting
				//	client
				for (auto & packet : get_packets()) client->Send(packet);
				
				//	Insert an entry specifying that disconnect
				//	processing has not occurred
				map.emplace(
					client,
					false
				);
				
				//	Send packet to all connected clients
				//	who are in the correct state
				auto packet=get_packet(client,true);
				for (auto & c : Server::Get().Clients) if (c->GetState()==ProtocolState::Play) c->Send(packet);
			
			});
		
		}
		
		
		void disconnect (SmartPointer<Client> client) {
		
			lock.Execute([&] () mutable {
			
				//	Attempt to find this client in the
				//	map
				auto loc=map.find(client);
				
				//	If there's no entry, this means that
				//	no client has been notified that this
				//	client ever connected/was online, and
				//	therefore we simply add an entry and
				//	proceed without sending packets
				if (loc==map.end()) {
				
					map.emplace(
						client,
						true
					);
					
					return;
				
				}
				
				//	Remove the player's entry -- they're
				//	disconnected
				map.erase(loc);
				
				//	Otherwise we send a packet to all connected
				//	clients notifying them
				auto packet=get_packet(client,false);
				for (auto & c : Server::Get().Clients) if (
					(c!=client) &&
					(c->GetState()==ProtocolState::Play)
				) c->Send(packet);
			
			});
		
		}
		
		
		void periodic () {
		
			auto & server=Server::Get();
			
			lock.Execute([&] () mutable {
			
				auto packets=get_packets();
				
				//	Loop over all connected clients in the
				//	Play state and send them these packets
				for (auto & c : server.Clients)
				if (c->GetState()==ProtocolState::Play)
				for (auto & packet : packets) c->Send(packet);
			
			});
			
			//	Execute the task again after a delay
			server.Pool().Enqueue(
				update_freq,
				[this] () mutable {	periodic();	}
			);
		
		}


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			auto & server=Server::Get();
			
			server.OnLogin.Add([this] (SmartPointer<Client> client) mutable {	login(std::move(client));	});
			server.OnDisconnect.Add([this] (SmartPointer<Client> client, const String &) mutable {	disconnect(std::move(client));	});
			server.Pool().Enqueue(
				update_freq,
				[this] () mutable {	periodic();	}
			);
		
		}


};


INSTALL_MODULE(PlayerList)

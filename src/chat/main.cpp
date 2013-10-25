#include <chat/chat.hpp>
#include <hash.hpp>
#include <packet.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>


using namespace MCPP;


namespace MCPP {


	static const String name("Chat Support");
	static const Word priority=1;
	static const String mods_dir("chat_mods");
	static const String log_prepend("Chat Support: ");
	static const String protocol_error("Protocol error");


	const String & Chat::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Chat::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	//	Maximum depth to which incoming JSON
	//	messages will be parsed
	static const Word max_depth=5;
	
	
	void Chat::Install () {
	
		auto & server=Server::Get();
	
		//	Incoming packet type
		typedef Packets::Play::Serverbound::ChatMessage type;
		
		//	Handle incoming chat messages
		server.Router(
			type::PacketID,
			ProtocolState::Play
		)=[this] (ReceiveEvent event) mutable {

			//	If there's no installed handler,
			//	abort at once
			if (!Chat) return;
		
			auto & packet=event.Data.Get<type>();
			
			ChatEvent dispatch;
			dispatch.From=event.From;
			dispatch.Body=std::move(packet.Value);
			
			Chat(std::move(dispatch));
		
		};
		
		//	Make sure that we don't keep
		//	module assets after the server
		//	is shutdown
		server.OnShutdown.Add([this] () mutable {	Chat=ChatHandler();	});
	
	}
	
	
	static const String server_username("SERVER");
	
	
	Vector<String> Chat::Send (const ChatMessage & message) {
	
		//	Create the packet
		Packets::Play::Clientbound::ChatMessage packet;
		packet.Value=Format(message);
		
		//	List of clients to whom the message
		//	could not be delivered
		Vector<String> retr;
		
		//	Return to sender if this is
		//	an echo message
		if (message.Echo && !message.From.IsNull()) {
		
			const_cast<ChatMessage &>(message).From->Send(packet);
		
			return retr;
		
		}
		
		auto & server=Server::Get();
		
		//	If this is a broadcast, simply send to everyone
		if (
			(message.To.Count()==0) &&
			(message.Recipients.Count()==0)
		) {
		
			for (auto & client : server.Clients) client->Send(packet);
			
			return retr;
		
		}
		
		//	Create two lookup tables for recipients
		//	whose handles we have -- one which relates
		//	the handle to the normalized username of
		//	that user, another which can simply be used
		//	to lookup the handle.
		std::unordered_map<String,SmartPointer<Client>> names;
		std::unordered_set<SmartPointer<Client>> handles;
		for (const auto & c : message.Recipients) {
		
			String lowercase;
			
			//	Special case for the server
			if (c.IsNull()) {
			
				handles.emplace(c);
				
				lowercase=server_username;
			
			//	Skip clients who are not in the
			//	appropriate state
			} else if (c->GetState()==ProtocolState::Play) {
			
				handles.emplace(c);
				
				lowercase=c->GetUsername().ToLower();
			
			} else {
			
				continue;
			
			}
			
			names.emplace(
				std::move(lowercase),
				c
			);
		
		}
		
		//	Rearrange the clients we're delivering
		//	to by name into a hash table, which maps
		//	their normalized (i.e. lowercased) username
		//	to the actual username that was used for
		//	them.
		//
		//	This will greatly reduce the overhead of
		//	tracking the recipients to whom we could
		//	or could not deliver
		std::unordered_map<String,const String *> to;
		for (const auto & s : message.To) {
		
			String lowercase(s);
			//	There's a special case for
			//	the server -- that username
			//	is always uppercase
			if (s!=server_username) lowercase.ToLower();
			
			//	If this user is also being delivered
			//	to by handle, ignore
			if (names.count(s)!=0) continue;
			
			to.emplace(
				std::move(lowercase),
				&s
			);
		
		}
		
		//	Scan connected users to find users
		//	with the usernames of our recipients
		if (to.size()!=0) for (auto & client : server.Clients) {
		
			//	We're only interested in clients
			//	in game
			if (client->GetState()!=ProtocolState::Play) continue;
			
			//	Check to see if we're sending to
			//	this user by name
			auto iter=to.find(client->GetUsername().ToLower());
			if (iter!=to.end()) {
			
				//	YES
			
				//	Send
				client->Send(packet);
				
				//	Remove from collection
				//
				//	The users left in the collection
				//	will be those users to whom
				//	we could not deliver
				to.erase(iter);
			
			}
		
		}
		
		//	Deliver to users we're delivering to
		//	by handle
		for (auto & client : message.Recipients) const_cast<SmartPointer<Client> &>(client)->Send(packet);
		
		//	Build the list of recipients who could
		//	not be found/delivered to
		for (const auto & pair : to) retr.Add(*pair.second);
		
		//	Log
		Log(message,retr);
		
		return retr;
	
	}


	static Singleton<Chat> singleton;
	
	
	Chat & Chat::Get () noexcept {
	
		return singleton.Get();
	
	}
	
	
}


extern "C" {


	Module * Load () {
	
		return &(Chat::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

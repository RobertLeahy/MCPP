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
		)=[this] (PacketEvent event) mutable {

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
	
	
	typedef Packets::Play::Clientbound::ChatMessage packet_type;
	
	
	static ChatMessage copy (const ChatMessage & message, const Vector<ChatStyle> & stack) {
	
		//	Copy everything except the actual
		//	body of the message
		ChatMessage retr;
		retr.From=message.From;
		retr.To=message.To;
		retr.Recipients=message.Recipients;
		retr.Echo=message.Echo;
		
		for (auto s : stack) retr << s;
		
		return retr;
	
	}
	
	
	static Vector<String> get_lines (const String & str) {
	
		Vector<String> retr;
	
		Vector<CodePoint> curr;
		bool trailing=false;
		for (auto cp : str.CodePoints()) {
		
			//	The "trailing" variable monitors
			//	for a trailing newline, which requires
			//	the insertion of an empty string at
			//	the end
			//
			//	Since another iteration of the loop implies
			//	that there's a character after the newline,
			//	we reset the variable to false on each
			//	iteration
			trailing=false;
		
			//	Skip carriage returns
			if (cp=='\r') continue;
			
			//	Append on newline
			if (cp=='\n') {
			
				retr.EmplaceBack(std::move(curr));
				
				trailing=true;
				
				continue;
			
			}
			
			//	Otherwise just add code point
			//	and move on
			curr.Add(cp);
		
		}
		
		//	Append last if necessary
		if (
			(curr.Count()!=0) ||
			trailing
		) retr.EmplaceBack(std::move(curr));
		
		return retr;
	
	}
	
	
	static Vector<packet_type> preprocess (const ChatMessage & message) {
	
		//	Maintain our own stack
		Vector<ChatStyle> stack;
		
		//	Chat messages we've split this chat
		//	message into
		Vector<ChatMessage> messages;
		//	Start with one message
		messages.Add(copy(message,stack));
		
		//	Loop over each element in the
		//	message we were passed in
		for (auto & token : message.Message) {
			
			switch (token.Type) {
			
				//	Maintain our own stack by intercepting
				//	push and pop commands
				case ChatFormat::Push:
					stack.Add(token.Style);
					break;
				case ChatFormat::Pop:
					if (stack.Count()!=0) stack.Delete(stack.Count()-1);
					break;
				//	Check segments for new lines, and create
				//	new messages for each newline
				case ChatFormat::Segment:{
				
					auto lines=get_lines(token.Segment);
					
					for (Word i=0;i<lines.Count();++i) {
					
						//	If this isn't the first line, create a
						//	new message
						if (i!=0) messages.Add(copy(message,stack));
						
						//	Skip empty strings
						if (lines[i].Size()==0) continue;
						
						messages[messages.Count()-1] << lines[i];
					
					}
				
				}continue;
				default:break;
			
			}
			
			messages[messages.Count()-1] << token;
		
		}
		
		//	Parse messages into packets
		Vector<packet_type> retr;
		for (auto & m : messages) {
		
			packet_type packet;
			packet.Value=Chat::Format(m);
			
			retr.Add(std::move(packet));
			
		}
		
		return retr;
	
	}
	
	
	static void send (SmartPointer<Client> client, const Vector<packet_type> & packets) {
	
		if (!client.IsNull()) client->Atomic([&] () mutable {	for (auto & packet : packets) client->Send(packet);	});
	
	}
	
	
	static const String server_username("SERVER");
	
	
	Vector<String> Chat::Send (const ChatMessage & message) {
	
		//	As of 1.7.2 Mojang has broken the client's
		//	rendering of newline characters.  The client
		//	now renders them as "LF" in a box, rather
		//	than actually inserting a line break.
		//
		//	Therefore a workaround is necessary.  This
		//	preprocess method takes and breaks a single
		//	ChatMessage into many ChatMessages, each one
		//	containing a single line.  It then creates
		//	a packet from each of these ChatMessages.
		//
		//	By using the Atomic method on the Client
		//	object, we're able to send all these packets
		//	without any risk of other packets being sent
		//	in between.
		//
		//	It's objectively bad design, but it's not
		//	my fault -- I'm just working around Mojang's
		//	poor design decisions.
		auto packets=preprocess(message);
		
		//	List of clients to whom the message
		//	could not be delivered
		Vector<String> retr;
		
		//	Return to sender if this is
		//	an echo message
		if (message.Echo && !message.From.IsNull()) {
		
			send(message.From,packets);
		
			return retr;
		
		}
		
		auto & server=Server::Get();
		
		//	If this is a broadcast, simply send to everyone
		if (
			(message.To.Count()==0) &&
			(message.Recipients.Count()==0)
		) {
		
			for (auto & client : server.Clients) send(client,packets);
			
			Log(message);
			
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
				send(client,packets);
				
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
		for (auto & client : message.Recipients) send(client,packets);
		
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

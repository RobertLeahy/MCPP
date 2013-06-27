#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>
#include <limits>
#include <algorithm>


namespace MCPP {


	static const String name("Chat Support");
	static const Word priority=1;
	static const String mods_dir("chat_mods");
	static const String log_prepend("Chat Support: ");
	static const String protocol_error("Protocol error");
	
	
	static Nullable<ModuleLoader> mods;
	
	
	ChatModule::ChatModule () {
	
		//	Fire up mod loader
		mods.Construct(
			Path::Combine(
				Path::GetPath(
					File::GetCurrentExecutableFileName()
				),
				mods_dir
			),
			[&] (const String & message, Service::LogType type) {
			
				String log(log_prepend);
				log << message;
				
				RunningServer->WriteLog(
					log,
					type
				);
			
			}
		);
	
	}
	
	
	ChatModule::~ChatModule () noexcept {
	
		mods->Unload();
	
	}


	const String & ChatModule::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word ChatModule::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void ChatModule::Install () {
	
		//	Get mods
		mods->Load();
	
		//	Install ourself into the server
		//	first, so that mods we load
		//	can overwrite our changes
		
		//	We need to handle 0x03
		
		//	Grab the old handler
		PacketHandler prev(
			std::move(
				RunningServer->Router[0x03]
			)
		);
		
		//	Install our handler
		RunningServer->Router[0x03]=[=] (SmartPointer<Client> client, Packet packet) {
		
			//	Unauthenticated users cannot chat
			if (client->GetState()!=ClientState::Authenticated) {
			
				//	Chain to the old handler
				if (prev) prev(
					std::move(client),
					std::move(packet)
				);
				//	...or kill the client
				else client->Disconnect(protocol_error);
				
				//	Don't execute any of
				//	the following code
				return;
			
			}
			
			//	If mods installed a handler,
			//	call it.
			if (Chat) Chat(
				client,
				packet.Retrieve<String>(0)
			);
			
			//	Chain to the old handle if
			//	applicable
			if (prev) prev(
				std::move(client),
				std::move(packet)
			);
		
		};
	
		//	Install mods
		mods->Install();
	
	}
	
	
	static inline void insert_sorted (Vector<Tuple<String,bool>> & list, String username) {
	
		for (Word i=0;i<list.Count();++i) {
		
			if (list[i].Item<0>()==username) return;
			
			if (list[i].Item<0>()>username) {
			
				list.Emplace(
					i,
					std::move(username),
					false
				);
				
				return;
			
			}
		
		}
		
		list.EmplaceBack(
			std::move(username),
			false
		);
	
	}
	
	
	static inline void flag_found (Vector<Tuple<String,bool>> & list, const String & username) {
	
		for (auto & t : list) {
		
			if (t.Item<0>()==username) {
			
				t.Item<1>()=true;
				
				return;
			
			}
		
		}
	
	}
	
	
	static inline bool is_recipient (const Vector<Tuple<String,bool>> & list, const String & username) {
	
		for (const auto & t : list) {
		
			if (t.Item<0>()==username) return true;
		
		}
		
		return false;
	
	}
	
	
	Vector<String> ChatModule::Send (const ChatMessage & message) {
	
		//	Build the packet
		Packet packet;
		packet.SetType<pt>();
		packet.Retrieve<pt,0>()=Format(message);
		
		//	Return to sender if applicable
		if (message.Echo && !message.From.IsNull()) {
		
			const_cast<ChatMessage &>(message).From->Send(packet);
			
			return Vector<String>();
		
		}
		
		//	Normalize the list of
		//	recipients and create
		//	a boolean flag for each
		//	one so that we can track
		//	whether or not we've delivered
		//	to them
		Vector<
			Tuple<
				String,
				bool
			>
		> recipients;
		//	Insert recipients sorted to decrease
		//	lookup time
		for (const auto & s : message.To) insert_sorted(
			recipients,
			s.ToLower()
		);
		for (const auto & c : message.Recipients) insert_sorted(
			recipients,
			c->GetUsername().ToLower()
		);
		
		//	Scan the list of connected
		//	users and send the message
		//	as appropriate
		if (
			(message.To.Count()!=0) ||
			(message.Recipients.Count()==0)
		) RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
		
			//	Skip all clients we have the handle
			//	for
			for (const auto & handle : message.Recipients) {
			
				if (static_cast<const Client *>(handle)==static_cast<Client *>(client)) return;
			
			}
			
			//	We're only interested in authenticated
			//	clients
			if (client->GetState()==ClientState::Authenticated) {
			
				//	If this is a broadcast, send unconditionally
				if (recipients.Count()==0) {
				
					client->Send(packet);
				
				//	See if this recipients is among
				//	the intended recipients
				} else {
				
					String username(client->GetUsername().ToLower());
					
					if (is_recipient(
						recipients,
						username
					)) {
					
						client->Send(packet);
						
						flag_found(
							recipients,
							username
						);
					
					}
				
				}
			
			}
		
		});
		
		//	Send to all the people we have the handle
		//	for
		for (auto & handle : message.Recipients) {
		
			const_cast<SmartPointer<Client> &>(handle)->Send(packet);
			
			flag_found(
				recipients,
				handle->GetUsername().ToLower()
			);
		
		}
		
		//	Build a list of clients to whom
		//	delivery failed
		Vector<String> dne;
		for (auto & t : recipients) {
		
			if (!t.Item<1>()) dne.Add(std::move(t.Item<0>()));
		
		}
		
		//	Log
		Log(message,dne);
		
		//	Return
		return dne;
	
	}


	Nullable<ChatModule> Chat;
	
	
}


extern "C" {


	Module * Load () {
	
		try {
		
			if (Chat.IsNull()) {
			
				//	Make sure the mod loader
				//	is uninitialized
				mods.Destroy();
				
				//	Create chat module
				Chat.Construct();
			
			}
			
			return &(*Chat);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		Chat.Destroy();
		
		if (!mods.IsNull()) mods->Unload();
	
	}
	
	
	void Cleanup () {
	
		mods.Destroy();
	
	}


}

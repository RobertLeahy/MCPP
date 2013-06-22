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
		for (const auto & s : message.To) {
		
			recipients.EmplaceBack(
				s.ToLower(),
				false
			);
		
		}
		
		//	Scan the list of connected
		//	users and send the message
		//	as appropriate
		RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
		
			//	Only interested in authenticated
			//	clients
			if (client->GetState()==ClientState::Authenticated) {
			
				//	If this is a broadcast, send
				//	unconditionally
				if (recipients.Count()==0) {
				
					client->Send(packet);
				
				//	See if this is an intended
				//	recipient
				} else {
			
					//	Normalize username to lowercase
					String username(client->GetUsername().ToLower());
				
					//	Scan the list of recipients,
					//	is this client in there?
					for (auto & t : recipients) {
					
						if (username==t.Item<0>()) {
						
							//	Send
							client->Send(packet);
						
							//	Flag as found
							t.Item<1>()=true;
						
							//	Found -- we're done
							break;
						
						}
					
					}
					
				}
			
			}
		
		});
		
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
	
	}


}

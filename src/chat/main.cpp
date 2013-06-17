#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>
#include <limits>


namespace MCPP {


	static const String name("Chat Support");
	static const Word priority=1;
	static const String mods_dir("chat_mods");
	static const String log_prepend("Chat Support: ");
	static const String protocol_error("Protocol error");
	
	
	String ChatModule::Sanitize (String subject, bool escape) {
	
		//	Determine the longest the string
		//	can be in a safe manner
		Word max_len;
		try {

			max_len=Word(
				SafeInt<Int16>(
					std::numeric_limits<Int16>::max()
				)
			);
		
		} catch (...) {
		
			max_len=std::numeric_limits<Word>::max();
		
		}
		
		//	Normalize to reduce number
		//	of code points (allows us
		//	to send more)
		subject.Normalize(NormalizationForm::NFC);
	
		Vector<CodePoint> sanitized;
		Word i=0;
		for (CodePoint cp : subject.CodePoints()) {
		
			//	Do not copy further
			//	code points if we've
			//	reached the maximum length
			if (i==max_len) break;
		
			if (
				//	Check for section sign
				(
					escape &&
					(cp==0xA7)
				) ||
				//	Check for code points outside
				//	the BMP
				(cp>0xFFFF)
			) continue;
			
			sanitized.Add(cp);
			
			++i;
		
		}
		
		//	Return newly-formed string
		return String(std::move(sanitized));
	
	}
	
	
	ChatModule::ChatModule () : mods(
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
	) {	}


	const String & ChatModule::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word ChatModule::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void ChatModule::Install () {
	
		//	Get mods
		mods.Load();
	
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
		mods.Install();
	
	}
	
	
	void ChatModule::Broadcast (const String & message) const {

		RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
		
			if (client->GetState()==ClientState::Authenticated) {
			
				Packet packet;
				packet.SetType<PacketTypeMap<0x03>>();
				packet.Retrieve<String>(0)=message;
			
				client->Send(packet);
			
			}
		
		});
	
	}
	
	
	Vector<String> ChatModule::Send (const Vector<String> & usernames, const String & message) const {
	
		//	A list of users that couldn't
		//	be found
		Vector<String> dne;
		
		//	Packet to send
		Packet packet;
		packet.SetType<PacketTypeMap<0x03>>();
		packet.Retrieve<String>(0)=message;
		
		//	Scan the list of recipients
		for (const auto & s : usernames) {
		
			//	Did we find this recipient?
			bool found=false;
		
			//	Scan connected users
			RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
			
				//	This is a user we can and have
				//	been requested to send to
				if (
					(client->GetState()==ClientState::Authenticated) &&
					(client->GetUsername()==s)
				) {
				
					//	Send
					
					client->Send(packet);
					
					found=true;
				
				}
			
			});
		
			//	Add to DNE list if applicable
			if (!found) dne.Add(s);
		
		}
		
		return dne;
	
	}


	Nullable<ChatModule> Chat;
	
	
}


extern "C" {


	Module * Load () {
	
		if (Chat.IsNull()) try {
		
			Chat.Construct();
			
			return &(*Chat);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		Chat.Destroy();
	
	}


}

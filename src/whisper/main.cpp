#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>


static const String name("Whisper Support");
static const Word priority=2;
static const String whisper_template("§d§l{0} whispers:§r§d {1}");
static const String whisper_log_template("[{0}] whispers [{1}]: {2}");
static const Regex whisper_regex(
	"^\\/w\\s+(\\w+)\\s+(.+)$",
	RegexOptions()
		.SetIgnoreCase()	//	The "w" shouldn't be case sensitive
		.SetSingleline()	//	If the client for some reason sends
							//	a bunch of newlines we want to match
							//	that.
);
static const String whisper_dne_template(
	"§c§l"	//	Bold and red
	"SERVER:"	//	Server error message
	"§r§c"	//	Not-bold and red
	" User "
	"§c§l"	//	Bold and red
	"{0}"	//	Non-existent username
	"§r§c"	//	Not-bold and red
	" does not exist"
);
static const String whisper_sent_template(
	"§d§l"	//	Bold and pink
	"You whisper {0}:"	//	Label
	"§r§d"	//	Not-bold and pink
	" {1}"	//	The message
);


class Whisper : public Module {


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			//	Extract previous handler
			ChatHandler prev(std::move(Chat->Chat));
			
			//	Install our handler
			Chat->Chat=[=] (SmartPointer<Client> client, const String & message) {
			
				//	See if the regex matches
				auto match=whisper_regex.Match(message);
				
				//	Proceed if the capture suceeded
				if (match.Success()) {
				
					String recipient(match[1].Value());
					String message(match[2].Value());
					String sender(client->GetUsername());
					
					//	Log
					RunningServer->WriteLog(
						String::Format(
							whisper_log_template,
							sender,
							recipient,
							message
						),
						Service::LogType::Information
					);
					
					if (Chat->Send(
						recipient,
						ChatModule::Sanitize(
							String::Format(
								whisper_template,
								sender,
								message
							),
							false
						)
					)) {
					
						//	Recipient exists and
						//	message was sent
						Chat->Send(
							std::move(client),
							ChatModule::Sanitize(
								String::Format(
									whisper_sent_template,
									std::move(recipient),
									std::move(message)
								),
								false
							)
						);
					
					} else {
					
						//	Recipient does not exist
						
						Chat->Send(
							std::move(client),
							ChatModule::Sanitize(
								String::Format(
									whisper_dne_template,
									std::move(recipient)
								),
								false
							)
						);
					
					}
				
				//	If the capture didn't succeed
				//	chain
				} else {
				
					if (prev) prev(
						std::move(client),
						message
					);
				
				}
			
			};
		
		}


};


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		if (mod_ptr==nullptr) try {
		
			mod_ptr=new Whisper();
		
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

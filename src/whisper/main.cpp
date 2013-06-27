#include <common.hpp>
#include <chat/chat.hpp>
#include <utility>


static const String name("Whisper Support");
static const Word priority=2;
static const Regex whisper_regex(
	"^\\/w\\s+(\\w+)\\s+(.+)$",
	RegexOptions()
		.SetIgnoreCase()	//	The "w" shouldn't be case sensitive
		.SetSingleline()	//	If the client for some reason sends
							//	a bunch of newlines we want to match
							//	that.
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
				
					ChatMessage message(
						client,
						match[1].Value(),
						match[2].Value()
					);
					
					//	Send
					if (Chat->Send(message).Count()==0) {
					
						//	Delivery success
						
						//	Send it back to the original
						//	sender
						message.Echo=true;
						
						Chat->Send(message);
					
					} else {
					
						//	Recipient does not exist
						
						ChatMessage error;
						error.AddRecipients(client);
						error.Message.EmplaceBack(ChatColour::Red);
						error.Message.EmplaceBack(ChatFormat::Label);
						error.Message.EmplaceBack(ChatFormat::LabelSeparator);
						error.Message.EmplaceBack(" User ");
						error.Message.EmplaceBack(ChatStyle::Bold);
						error.Message.EmplaceBack(match[1].Value());
						error.Message.EmplaceBack(ChatFormat::PopStyle);
						error.Message.EmplaceBack(" does not exist");
						
						//	Send
						Chat->Send(error);
					
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


static Nullable<Whisper> whisper;


extern "C" {


	Module * Load () {
	
		try {
		
			if (whisper.IsNull()) whisper.Construct();
			
			return &(*whisper);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		whisper.Destroy();
	
	}


}

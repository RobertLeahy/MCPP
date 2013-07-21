#include <common.hpp>
#include <op/op.hpp>
#include <chat/chat.hpp>
#include <utility>


static const String name("Chat Op/Deop Support");
static const Word priority=2;
static const Regex op_regex(
	"^\\/op\\s+(.*)$",
	RegexOptions().SetIgnoreCase().SetSingleline()
);
static const Regex deop_regex(
	"^\\/deop\\s+(.*)$",
	RegexOptions().SetIgnoreCase().SetSingleline()
);


class ChatOp : public Module {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	Extract previous handler
			//	for chaining
			ChatHandler prev(std::move(Chat->Chat));
			
			//	Install our handler
			Chat->Chat=[=] (SmartPointer<Client> client, const String & message) {
			
				//	Only proceed if user-in-question
				//	is op.
				//
				//	Non-ops can't op or deop
				if (Ops->IsOp(client->GetUsername())) {
				
					//	Opping?
					auto match=op_regex.Match(message);
					
					//	Yes
					if (match.Success()) {
					
						//	Add op status
						Ops->Op(match[1].Value());
						
						ChatMessage message;
						message	<<	ChatStyle::BrightGreen
								<<	ChatStyle::Bold
								<<	client->GetUsername()
								<<	ChatFormat::Pop
								<<	" opped "
								<<	ChatStyle::Bold
								<<	match[1].Value();
								
						Chat->Send(message);
					
						return;
					
					}
					
					//	Deopping?
					match=deop_regex.Match(message);
					
					if (match.Success()) {
					
						//	Remove op status
						Ops->DeOp(match[1].Value());
						
						ChatMessage message;
						message <<	ChatStyle::Red
								<<	ChatStyle::Bold
								<<	client->GetUsername()
								<<	ChatFormat::Pop
								<<	" deopped "
								<<	ChatStyle::Bold
								<<	match[1].Value();
								
						Chat->Send(message);
								
						return;
					
					}
				
				}
				
				//	Chain if applicable
				if (prev) prev(
					std::move(client),
					message
				);
			
			};
		
		}


};


static Nullable<ChatOp> chat_op;


extern "C" {


	Module * Load () {
	
		try {
		
			chat_op.Construct();
			
			return &(*chat_op);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		chat_op.Destroy();
	
	}


}

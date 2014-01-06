#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <mod.hpp>
#include <server.hpp>


using namespace MCPP;


static const String name("Chat Login/Logout Broadcast");
static const Word priority=1;
static const String online(" has come online");
static const String offline(" has gone offline");
static const String start_reason(" (");
static const String end_reason(")");


class LoginLogout : public Module {


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			auto & server=Server::Get();
			
			//	Subscribe to the on login event
			server.OnLogin.Add([] (SmartPointer<Client> client) {
			
				ChatMessage message;
				message	<<	ChatStyle::BrightGreen
						<<	ChatStyle::Bold
						<<	client->GetUsername()
						<<	ChatFormat::Pop
						<<	online;
						
				Chat::Get().Send(message);
			
			});
			
			//	Subscribe to the on disconnect event
			server.OnDisconnect.Add([] (SmartPointer<Client> client, const String & reason) {
			
				//	Only proceed if the user was authenticated
				if (client->GetState()!=ProtocolState::Play) return;
				
				ChatMessage message;
				message	<<	ChatStyle::Red
						<<	ChatStyle::Bold
						<<	client->GetUsername()
						<<	ChatFormat::Pop
						<<	offline;
						
				//	If there was a reason associated with the disconnect,
				//	append that
				if (reason.Size()!=0) message	<<	start_reason
												<<	ChatStyle::Bold
												<<	reason
												<<	ChatFormat::Pop
												<<	end_reason;
												
				Chat::Get().Send(message);
			
			});
		
		}


};


INSTALL_MODULE(LoginLogout)

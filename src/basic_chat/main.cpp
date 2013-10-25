#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <chat_provider.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <utility>


using namespace MCPP;


static const String name("Basic Chat Support");
static const Word priority=1;


class BasicChat : public ChatProvider, public Module {


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			//	Install our chat handler
			Chat::Get().Chat=[] (ChatEvent event) {
			
				//	Send
				Chat::Get().Send(
					ChatMessage(
						std::move(event.From),
						event.Body
					)
				);
			
			};
			
			//	Install ourselves as the chat
			//	handler into the server
			Server::Get().SetChatProvider(this);
		
		}
		
		
		virtual void Send (const String & message) override {
		
			Chat::Get().Send(
				ChatMessage(
					SmartPointer<Client>(),
					message
				)
			);
		
		}


};


INSTALL_MODULE(BasicChat)

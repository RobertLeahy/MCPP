#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <info/info.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <network.hpp>


using namespace MCPP;


static const String name("Connection Handler Information");
static const Word priority=1;
static const String identifier("handler");
static const String help("Displays information about the server's connection handler.");
static const String label_separator(": ");


static const String handler_banner("MAIN SERVER CONNECTION HANDLER:");
static const String sent_label("Bytes Sent");
static const String received_label("Bytes Received");
static const String outgoing_label("Successful Outgoing Connections");
static const String incoming_label("Successful Incoming Connections");
static const String accepted_label("Connections Accepted");
static const String disconnected_label("Connections Terminated");
static const String listening_label("Listening Sockets");
static const String connected_label("Connected Sockets");
static const String workers_label("Number of Worker Threads");


class HandlerInfo : public Module, public InformationProvider {


	private:
	
	
		template <typename T>
		static void line (ChatMessage & message, const String & label, const T & data) {
		
			message	<<	Newline
					<<	ChatStyle::Bold
					<<	label
					<<	label_separator
					<<	ChatFormat::Pop
					<<	data;
		
		}


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			Information::Get().Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			//	Get data about the connection handler
			auto info=Server::Get().Handler().GetInfo();
			
			message	<<	ChatStyle::Bold
					<<	handler_banner
					<<	ChatFormat::Pop;
					
			line(message,sent_label,info.Sent);
			line(message,received_label,info.Received);
			line(message,accepted_label,info.Accepted);
			line(message,disconnected_label,info.Disconnected);
			line(message,incoming_label,info.Incoming);
			line(message,outgoing_label,info.Outgoing);
			line(message,listening_label,info.Listening);
			line(message,connected_label,info.Connected);
			line(message,workers_label,info.Workers);
		
		}


};


INSTALL_MODULE(HandlerInfo)

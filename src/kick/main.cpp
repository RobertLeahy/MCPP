#include <rleahylib/rleahylib.hpp>
#include <command/command.hpp>
#include <chat/chat.hpp>
#include <permissions/permissions.hpp>
#include <mod.hpp>
#include <server.hpp>


using namespace MCPP;


static const String name("Kick Command");
static const Word priority=1;
static const String identifier("kick");
static const String summary("Kicks a player.");
static const String help(
	"Syntax: /kick <player name>\n"
	"Kicks the player whose name is given, disconnecting them from the server."
);
static const String reason("Kicked");
static const String by(" by {0}");


class Kick : public Module, public Command {


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			Commands::Get().Add(
				identifier,
				this
			);
		
		}
		
		
		virtual void Summary (const String &, ChatMessage & message) override {

			message << summary;
		
		}
		
		
		virtual void Help (const String &, ChatMessage & message) override {
		
			message << help;
		
		}
		
		
		virtual Vector<String> AutoComplete (const CommandEvent & event) override {
			
			if (event.Arguments.Count()!=1) return Vector<String>();
			
			auto clients=Server::Get().Clients.Get(
				event.Arguments[0],
				ClientSearch::Begin
			);
			
			auto retr=Vector<String>(clients.Count());
			for (auto & client : clients) retr.Add(
				client->GetUsername()
			);
			
			return retr;
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			return event.Issuer.IsNull() ? true : Permissions::Get().GetUser(event.Issuer).Check(event.Identifier);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
			
			//	There needs to be an argument
			if (event.Arguments.Count()==0) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Create the reason
			String reason(::reason);
			//	If this command was issued by
			//	a connected user, we report who
			//	it is
			if (!event.Issuer.IsNull()) reason << String::Format(
				by,
				event.Issuer->GetUsername()
			);
			
			//	Get all clients with the given
			//	username.  We use the raw arguments
			//	so that we get the exact name that
			//	the issuer specified
			for (auto & client : Server::Get().Clients.Get(event.RawArguments)) client->Disconnect(reason);
			
			return retr;
		
		}


};


INSTALL_MODULE(Kick)

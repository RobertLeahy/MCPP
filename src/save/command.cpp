#include <chat/chat.hpp>
#include <command/command.hpp>
#include <permissions/permissions.hpp>
#include <save/save.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <exception>
#include <utility>


using namespace MCPP;


static const Word priority=1;
static const String name("Save Commands");
static const String identifier("save");
static const String on("on");
static const String off("off");


class SaveCommand : public Module, public Command {


	private:
	
	
		static void save (SmartPointer<Client> issuer) {
		
			try {
		
				ChatMessage msg;
				msg.Recipients.Add(std::move(issuer));
				msg	<<	ChatStyle::BrightGreen
					<<	ChatStyle::Bold
					<<	"Saving...";
				
				auto & chat=Chat::Get();
				
				chat.Send(msg);
				
				Timer timer(Timer::CreateAndStart());
				
				SaveManager::Get()();
				
				auto elapsed=timer.ElapsedNanoseconds();
				
				msg.Message=Vector<ChatToken>();
				msg	<<	ChatStyle::BrightGreen
					<<	ChatStyle::Bold
					<<	"Saved"
					<<	ChatFormat::Pop
					<<	" (took "
					<<	elapsed
					<<	"ns)";
					
				chat.Send(msg);
				
			} catch (...) {
			
				try {
				
					Server::Get().Panic(std::current_exception());
				
				} catch (...) {	}
				
				throw;
			
			}
		
		}
		
		
		static ChatMessage enable () {
		
			ChatMessage retr;
			retr	<<	ChatStyle::Red
					<<	ChatStyle::Bold
					<<	"Saving resumed";
					
			SaveManager::Get().Resume();
			
			return retr;
		
		}
		
		
		static ChatMessage disable () {
		
			ChatMessage retr;
			retr	<<	ChatStyle::BrightGreen
					<<	ChatStyle::Bold
					<<	"Saving paused";
			
			SaveManager::Get().Pause();
			
			return retr;
		
		}
		
		
		static CommandResult enable_disable (CommandEvent event) {
		
			CommandResult retr;
		
			if (event.Arguments[0]==on) {
			
				retr.Message=enable();
				
			} else if (event.Arguments[0]==off) {
			
				retr.Message=disable();
				
			} else {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			retr.Status=CommandStatus::Success;
			
			return retr;
		
		}


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {

			return name;

		}
		
		
		virtual void Install () override {
		
			Commands::Get().Add(
				identifier,
				this
			);
		
		}
		
		
		virtual void Summary (const String &, ChatMessage & message) override {
		
			message << "Manipulates the save system.";
		
		}
		
		
		virtual void Help (const String &, ChatMessage & message) override {
		
			message	<<	"Syntax: "
					<<	ChatStyle::Bold
					<<	"/"
					<<	identifier
					<<	" [on|off]"
					<<	ChatFormat::Pop
					<<	Newline
					<<	"If executed with no arguments, immediately performs a save.  "
						"If executed with the \"on\" argument, enables periodic automatic saving.  "
						"If executed with the \"off\" argument, disables periodic automatic saving.";
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			if (event.Issuer.IsNull()) return true;
			
			return Permissions::Get().GetUser(event.Issuer).Check(identifier);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
			
			switch (event.Arguments.Count()) {
			
				case 0:
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wpedantic"
					Server::Get().Pool().Enqueue([client=std::move(event.Issuer)] () mutable {	save(std::move(client));	});
					#pragma GCC diagnostic pop
					retr.Status=CommandStatus::Success;
					break;
				case 1:
					retr=enable_disable(std::move(event));
					break;
				default:
					retr.Status=CommandStatus::SyntaxError;
					break;
			
			}
			
			return retr;
		
		}


};


INSTALL_MODULE(SaveCommand)

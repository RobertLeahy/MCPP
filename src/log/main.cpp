#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <command/command.hpp>
#include <op/op.hpp>
#include <concurrency_manager.hpp>
#include <data_provider.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <utility>


using namespace MCPP;


static const Word priority=1;
static const String name("Logging to Chat");
static const String identifier("log");
static const String summary("Sends server log entries through chat.");
static const String help(
	"Syntax: /log\n"
	"If server log entries are not being sent through chat to the invoker of this command, begins sending them.\n"
	"If server log entries are being sent through chat to the invoker of this command, stops sending them."
);


class LogToChat : public Module, public Command {


	private:
	
	
		std::unordered_set<SmartPointer<Client>> clients;
		std::unordered_set<SmartPointer<Client>> logging;
		Mutex lock;
		Nullable<ConcurrencyManager> cm;
		std::atomic<bool> capture;
	
	
	public:
	
	
		LogToChat () noexcept {
		
			capture=false;
		
		}
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			auto & server=Server::Get();
		
			//	Construct the concurrency manager
			cm.Construct(
				server.Pool(),
				1,
				[] () {	Server::Get().Panic();	}
			);
			
			//	Hook into server events
			server.OnConnect.Add([this] (SmartPointer<Client> client) mutable {
			
				lock.Execute([&] () {	clients.insert(std::move(client));	});
			
			});
			
			server.OnDisconnect.Add([this] (SmartPointer<Client> client, const String &) mutable {
			
				lock.Execute([&] () {
				
					clients.erase(client);
					
					if (
						(logging.erase(client)!=0) &&
						(logging.size()==0)
					) capture=false;
					
				});
			
			});
			
			server.OnLog.Add([this] (const String & log_str, Service::LogType type) mutable {
			
				if (capture) cm->Enqueue([=] () {
				
					//	Get human readable string for the
					//	log type
					String type_str=DataProvider::GetLogType(type).ToUpper();
					
					//	Begin preparing a chat message
					ChatMessage message;
					
					//	Choose a colour for the type of
					//	message we're logging
					switch (type) {
					
						case Service::LogType::Success:
						case Service::LogType::SecuritySuccess:
							message << ChatStyle::BrightGreen;
							break;
							
						case Service::LogType::Error:
						case Service::LogType::Critical:
						case Service::LogType::Emergency:
						case Service::LogType::SecurityFailure:
							message << ChatStyle::Red;
							break;
							
						case Service::LogType::Warning:
						case Service::LogType::Alert:
							message << ChatStyle::Yellow;
							break;
							
						case Service::LogType::Information:
						case Service::LogType::Debug:
						default:break;
					
					}
					
					message	<<	ChatStyle::Bold
							<<	type_str
							<<	":"
							<<	Newline
							<<	ChatFormat::Pop
							<<	ChatFormat::Pop
							<<	log_str;
							
					//	Figure out who we're sending this to
					if (!lock.Execute([&] () {
					
						if (logging.size()==0) return false;
						
						for (const auto & client : logging) message.AddRecipients(client);
						
						return true;
					
					})) return;
					
					Chat::Get().Send(message);
				
				});
			
			});
			
			//	Add ourselves to the list of
			//	installed commands
			Commands::Get().Add(this);
		
		}
	
	
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual bool Check (SmartPointer<Client> client) const override {
		
			return Ops::Get().IsOp(client->GetUsername());
		
		}
		
		
		virtual const String & Summary () const noexcept override {
		
			return summary;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual bool Execute (SmartPointer<Client> client, const String &, ChatMessage &) override {
		
			//	Guard against nulls (in case this
			//	command is invoked through the
			//	interpreter)
			if (client.IsNull()) return true;
		
			lock.Execute([&] () {
			
				//	Guard against race conditions
				if (clients.count(client)!=0) {
				
					if (logging.erase(client)==0) {
					
						logging.insert(
							std::move(client)
						);
						
						capture=true;
					
					} else if (logging.size()==0) {
					
						capture=false;
					
					}
				
				}
			
			});
			
			//	Unconditionally return true -- no syntax
			//	parsing and therefore no syntax errors
			return true;
		
		}


};


static Nullable<LogToChat> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <command/command.hpp>
#include <op/op.hpp>
#include <data_provider.hpp>
#include <mod.hpp>


static const String name("Configuration Manipulation Command");
static const Word priority=1;
static const String identifier("set");
static const String summary("Sets or unsets configuration keys.");
static const String help(
	"Syntax: /set <key> [<value>]\n"
	"Changes or clears a setting in the backing store.\n"
	"The server may have to be rebooted for these changes to take effect, depending on the setting changed.\n"
	"If \"value\" is not specified, clears \"key\", otherwise sets \"key\" to \"value\"."
);
static const Regex parse("[^\\s]+");
static const String deleted_template("\"{0}\" cleared");
static const String set_template("\"{0}\" set to \"{1}\"");


class Set : public Module, public Command {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			Commands::Get().Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual const String & Summary () const noexcept override {
		
			return summary;
		
		}
		
		
		virtual bool Check (SmartPointer<Client> client) const override {
		
			return Ops::Get().IsOp(client->GetUsername());
		
		}
		
		
		virtual bool Execute (SmartPointer<Client>, const String & args, ChatMessage & message) override {
		
			auto matches=parse.Matches(args);
			
			//	If exactly one match, we're
			//	clearing a setting
			if (matches.Count()==1) {
			
				auto key=matches[0].Value();
			
				Server::Get().Data().DeleteSetting(key);
				
				message	<<	ChatStyle::Yellow
						<<	ChatStyle::Bold
						<<	String::Format(
								deleted_template,
								key
							);
			
				return true;
			
			}
			
			//	If exactly two matches, we're
			//	setting a setting
			if (matches.Count()==2) {
			
				auto key=matches[0].Value();
				auto value=matches[1].Value();
				
				Server::Get().Data().SetSetting(
					key,
					value
				);
				
				message	<<	ChatStyle::Yellow
						<<	ChatStyle::Bold
						<<	String::Format(
								set_template,
								key,
								value
							);
							
				return true;
			
			}
			
			//	If we fall through to here,
			//	the syntax of the command was
			//	invalid
			return false;
		
		}


};


INSTALL_MODULE(Set)

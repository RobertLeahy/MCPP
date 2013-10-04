#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <command/command.hpp>
#include <op/op.hpp>
#include <data_provider.hpp>
#include <mod.hpp>


static const String name("Configuration Display Command");
static const Word priority=1;
static const String identifier("get");
static const String summary("Displays configuration keys.");
static const String help(
	"Syntax: /get <key>\n"
	"Retrieves a setting from the backing store."
);
static const String unset_template("\"{0}\" is not set");
static const String display_template("\"{0}\" has the value \"{1}\"");


class Get : public Module, public Command {


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
		
			auto trimmed=args.Trim();
			
			if (trimmed.Size()==0) return false;
			
			auto value=Server::Get().Data().GetSetting(trimmed);
			
			message	<<	ChatStyle::Yellow
					<<	ChatStyle::Bold
					<<	(
							value.IsNull()
								?	String::Format(
										unset_template,
										trimmed
									)
								:	String::Format(
										display_template,
										trimmed,
										*value
									)
						);
						
			return true;
		
		}


};


INSTALL_MODULE(Get)

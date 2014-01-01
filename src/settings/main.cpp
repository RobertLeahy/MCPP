#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <command/command.hpp>
#include <permissions/permissions.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <stdexcept>
#include <utility>


using namespace MCPP;


static const String name("Configuration Manipulation Command");
static const Word priority=1;
static const String get_identifier("get");
static const String set_identifier("set");
static const String set_summary("Sets configuration settings in the backing store.");
static const String get_summary("Displays configuration settings from the backing store.");
static const String set_help(
	"Syntax: /set <name> [<value>]\n"
	"Sets the configuration setting given by \"name\" to \"value\".\n"
	"If \"value\" is not given, clears the configuration setting given by \"name\".\n"
	"Note that in some cases the server will have to be restarted for changes to take effect."
);
static const String get_help(
	"Syntax: /get <name>\n"
	"Displays the configuration setting given by \"name\".\n"
	"Note that settings displayed by this command are taken directly from the backing store, and may not reflect the configuration settings currently in effect."
);
static const String not_set []={
	"Setting \"",
	"\" is not set"
};
static const String value []={
	"Setting \"",
	"\" has value \"",
	"\""
};
static const Regex set_regex("^\\S+\\s+(\\S.*)$");
static const char * parsing_irregularity="Irregularity while parsing arguments";
static const String cleared []={
	"Setting \"",
	"\" cleared"
};
static const String value_set []={
	"Setting \"",
	"\" set to \"",
	"\""
};
static const String took(" (took {0}ns)");


class Settings : public Module, public Command {


	private:
	
	
		template <Word, Word n>
		static void interpolate_impl (ChatMessage & message, const String (& arr) [n]) {
		
			message << arr[n-1];
		
		}
		
		
		template <Word i, Word n, typename T, typename... Args>
		static void interpolate_impl (ChatMessage & message, const String (& arr) [n], T && curr, Args &&... args) {
		
			message	<<	arr[i]
					<<	ChatStyle::Bold
					<<	curr
					<<	ChatFormat::Pop;
					
			interpolate_impl<i+1>(
				message,
				arr,
				std::forward<Args>(args)...
			);
		
		}
	
	
		template <Word n, typename... Args>
		static void interpolate (ChatMessage & message, const String (& arr) [n], Args &&... args) {
		
			interpolate_impl<0>(
				message,
				arr,
				std::forward<Args>(args)...
			);
		
		}
	
	
		static bool is_set (const String & identifier) {
		
			return identifier==set_identifier;
		
		}
		
		
		static CommandResult set (CommandEvent event) {
		
			CommandResult retr;
			
			//	If there are no arguments, that's unconditionally
			//	a syntax error
			if (event.Arguments.Count()==0) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Command will now always succeed
			retr.Status=CommandStatus::Success;
			
			//	Get the data provider, we'll need it
			//	regardless of what happens now
			auto & data=Server::Get().Data();
			
			const auto & setting=event.Arguments[0];
			
			//	If there's just the one argument, we're
			//	clearing a setting
			if (event.Arguments.Count()==1) {
			
				Timer timer(Timer::CreateAndStart());
				data.DeleteSetting(setting);
				timer.Stop();
				
				interpolate(
					retr.Message,
					cleared,
					setting
				);
				retr.Message << String::Format(
					took,
					timer.ElapsedNanoseconds()
				);
				
				return retr;
				
			}
			
			//	Otherwise we're actually setting a setting
			//	to some value
			
			//	We extract the raw second argument onwards,
			//	preserving exactly what the user typed
			auto match=set_regex.Match(event.RawArguments);
			//	If the match isn't a success, that's a traumatic
			//	failure
			if (!match.Success()) throw std::runtime_error(parsing_irregularity);
			
			const auto & value=match[1].Value();
			
			//	Set the setting
			Timer timer(Timer::CreateAndStart());
			data.SetSetting(
				setting,
				value
			);
			timer.Stop();
			
			interpolate(
				retr.Message,
				value_set,
				setting,
				value
			);
			retr.Message << String::Format(
				took,
				timer.ElapsedNanoseconds()
			);
			
			return retr;
		
		}
		
		
		static CommandResult get (CommandEvent event) {
		
			CommandResult retr;
		
			//	The only requirement is that there be
			//	one argument, which is the name of the
			//	configuration setting which is to be
			//	retrieved
			if (event.Arguments.Count()!=1) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Command will now unconditionally succeed
			retr.Status=CommandStatus::Success;
			
			const auto & setting=event.Arguments[0];
			
			//	Get the configuration setting from the
			//	backing store
			Timer timer(Timer::CreateAndStart());
			auto result=Server::Get().Data().GetSetting(setting);
			timer.Stop();
			
			//	Produce result
			if (result.IsNull()) interpolate(
				retr.Message,
				not_set,
				setting
			);
			else interpolate(
				retr.Message,
				value,
				setting,
				*result
			);
			
			retr.Message << String::Format(
				took,
				timer.ElapsedNanoseconds()
			);
			
			return retr;
		
		}


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			auto & commands=Commands::Get();
			
			commands.Add(
				get_identifier,
				this
			);
			commands.Add(
				set_identifier,
				this
			);
		
		}
		
		
		virtual void Summary (const String & identifier, ChatMessage & message) override {
		
			message << (is_set(identifier) ? set_summary : get_summary);
		
		}
		
		
		virtual void Help (const String & identifier, ChatMessage & message) override {
		
			message << (is_set(identifier) ? set_help : get_help);
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			return event.Issuer.IsNull() ? true : Permissions::Get().GetUser(event.Issuer).Check(event.Identifier);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			return is_set(event.Identifier) ? set(std::move(event)) : get(std::move(event));
		
		}


};


INSTALL_MODULE(Settings)

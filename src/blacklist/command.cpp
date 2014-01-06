#include <blacklist/blacklist.hpp>
#include <chat/chat.hpp>
#include <command/command.hpp>
#include <permissions/permissions.hpp>
#include <ip_address_range.hpp>
#include <mod.hpp>
#include <utility>


using namespace MCPP;


static const Word priority=1;
static const String name("Blacklist Command");
static const String identifier("blacklist");
static const String add_arg("add");
static const String remove_arg("remove");
static const String quiet("-q");
static const String regex_template("^{0}");


static const Regex mask_regex("^\\s*(.*?)\\s+mask\\s+(.*?)\\s*$");
static const Regex cidr_regex("^\\s*(.*?)\\/(.*?)\\s*$");
static const Regex range_regex("^\\s*(.*?)\\-(.*?)\\s*$");


class BlacklistCommand : public Module, public Command {


	private:
	
	
		class Arguments {
		
		
			public:
			
			
				//	Whether the command should proceed
				//	quietly
				bool Quiet;
				//	Whether the command should add or
				//	remove
				bool Add;
				//	The IP address range that should be
				//	added or removed (as applicable)
				IPAddressRange Range;
		
		
		};
		
		
		static Nullable<IPAddressRange> get_range (const String * begin, const String * end) {
		
			Nullable<IPAddressRange> retr;
			
			//	Merge all the disjoint arguments
			//	into one argument which can then
			//	be parsed
			String str;
			bool first=true;
			for (;begin!=end;++begin) {
			
				if (first) first=false;
				else str << " ";
				
				str << *begin;
			
			}
			
			//	Attempt to extract a mask
			auto match=mask_regex.Match(str);
			if (match.Success()) {
			
				try {
				
					retr=IPAddressRange::CreateMask(
						IPAddress(match[1].Value()),
						IPAddress(match[2].Value())
					);
				
				} catch (...) {	}
				
				return retr;
			
			}
			
			//	Attempt to extract a CIDR formatted
			//	range
			match=cidr_regex.Match(str);
			if (match.Success()) {
			
				Word bits;
				if (!match[2].Value().ToInteger(&bits)) return retr;
			
				try {
				
					retr.Construct(
						IPAddress(match[1].Value()),
						bits
					);
				
				} catch (...) {	}
				
				return retr;
			
			}
			
			//	Attempt to extract a start/end delimited
			//	range
			match=range_regex.Match(str);
			if (match.Success()) {
			
				try {
				
					retr.Construct(
						IPAddress(match[1].Value()),
						IPAddress(match[2].Value())
					);
				
				} catch (...) {	}
				
				return retr;
			
			}
			
			//	Attempt to extract a simple IP address
			try {
			
				retr.Construct(IPAddress(str));
			
			} catch (...) {	}
			
			return retr;
		
		}
		
		
		static Nullable<Arguments> parse (Vector<String> args) {
		
			Nullable<Arguments> retr;
			
			//	We can't do anything with no arguments
			if (args.Count()==0) return retr;
			
			auto begin=args.begin();
			auto end=args.end();
			
			retr.Construct();
			
			//	Attempt to parse quiet argument
			if (*begin==quiet) {
			
				retr->Quiet=true;
				
				++begin;
			
			} else {
			
				retr->Quiet=false;
			
			}
			
			//	If we ran out of arguments, fail
			if (begin==end) goto fail;
			
			//	Try and determine whether we're
			//	adding or removing
			if (*begin==add_arg) retr->Add=true;
			else if (*begin==remove_arg) retr->Add=false;
			else goto fail;
			
			if ((++begin)==end) goto fail;
			
			//	Try and get the range that we'll be
			//	adding or removing
			{
			
				auto range=get_range(begin,end);
				if (range.IsNull()) goto fail;
				
				retr->Range=std::move(*range);
			
			}
			
			return retr;
			
			fail:
			retr.Destroy();
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
		
			Commands::Get().Add(
				identifier,
				this
			);
		
		}
		
		
		virtual void Summary (const String &, ChatMessage & message) override {
		
			message	<<	"Adds or removes IPs or IP ranges to/from the blacklist.";
		
		}
		
		
		virtual void Help (const String &, ChatMessage & message) override {
		
			message	<<	"Syntax: "
					<<	ChatStyle::Bold
					<<	"/"
					<<	identifier
					<<	" ["
					<<	quiet
					<<	"] "
					<<	add_arg
					<<	"|"
					<<	remove_arg
					<<	" <IP|IP range>"
					<<	ChatFormat::Pop
					<<	Newline
					<<	"If "
					<<	add_arg
					<<	" is specified, adds an IP or IP range to the blacklist.  If "
					<<	remove_arg
					<<	" is specified, removes an IP or IP range from the blacklist.  If "
					<<	quiet
					<<	" is specified, does not broadcast a notification of this action.";
		
		}
		
		
		virtual Vector<String> AutoComplete (const CommandEvent & event) override {
		
			Vector<String> retr;
			
			if (event.Arguments.Count()>2) return retr;
			
			Word loc;
			if (event.Arguments.Count()==2) {
			
				if (event.Arguments[0]!=quiet) return retr;
			
				loc=1;
			
			} else {
			
				loc=0;
			
			}
			
			Regex regex(
				String::Format(
					regex_template,
					Regex::Escape(event.Arguments[loc])
				)
			);
			
			if (regex.IsMatch(add_arg)) retr.Add(add_arg);
			if (regex.IsMatch(remove_arg)) retr.Add(remove_arg);
			if ((loc==0) && regex.IsMatch(quiet)) retr.Add(quiet);
			
			return retr;
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			if (event.Issuer.IsNull()) return true;
			
			return Permissions::Get().GetUser(event.Issuer).Check(identifier);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
			
			auto args=parse(std::move(event.Arguments));
			
			if (args.IsNull()) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			retr.Status=CommandStatus::Success;
			
			auto & blacklist=Blacklist::Get();
			
			ChatMessage message;
			message << ChatStyle::Bold;
			
			if (args->Add) {
			
				message	<<	ChatStyle::Red
						<<	"Adding "
						<<	String(args->Range)
						<<	" to";
						
				blacklist.Add(std::move(args->Range));
			
			} else {
			
				message	<<	ChatStyle::BrightGreen
						<<	"Removing "
						<<	String(args->Range)
						<<	" from";
						
				blacklist.Remove(std::move(args->Range));
			
			}
			
			message << " blacklist";
			
			if (args->Quiet) retr.Message=std::move(message);
			else Chat::Get().Send(message);
			
			return retr;
		
		}


};


INSTALL_MODULE(BlacklistCommand)

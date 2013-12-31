#include <command/command.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <utility>


using namespace MCPP;


namespace MCPP {


	static const Word priority=2;
	static const String name("Command Support");


	//	Precedes all help output
	static const String help_banner("====HELP====");
	//	Prefix that precedes commands
	static const String prefix("/");
	//	Separates command from its summary in help
	static const String summary_separator(" - ");
	//	Starts off the chat message that's sent when
	//	incorrect syntax is used
	static const String incorrect_syntax_label("Incorrect syntax");
	//	Introduces "/help" in incorrect syntax strings
	static const String try_label(" try ");
	//	The identifier for the help command
	static const String help_identifier("help");
	//	A space
	static const String separator(" ");
	//	Informs the user that they are not permitted
	//	to execute a command
	static const String not_permitted("You are not permitted to do that");
	//	Informs the user that a command they attempted
	//	to execute does not exist
	static const String does_not_exist("Command does not exist");
	//	Gives help with the help command
	static const String help_help("For help with a specific command type ");
	//	The syntax of the help command is /help and then this
	static const String help_syntax("<command>");
	
	
	//	Parses a command
	static const Regex parse("^\\/(\\S*)(?:\\s*(.*))?$");
	//	Splits the arguments to a command
	//	on whitespace
	static const Regex split("(?<=\\s|^)\\S+(?=\\s|$)");
	//	Checks to see if there's trailing whitespace
	static const Regex trailing_whitespace("\\s$");


	Command * Commands::get (const String & identifier) {
	
		auto iter=map.find(identifier);
		
		return (iter==map.end()) ? nullptr : iter->second;
	
	}
	
	
	ChatMessage Commands::incorrect_syntax () {
	
		ChatMessage retr;
		retr	<<	ChatStyle::Red
				<<	ChatStyle::Bold
				<<	incorrect_syntax_label
				<<	ChatFormat::Pop
				<<	try_label
				<<	ChatStyle::Bold
				<<	prefix
				<<	help_identifier;
		
		return retr;
	
	}
	
	
	ChatMessage Commands::incorrect_syntax (const String & identifier) {
	
		auto retr=incorrect_syntax();
		
		retr << separator << identifier;
		
		return retr;
	
	}
	
	
	ChatMessage Commands::command_dne () {
	
		ChatMessage retr;
		retr	<<	ChatStyle::Red
				<<	ChatStyle::Bold
				<<	does_not_exist;
		
		return retr;
	
	}
	
	
	ChatMessage Commands::insufficient_privileges () {
	
		ChatMessage retr;
		retr	<<	ChatStyle::Red
				<<	ChatStyle::Bold
				<<	not_permitted;
		
		return retr;
	
	}
	
	
	ChatMessage Commands::help (CommandEvent event) {
	
		ChatMessage retr;
		retr	<<	ChatStyle::Yellow
				<<	ChatStyle::Bold
				<<	help_banner
				<<	ChatFormat::Pop
				<<	Newline;
				
		if (event.Arguments.Count()==0) {
		
			//	If there are no arguments, we retrieve
			//	a listing and summary of all commands
			
			retr	<<	help_help
					<<	ChatStyle::Bold
					<<	prefix
					<<	help_identifier
					<<	separator
					<<	help_syntax
					<<	ChatFormat::Pop		//	Get rid of bold
					<<	ChatFormat::Pop;	//	Get rid of yellow (from above)
			
			//	Loop and only show user commands that
			//	they are permitted to use
			for (const auto & c : list) {
			
				event.Identifier=c.Item<0>();
			
				if (c.Item<1>()->Check(event)) {
			
					retr	<<	Newline
							<<	ChatStyle::Bold
							<<	prefix
							<<	c.Item<0>()
							<<	ChatFormat::Pop
							<<	summary_separator;
							
					c.Item<1>()->Summary(c.Item<0>(),retr);
					
				}
			
			}
		
		} else if (event.Arguments.Count()==1) {
		
			//	Attempt to find and deliver help specific
			//	to this command
			
			event.Identifier=event.Arguments[0];
			auto command=get(event.Identifier);
			
			//	Make sure command exists
			if (command==nullptr) return command_dne();
			
			//	Make sure user can view this command
			if (!command->Check(event)) return insufficient_privileges();
			
			//	Output
			retr	<<	ChatStyle::Bold
					<<	prefix
					<<	event.Identifier
					<<	ChatFormat::Pop	//	Get rid of bold
					<<	ChatFormat::Pop	//	Get rid of yellow (from above)
					<<	Newline;
					
			command->Help(event.Identifier,retr);
		
		} else {
		
			//	If there is more than one argument,
			//	that's an error
			return incorrect_syntax();
		
		}
		
		return retr;
	
	}
	
	
	Nullable<CommandEvent> Commands::parse (SmartPointer<Client> client, const String & str, bool keep_trailing) {
	
		Nullable<CommandEvent> retr;
	
		//	If it's not actually a command, return
		//	at once
		auto match=::parse.Match(str);
		if (!match.Success()) return retr;
		
		//	Split the arguments
		auto raw_args=match[2].Value();
		auto matches=split.Matches(raw_args);
		Vector<String> args;
		for (auto & match : matches) args.Add(match.Value());
		
		//	If we're keeping trailing whitespace, check
		//	for it
		if (
			keep_trailing &&
			trailing_whitespace.IsMatch(str)
		) args.EmplaceBack();
		
		retr=CommandEvent{
			std::move(client),
			match[1].Value(),
			std::move(args),
			str,
			std::move(raw_args)
		};
		
		return retr;
	
	}
	
	
	Nullable<ChatMessage> Commands::execute (SmartPointer<Client> client, const String & str) {
	
		Nullable<ChatMessage> retr;
	
		//	Attempt to parse the command
		auto event=parse(
			std::move(client),
			str
		);
		
		//	If the parse failed, it's not
		//	actually a command
		if (event.IsNull()) return retr;
		
		//	Is it the help command?
		if (event->Identifier==help_identifier) {
		
			retr=help(std::move(*event));
			
			return retr;
		
		}
		
		//	Attempt to look up this command
		auto command=get(event->Identifier);
		
		//	If there's no such command, return that
		if (command==nullptr) {
		
			retr=command_dne();
			
			return retr;
		
		}
		
		//	Ensure command processing should proceed
		if (!command->Check(*event)) {
		
			retr=insufficient_privileges();
			
			return retr;
		
		}
		
		//	Save the identifier -- we'll need it if
		//	the syntax is incorrect
		auto identifier=event->Identifier;
		
		//	Otherwise execute the command
		auto result=command->Execute(std::move(*event));
		
		switch (result.Status) {
		
			case CommandStatus::Success:
				retr=std::move(result.Message);
				break;
			case CommandStatus::SyntaxError:
			default:
				retr=incorrect_syntax(identifier);
				break;
			case CommandStatus::DoesNotExist:
				retr=command_dne();
				break;
			case CommandStatus::Forbidden:
				retr=insufficient_privileges();
				break;
		
		}
		
		return retr;
	
	}
	
	
	Vector<String> Commands::auto_complete (SmartPointer<Client> client, const String & str) {
	
		Vector<String> retr;
	
		//	Parse
		auto event=parse(
			std::move(client),
			str,
			true
		);
		
		//	If the parse failed, return at once
		if (event.IsNull()) return retr;
		
		//	If the parse actually extracted a full
		//	command identifier, we try and find that
		//	command to get auto completions, otherwise
		//	we try and autocomplete to a command
		//	identifier
		
		if (event->Arguments.Count()==0) {
		
			//	Construct a regular expression which
			//	matches the stem the user gave
			Regex regex(
				String::Format(
					"^{0}",
					Regex::Escape(event->Identifier)
				)
			);
			
			//	Loop over each command and create list
			//	of suggestions based on matches
			for (const auto & t : list) if (
				regex.IsMatch(t.Item<0>()) &&
				//	Make sure this user can actually run
				//	this command
				t.Item<1>()->Check(*event)
			) retr.Add(t.Item<0>());
		
		} else {
		
			//	Attempt to get command
			auto command=get(event->Identifier);
			
			if (
				//	If the command could not be found,
				//	return at once
				(command==nullptr) ||
				//	If the user can't actually execute the
				//	command, return at once
				!command->Check(*event)
			) return retr;
			
			//	Get auto completions
			retr=command->AutoComplete(*event);
		
		}
		
		return retr;
	
	}
	
	
	static Singleton<Commands> singleton;
	
	
	Commands & Commands::Get () noexcept {
	
		return singleton.Get();
	
	}
	
	
	const String & Commands::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Commands::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void Commands::Install () {
	
		typedef Packets::Play::Clientbound::TabComplete reply;
		typedef Packets::Play::Serverbound::TabComplete request;
		
		auto & server=Server::Get();
	
		//	Install auto complete handler
		server.Router(
			request::PacketID,
			request::State
		)=[this] (PacketEvent event) mutable {
		
			auto packet=event.Data.Get<request>();
			
			reply p;
			p.Match=auto_complete(event.From,packet.Text);
			
			event.From->Send(p);
		
		};
		
		//	Install chat handler
		
		auto & chat=Chat::Get();
		
		//	Save previous handler
		auto prev=std::move(chat.Chat);
		
		//	Install new handler
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpedantic"
		chat.Chat=[this,prev=std::move(prev)] (ChatEvent event) mutable {
		
			//	Attempt to execute as a command
			auto result=execute(event.From,event.Body);
			
			if (result.IsNull()) {
			
				//	If parsing/execution fails, attempt
				//	to pass through
				if (prev) prev(std::move(event));
			
			//	Sending empty messages crashes client
			} else if (result->Message.Count()!=0) {
			
				//	If parsing/execute succeeds, send
				//	response
				
				result->Recipients.Add(std::move(event.From));
				
				Chat::Get().Send(*result);
			
			}
		
		};
		#pragma GCC diagnostic pop
		
		//	Install as command interpreter into
		//	the server
		server.SetCommandInterpreter(this);
	
	}
	
	
	Nullable<String> Commands::operator () (const String & str) {
	
		Nullable<String> retr;
		
		auto result=execute(
			SmartPointer<Client>(),
			String("/")+str
		);
		
		if (!result.IsNull()) retr=Chat::ToString(*result);
		
		return retr;
	
	}
	
	
	void Commands::Add (String identifier, Command * command) {
	
		//	Don't proceed with adding null pointers
		if (command==nullptr) return;
	
		//	Find insertion point in vector
		Word loc=0;
		for (
			;
			(loc<list.Count()) &&
			(list[loc].Item<0>()<identifier);
			++loc
		);
		
		//	Delete duplicate command if
		//	present
		auto iter=map.find(identifier);
		if (iter!=map.end()) {
		
			map.erase(iter);
			list.Delete(loc);
		
		}
		
		//	Add
		list.Emplace(
			loc,
			identifier,
			command
		);
		try {
		
			map.emplace(
				std::move(identifier),
				command
			);
		
		} catch (...) {
		
			//	Roll back on throw
			
			list.Delete(loc);
			
			throw;
		
		}
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(Commands::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

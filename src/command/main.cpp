#include <command/command.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <utility>
#include <algorithm>


using namespace MCPP;


namespace MCPP {


	static const String name("Command Support");
	static const Word priority=2;
	static const String mods_dir("command_mods");
	static const String log_prepend("Command Support: ");
	static const String help_command("help");
	static const Regex parse(
		"^([^\\s]+)(?:\\s+(.*))?$",
		RegexOptions().SetSingleline()
	);
	static const Regex is_command(
		"^\\/(.*)$",
		RegexOptions().SetSingleline()
	);
	static const Regex contains_whitespace("\\s");
	static const String protocol_error("Protocol error");
	
	
	const String & Commands::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Commands::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	inline Command * Commands::retrieve (const String & id) {
	
		for (auto * c : commands)
		if (c->Identifier()==id)
		return c;
		
		return nullptr;
	
	}
	
	
	inline Vector<Command *> Commands::retrieve (const Regex & regex) {
	
		Vector<Command *> retr;
		
		for (auto * c : commands)
		if (regex.IsMatch(c->Identifier()))
		retr.Add(c);
		
		return retr;
	
	}
	
	
	ChatMessage Commands::incorrect_syntax (Command * command) {
	
		ChatMessage message;
		message	<< ChatStyle::Red
				<< ChatStyle::Bold
				<< "Incorrect syntax"
				<< ChatFormat::Pop
				<< " try "
				<< ChatStyle::Bold
				<< "/help";
				
		if (command!=nullptr) message << " " << command->Identifier();
		
		return message;
	
	}
	
	
	ChatMessage Commands::command_dne () {
	
		ChatMessage message;
		message	<< ChatStyle::Red
				<< ChatStyle::Bold
				<< "Command does not exist";
				
		return message;
	
	}
	
	
	ChatMessage Commands::insufficient_privileges () {
	
		ChatMessage message;
		message	<< ChatStyle::Red
				<< ChatStyle::Bold
				<< "You are not permitted to do that";
				
		return message;
	
	}
	
	
	ChatMessage Commands::help (SmartPointer<Client> client, const String & id) {
	
		ChatMessage message;
		message	<< ChatStyle::Yellow
				<< ChatStyle::Bold
				<< "====HELP===="
				<< ChatFormat::Pop
				<< Newline;
	
		if (id.Size()==0) {
		
			//	Retrieve command listing and
			//	summary
			
			message	<< "For help with a specific command type "
					<< ChatStyle::Bold
					<< "/help <command>"
					<< ChatFormat::Pop	//	Get rid of bold
					<< ChatFormat::Pop;	//	Get rid of yellow
					
			//	Loop for all commands
			for (const auto * c : commands) {
			
				//	Only show the user help for
				//	commands that they can
				//	use
				if (
					client.IsNull() ||
					c->Check(client)
				) {
			
					message	<< Newline
							<< ChatStyle::Bold
							<< "/"
							<< c->Identifier()
							<< ChatFormat::Pop
							<< " - "
							<< c->Summary();
							
				}
			
			}
		
		} else {
		
			//	Help for a specific command
			
			//	Attempt to retrieve command
			auto * command=retrieve(id);
			
			//	Command does not exist
			if (command==nullptr) return command_dne();
			
			//	User cannot use this command
			if (!(
				client.IsNull() ||
				command->Check(client)
			)) return insufficient_privileges();
			
			message	<< ChatStyle::Bold
					<< "/"
					<< id
					<< ChatFormat::Pop	//	Get rid of bold
					<< ChatFormat::Pop	//	Get rid of yellow
					<< Newline
					<< command->Help();
		
		}
		
		return message;
	
	}
	
	
	ChatMessage Commands::execute (SmartPointer<Client> client, const String & cmd, const String & args) {
	
		//	Is this help?
		if (cmd==help_command) return help(
			std::move(client),
			args
		);
		
		//	Attempt to retrieve corresponding
		//	command
		auto * command=retrieve(cmd);
		if (command==nullptr) return command_dne();
		
		//	Can this user execute this command?
		if (!(
			client.IsNull() ||
			command->Check(client)
		)) return insufficient_privileges();
		
		//	Execute command
		ChatMessage message;
		if (!command->Execute(
			client,
			args,
			message
		)) return incorrect_syntax(command);
		
		return message;
	
	}
	
	
	ChatMessage Commands::parse_and_execute (SmartPointer<Client> client, const String & args) {
	
		//	Attempt to parse
		auto parsed=parse.Match(args);
		//	Could not parse -- incorrect syntax
		if (!parsed.Success()) return incorrect_syntax(nullptr);
		
		//	EXECUTE
		return execute(
			std::move(client),
			parsed[1].Value(),
			(parsed[2].Count()==0) ? String() : parsed[2].Value()
		);
	
	}
	
	
	String Commands::get_command (const String & msg) {
	
		auto match=is_command.Match(msg);
		if (match.Success()) return match[1].Value();
		
		return String();
	
	}
	
	
	void Commands::Add (Command * command) {
	
		if (command!=nullptr) {
		
			//	Find insertion point
			Word i=0;
			for (
				;
				(i<commands.Count()) &&
				(command->Identifier()>commands[i]->Identifier());
				++i
			);
			
			//	Insert in sorted order
			commands.Insert(
				command,
				i
			);
			
		}
	
	}
	
	
	void Commands::Install () {
		
		//	Install our handlers
		
		//	Handle chat events
		auto chat=std::move(Chat::Get().Chat);
		Chat::Get().Chat=[=] (SmartPointer<Client> client, const String & msg) {
		
			//	Is this chat message a command?
			auto cmd=get_command(msg);
			if (cmd.Size()!=0) {
			
				//	YES
			
				auto message=parse_and_execute(client,cmd);
				
				//	If there's a message to send,
				//	send it
				if (message.Message.Count()!=0) {
				
					//	Don't add on a recipient if the
					//	command itself already specified
					//	recipients
					if (
						(message.To.Count()==0) &&
						(message.Recipients.Count()==0)
					) message.AddRecipients(std::move(client));
					
					Chat::Get().Send(message);
				
				}
			
			//	NO, attempt to forward call through
			} else if (chat) {
			
				chat(
					std::move(client),
					msg
				);
			
			}
		
		};
		
		auto & server=Server::Get();
		
		//	Handle auto-complete events
		auto complete=std::move(server.Router[0xCB]);
		server.Router[0xCB]=[=] (SmartPointer<Client> client, Packet packet) {
		
			typedef PacketTypeMap<0xCB> pt;
			
			//	Make sure client is authenticated
			if (client->GetState()!=ClientState::Authenticated) {
			
				//	Chain through if possible
				if (complete) complete(
					std::move(client),
					std::move(packet)
				);
				//	Nothing to chain through to, kill the
				//	client
				else client->Disconnect(protocol_error);
				
				return;
			
			}
			
			//	Is this a command?
			auto match=is_command.Match(
				packet.Retrieve<pt,0>()
			);
			if (match.Success()) {
			
				//	YES
				
				//	Does it contain whitespace?
				if (contains_whitespace.IsMatch(match[1].Value())) {
				
					//	YES -- it's a command and we need
					//	to auto complete its arguments
					
					auto parsed=parse.Match(match[1].Value());
					
					const auto * command=retrieve(parsed[1].Value());
					//	No command by that name, or user
					//	doesn't have access to that command,
					//	ignore
					if (
						(command==nullptr) ||
						!command->Check(client)
					) return;
					
					auto & second=parsed[2];
					
					//	Get auto completions
					auto completions=command->AutoComplete(
						(second.Count()==0) ? String() : second.Value()
					);
					
					//	No completions, ignore
					if (completions.Count()==0) return;
					
					//	Send auto completions to client
					Packet packet;
					packet.SetType<pt>();
					auto & str=packet.Retrieve<pt,0>();
					for (const auto & c : completions) {
					
						if (str.Size()!=0) str << GraphemeCluster('\0');
						
						str << c;
					
					}
					client->Send(packet);
				
				} else {
				
					//	NO -- we need to autocomplete the command
					//	itself
					
					auto commands=retrieve(
						Regex(
							String::Format(
								"^{0}",
								Regex::Escape(
									match[1].Value()
								)
							)
						)
					);
					//	No possible auto completions, ignore
					if (commands.Count()==0) return;
					
					//	Send auto completions to client
					Packet packet;
					packet.SetType<pt>();
					auto & str=packet.Retrieve<pt,0>();
					for (const auto * c : commands)
					if (c->Check(client)) {
					
						if (str.Size()!=0) str << GraphemeCluster('\0');
						
						str << "/" << c->Identifier();
					
					}
					if (str.Size()!=0) client->Send(packet);
				
				}
			
			//	NO, attempt to pass through
			} else if (complete) {
			
				complete(
					std::move(client),
					std::move(packet)
				);
			
			}
		
		};
		
		//	We are the server's command interpreter,
		//	until and unless we're overwritten by
		//	another module
		server.SetCommandInterpreter(this);
	
	}
	
	
	Nullable<String> Commands::operator () (const String & command) {
	
		//	Execute command
		auto message=parse_and_execute(
			SmartPointer<Client>(),
			command
		);
		
		Nullable<String> retr;
		
		if (message.Message.Count()!=0) retr.Construct(
			Chat::Format(
				message,
				false
			)
		);
		
		return retr;
	
	}
	
	
	bool Command::Check (SmartPointer<Client>) const {
	
		return true;
	
	}
	
	
	Vector<String> Command::AutoComplete (const String &) const {
	
		return Vector<String>();
	
	}
	
	
	static Singleton<Commands> singleton;
	
	
	Commands & Commands::Get () noexcept {
	
		return singleton.Get();
	
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

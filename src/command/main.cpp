#include <command/command.hpp>
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
	
	
	void Commands::incorrect_syntax (SmartPointer<Client> client, Command * command) {
	
		ChatMessage message;
		message.AddRecipients(std::move(client));
		message	<< ChatStyle::Red
				<< ChatStyle::Bold
				<< "Incorrect syntax"
				<< ChatFormat::Pop
				<< " try "
				<< ChatStyle::Bold
				<< "/help";
				
		if (command!=nullptr) message << " " << command->Identifier();
		
		Chat::Get().Send(message);
	
	}
	
	
	void Commands::command_dne (SmartPointer<Client> client) {
	
		ChatMessage message;
		message.AddRecipients(std::move(client));
		message	<< ChatStyle::Red
				<< ChatStyle::Bold
				<< "Command does not exist";
				
		Chat::Get().Send(message);
	
	}
	
	
	void Commands::insufficient_privileges (SmartPointer<Client> client) {
	
		ChatMessage message;
		message.AddRecipients(std::move(client));
		message	<< ChatStyle::Red
				<< ChatStyle::Bold
				<< "You are not permitted to do that";
				
		Chat::Get().Send(message);
	
	}
	
	
	void Commands::help (SmartPointer<Client> client, const String & id) {
	
		ChatMessage message;
		message.AddRecipients(client);
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
				if (c->Check(client)) {
			
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
			
			if (command==nullptr) {
			
				//	Command does not exist
			
				command_dne(std::move(client));
				
				return;
			
			}
			
			if (!command->Check(client)) {
			
				//	User cannot use command
				
				insufficient_privileges(std::move(client));
				
				return;
			
			}
			
			message	<< ChatStyle::Bold
					<< "/"
					<< id
					<< ChatFormat::Pop	//	Get rid of bold
					<< ChatFormat::Pop	//	Get rid of yellow
					<< Newline
					<< command->Help();
		
		}
		
		Chat::Get().Send(message);
	
	}
	
	
	void Commands::Add (Command * command) {
	
		if (command!=nullptr) {
		
			//	Find insertion point
			Word i=0;
			for (
				;
				(i<commands.Count()) &&
				(command->Identifier()<commands[i]->Identifier());
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
			auto match=is_command.Match(msg);
			if (match.Success()) {
			
				//	YES
			
				//	Attempt to parse
				auto parsed=parse.Match(match[1].Value());
				if (!parsed.Success()) {
				
					//	Could not parse, incorrect syntax
				
					incorrect_syntax(std::move(client),nullptr);
					
					return;
				
				}
				
				auto & second=parsed[2];
				
				//	Is this help?
				if (parsed[1].Value()==help_command) {
				
					//	Yes
					
					help(
						std::move(client),
						(second.Count()==0) ? String() : second.Value()
					);
					
					return;
				
				}
				
				//	Attempt to retrieve corresponding
				//	command
				auto * command=retrieve(parsed[1].Value());
				if (command==nullptr) {
				
					//	Command does not exist
				
					command_dne(std::move(client));
					
					return;
				
				}
				
				//	Can this user execute this command?
				if (!command->Check(client)) {
				
					//	No, insufficient privileges
				
					insufficient_privileges(std::move(client));
					
					return;
				
				}
				
				//	Execute command
				ChatMessage message;
				if (!command->Execute(
					client,
					(second.Count()==0) ? String() : second.Value(),
					message
				)) {
				
					//	Incorrect syntax
					
					incorrect_syntax(std::move(client),command);
				
				}
				
				//	Send if appropriate
				if (message.Message.Count()!=0) {
				
					message.AddRecipients(std::move(client));
				
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
		
		//	Handle auto-complete events
		auto complete=std::move(Server::Get().Router[0xCB]);
		Server::Get().Router[0xCB]=[=] (SmartPointer<Client> client, Packet packet) {
		
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

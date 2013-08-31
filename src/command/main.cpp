#include <command/command.hpp>
#include <utility>
#include <algorithm>


namespace MCPP {


	static const String name("Command Support");
	static const Word priority=2;
	static const String mods_dir("command_mods");
	static const String log_prepend("Command Support: ");
	static const String help_command("help");
	static const Regex parse("^([^\\s]+)(?:\\s+(.*))?$");
	static const Regex is_command("^\\/(.*)$");
	static const Regex contains_whitespace("\\s");
	
	
	static Nullable<ModuleLoader> mods;
	
	
	CommandModule::CommandModule () {
	
		//	Fire up mod loader
		mods.Construct(
			Path::Combine(
				Path::GetPath(
					File::GetCurrentExecutableFileName()
				),
				mods_dir
			),
			[] (const String & message, Service::LogType type) {
			
				String log(log_prepend);
				log << message;
				
				RunningServer->WriteLog(
					log,
					type
				);
			
			}
		);
	
	}
	
	
	CommandModule::~CommandModule () noexcept {
	
		mods->Unload();
	
	}
	
	
	const String & CommandModule::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word CommandModule::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	inline Command * CommandModule::retrieve (const String & id) {
	
		for (auto * c : commands)
		if (c->Identifier()==id)
		return c;
		
		return nullptr;
	
	}
	
	
	inline Vector<Command *> CommandModule::retrieve (const Regex & regex) {
	
		Vector<Command *> retr;
		
		for (auto * c : commands)
		if (regex.IsMatch(c->Identifier()))
		retr.Add(c);
		
		return retr;
	
	}
	
	
	void CommandModule::incorrect_syntax (SmartPointer<Client> client, Command * command) {
	
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
		
		Chat->Send(message);
	
	}
	
	
	void CommandModule::command_dne (SmartPointer<Client> client) {
	
		ChatMessage message;
		message.AddRecipients(std::move(client));
		message	<< ChatStyle::Red
				<< ChatStyle::Bold
				<< "Command does not exist";
				
		Chat->Send(message);
	
	}
	
	
	void CommandModule::insufficient_privileges (SmartPointer<Client> client) {
	
		ChatMessage message;
		message.AddRecipients(std::move(client));
		message	<< ChatStyle::Red
				<< ChatStyle::Bold
				<< "You are not permitted to do that";
				
		Chat->Send(message);
	
	}
	
	
	void CommandModule::help (SmartPointer<Client> client, const String & id) {
	
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
		
		Chat->Send(message);
	
	}
	
	
	void CommandModule::Add (Command * command) {
	
		if (command!=nullptr) commands.Add(command);
	
	}
	
	
	void CommandModule::Install () {
	
		//	Get mods
		mods->Load();
		
		//	Install our handlers
		
		//	Handle chat events
		auto chat=std::move(Chat->Chat);
		Chat->Chat=[=] (SmartPointer<Client> client, const String & msg) {
		
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
				
					Chat->Send(message);
					
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
		auto complete=std::move(RunningServer->Router[0xCB]);
		RunningServer->Router[0xCB]=[=] (SmartPointer<Client> client, Packet packet) {
		
			typedef PacketTypeMap<0xCB> pt;
			
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
		
		//	Install mods
		mods->Install();
		
		//	Sort commands so they appear
		//	in alphabetical order in calls
		//	to help
		std::sort(
			commands.begin(),
			commands.end(),
			[] (const Command * a, const Command * b) {
			
				return a->Identifier()<b->Identifier();
			
			}
		);
	
	}
	
	
	Nullable<CommandModule> Commands;


}


extern "C" {


	Module * Load () {
	
		try {
		
			if (Commands.IsNull()) {
			
				mods.Destroy();
				
				Commands.Construct();
			
			}
			
			return &(*Commands);
		
		} catch (...) { }
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		Commands.Destroy();
		
		if (!mods.IsNull()) mods->Unload();
	
	}
	
	
	void Cleanup () {
	
		mods.Destroy();
	
	}


}

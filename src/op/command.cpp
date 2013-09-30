#include <op/op.hpp>
#include <command/command.hpp>
#include <chat/chat.hpp>


static const String op_identifier("op");
static const String deop_identifier("deop");
static const String op_summary("Elevates a player to server operator.");
static const String deop_summary("Demotes a player from server operator.");
static const String op_help(
	"Syntax: /op <player name>\n"
	"<player name> becomes a server operator."
);
static const String deop_help(
	"Syntax: /deop <player name>\n"
	"<player name> is no longer a server operator."
);


class OpCommand : public Command {


	private:
	
	
		bool op;


	public:
	
	
		OpCommand () = delete;
		OpCommand (bool op) noexcept : op(op) {	}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return op ? op_identifier : deop_identifier;
		
		}
		
		
		virtual bool Check (SmartPointer<Client> client) const override {
		
			//	Only ops can op/deop
			return Ops::Get().IsOp(client->GetUsername());
		
		}
		
		
		virtual const String & Summary () const noexcept override {
		
			return op ? op_summary : deop_summary;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return op ? op_help : deop_help;
		
		}
		
		
		virtual Vector<String> AutoComplete (const String & args) const override {
		
			Vector<String> retr;
			
			//	Create a regex to filter users
			//	so we only select users that have
			//	a username which begins with the
			//	passed string
			Regex regex(
				String::Format(
					"^{0}",
					Regex::Escape(
						args
					)
				),
				//	Usernames are not case sensitive
				RegexOptions().SetIgnoreCase()
			);
			
			for (auto & client : Server::Get().Clients) {
			
				//	Only authenticated users are
				//	valid chat targets
				if (client->GetState()==ClientState::Authenticated) {
				
					//	Get client's username
					String username(client->GetUsername());
					
					//	Only autocomplete if this user
					//	matches the regex and is a 
					//	valid target (i.e. is not already
					//	an op when the user is trying to
					//	autocomplete /op).
					if (
						regex.IsMatch(username) &&
						(op!=Ops::Get().IsOp(username))
					) {
					
						//	Normalize to lower case
						username.ToLower();
						
						//	Add
						retr.Add(std::move(username));
					
					}
				
				}
			
			}
			
			return retr;
		
		}
		
		
		virtual bool Execute (SmartPointer<Client> client, const String & args, ChatMessage & message) override {
		
			//	No player name, syntax error
			if (args.Size()==0) return false;
			
			//	Is the target player already
			//	opped/deopped?
			if (Ops::Get().IsOp(args)==op) {
			
				//	YES
				
				//	Return error message
				message	<< ChatStyle::Red
						<< ChatStyle::Bold
						<< "Player \""
						<< args
						<< "\" is "
						<< (op ? "already a server operator" : "not a server operator");
			
			} else {
			
				//	NO
				
				ChatMessage message;
				message << ChatStyle::Bold;
				
				if (op) {
				
					Ops::Get().Op(args);
					
					message << ChatStyle::BrightGreen << "Opping " << args;
				
				} else {
				
					Ops::Get().DeOp(args);
					
					message << ChatStyle::Red << "Deopping " << args;
				
				}
				
				Chat::Get().Send(message);
			
			}
			
			return true;
		
		}


};


static const Word priority=1;
static const String name("Op/Deop Commands");


class OpCommandModule : public Module {


	private:
	
	
		OpCommand op;
		OpCommand deop;


	public:
	
	
		OpCommandModule () noexcept : op(true), deop(false) {	}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			Commands::Get().Add(&op);
			Commands::Get().Add(&deop);
		
		}


};


static Nullable<OpCommandModule> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

#include <rleahylib/rleahylib.hpp>
#include <rleahylib/main.hpp>
#include <cli/cli.hpp>
#include <server.hpp>
#include <atomic>
#include <cstdlib>
#include <exception>
#include <utility>


using namespace MCPP;


class ServerWrapper : public CLIProvider {


	private:
	
	
		virtual void operator () (String in) override {
		
			
		
		}
	
	
	public:
	
	
		ServerWrapper () {
	
			auto & server=Server::Get();
			
			//	Hook into the server
			server.OnLog.Add([this] (const String & str, Service::LogType type) mutable {	WriteLog(str,type);	});
			server.OnChatLog.Add([this] (
				const String & from,
				const Vector<String> & to,
				const String & message,
				const Nullable<String> & notes
			) mutable {	WriteChatLog(from,to,message,notes);	});
	
		}


};


typedef Tuple<UInt32,ProtocolState,ProtocolDirection> PacketID;


class CLIApp {


	private:
	
	
		CLI cli;
		Nullable<ServerWrapper> wrapper;
		std::atomic<bool> do_restart;
		//	Packets to log
		Nullable<Vector<PacketID>> packets;
		//	Keys to log
		Nullable<Vector<String>> keys;
	
	
	public:
	
	
		CLIApp (
			Nullable<Vector<PacketID>> packets,
			Nullable<Vector<String>> keys
		) noexcept : packets(std::move(packets)), keys(std::move(keys)) {	}
		
		
		~CLIApp () noexcept {
		
			//	Kill the server
			Server::Get().Destroy();
		
			//	Make sure the CLI is not attached
			//	to the wrapper as we shut down
			cli.ClearProvider();
		
		}
		
		
		void operator () () {
		
			do {
			
				do_restart=false;
			
				//	Create wrapper
				wrapper.Construct();
				
				//	Install wrapper into CLI
				cli.SetProvider(&(*wrapper));
				
				auto & server=Server::Get();
				
				//	Install handlers
				server.OnShutdown.Add([this] () mutable {	cli.Shutdown();	});
				server.OnRestart=[this] () mutable {
				
					do_restart=true;
					cli.Shutdown();
				
				};
				
				//	Setup debugging if necessary
				if (!(
					packets.IsNull() &&
					keys.IsNull()
				)) {
				
					//	At least something is being
					//	debugged
				
					server.SetDebug(true);
					
					//	Packets
					if (!packets.IsNull()) {
					
						//	Empty list means debug everything
						if (packets->Count()==0) server.SetDebugAllPackets(true);
						else for (auto & id : *packets) server.SetDebugPacket(
							id.Item<0>(),
							id.Item<1>(),
							id.Item<2>()
						);
					
					}
					
					//	Keys
					if (!keys.IsNull()) {
					
						//	Empty list means debug everything
						if (keys->Count()==0) server.SetVerboseAll(true);
						else for (auto & key : *keys) server.SetVerbose(key);
					
					}
				
				}
				
				//	Start the server
				server.Start();
				
				//	Wait as the server runs
				if (cli.Wait()==CLI::ShutdownReason::Panic) std::abort();
				
				//	Shutdown server
				Server::Destroy();
				
			} while (do_restart);
		
		}


};


//	Detects the hex prefix and extracts
//	associated digits
static const Regex hex_regex("^\\s*0x(\\d+)\\s*$");
//	Parses protocol states
static const Tuple<Regex,ProtocolState> states []={
	{
		Regex(
			"^\\s*h(?:andshak(?:e|ing))?\\s*$",
			RegexOptions().SetIgnoreCase()
		),
		ProtocolState::Handshaking
	},
	{
		Regex(
			"^\\sp(?:lay)?\\s*$",
			RegexOptions().SetIgnoreCase()
		),
		ProtocolState::Play
	},
	{
		Regex(
			"^\\s*s(?:tatus)?\\s*$",
			RegexOptions().SetIgnoreCase()
		),
		ProtocolState::Status
	},
	{
		Regex(
			"^\\s*l(?:ogin)?\\s*$",
			RegexOptions().SetIgnoreCase()
		),
		ProtocolState::Login
	}
};
//	Parses protocol directions
static const Tuple<Regex,ProtocolDirection> directions []={
	{
		Regex(
			"^\\s*c(?:lient(?:bound)?)?\\s*$",
			RegexOptions().SetIgnoreCase()
		),
		ProtocolDirection::Clientbound
	},
	{
		Regex(
			"^\\s*s(?:erver(?:bound)?)?\\s*$",
			RegexOptions().SetIgnoreCase()
		),
		ProtocolDirection::Serverbound
	},
	{
		Regex(
			"^\\s*b(?:oth)?\\s*$",
			RegexOptions().SetIgnoreCase()
		),
		ProtocolDirection::Both
	}
};


class CLIAppOptions {


	public:
	
	
		//	Packets that will be debugged
		Nullable<Vector<PacketID>> Packets;
		//	Keys that will be debugged
		Nullable<Vector<String>> Keys;
		
		
	private:
	
	
		bool all (const Vector<String> & args) noexcept {
		
			if (!(
				(args.Count()==0) &&
				Packets.IsNull() &&
				Keys.IsNull()
			)) return false;
			
			Packets.Construct();
			Keys.Construct();
			
			return true;
		
		}
		
		
		bool all_packets (const Vector<String> & args) noexcept {
		
			if (!(
				(args.Count()==0) &&
				Packets.IsNull()
			)) return false;
			
			Packets.Construct();
			
			return true;
		
		}
		
		
		bool all_keys (const Vector<String> & args) noexcept {
		
			if (!(
				(args.Count()==0) &&
				Keys.IsNull()
			)) return false;
			
			Keys.Construct();
			
			return true;
		
		}
		
		
		Nullable<UInt32> get_packet_id (const String & str) {
		
			Nullable<UInt32> retr;
			
			auto match=hex_regex.Match(str);
			
			UInt32 from_str;
			if (
				match.Success()
					?	match[1].Value().ToInteger(&from_str,16)
					:	str.ToInteger(&from_str)
			) retr.Construct(from_str);
			
			return retr;
		
		}
		
		
		Nullable<ProtocolState> get_protocol_state (const String & str) {
		
			Nullable<ProtocolState> retr;
			
			for (auto & t : states) if (t.Item<0>().IsMatch(str)) {
			
				retr.Construct(t.Item<1>());
				
				break;
			
			}
			
			return retr;
		
		}
		
		
		Nullable<ProtocolDirection> get_protocol_direction (const String & str) {
		
			Nullable<ProtocolDirection> retr;
			
			for (auto & t : directions) if (t.Item<0>().IsMatch(str)) {
			
				retr.Construct(t.Item<1>());
				
				break;
			
			}
			
			return retr;
		
		}
		
		
		bool packets (const Vector<String> & args) {
		
			if (
				//	There must be arguments -- the
				//	packets that shall be logged
				(args.Count()==0) ||
				//	Packets can't already have been
				//	initialized by attempting to log
				//	all packets
				(
					!Packets.IsNull() &&
					(Packets->Count()==0)
				) ||
				//	Each packet is described by
				//	a 3-tuple of ID, state, and
				//	direction, therefore the number
				//	of arguments must be divisible
				//	by three
				((args.Count()%3)!=0)
			) return false;
			
			if (Packets.IsNull()) Packets.Construct();
			
			for (Word i=0;i<args.Count();) {
			
				//	Get the packet ID
				auto id=get_packet_id(args[i++]);
				if (id.IsNull()) return false;
				
				//	Get the protocol state
				auto state=get_protocol_state(args[i++]);
				if (state.IsNull()) return false;
				
				//	Get the protocol direction
				auto dir=get_protocol_direction(args[i++]);
				if (dir.IsNull()) return false;
				
				//	Add
				Packets->EmplaceBack(*id,*state,*dir);
			
			}
			
			return true;
		
		}
		
		
		bool keys (const Vector<String> & args) {
		
			if (
				(args.Count()==0) ||
				(
					!Keys.IsNull() &&
					(Keys->Count()==0)
				)
			) return false;
			
			if (Keys.IsNull()) Keys.Construct();
			
			for (const auto & s : args) Keys->Add(s);
		
			return true;
		
		}
	
	
	public:
	
	
		bool Add (CommandLineArgument arg) {
		
			//	If there's no proper flag, fail
			if (arg.Flag.IsNull()) return false;
			
			auto & flag=*arg.Flag;
			auto & args=arg.Arguments;
			
			return (
				((flag=="v") || (flag=="V"))
					?	all(args)
					:	(
							(flag=="Vp")
								?	all_packets(args)
								:	(
										(flag=="Vk")
											?	all_keys(args)
											:	(
													(flag=="vp")
														?	packets(args)
														:	(
																(flag=="vk")
																	?	keys(args)
																	:	false
															)
												)
									)
						)
			);
		
		}


};


//	Help message
static const String help_string(
	"MCPP Command-Line Front-End\n"
	"\n"
	"Help:\n"
	"\n"
	"/?, -?\n"
	"\tDisplays this help\n"
	"/V, -V, /v, -v\n"
	"\tRuns the server in verbose mode\n"
	"/Vp, -Vp\n"
	"\tRuns the server and outputs debug information for all packets\n"
	"/Vk, -Vk\n"
	"\tRuns the server with all module keys set to verbose\n"
	"/vp, -vp <packet ID> <state> <direction> [...]\n"
	"\tRuns the server outputting debug information for certain packets\n"
	"/vk, -vk <key> [...]\n"
	"\tRuns the server with certain module keys set to verbose\n"
	"\n"
	"Use CTRL+C at anytime to shut the server down\n"
	"\n"
	"Enter \"chat\" to see the chat log, \"log\" to see the server log"
);
//	Arguments which will cause help to be
//	displayed
static const String help_args[]={
	"help",
	"?"
};
//	Message displayed when there's a problem
//	parsing command line arguments
static const String error_parsing(
	"Error parsing command line arguments\n"
	"Try /?"
);


//	Bootstraps the application
int Main (const Vector<const String> & args) {

	try {
	
		CLIAppOptions options;
		
		//	Flags used to maintain state
		//	during command line argument
		//	parsing
		bool error=false;
		bool help=false;
		
		//	Parse command line arguments
		ParseCommandLineArguments(args,[&] (CommandLineArgument arg) {
		
			//	Abort if help or error
			if (help || error) return;
			
			//	Check for the help flag
			auto trimmed_raw=arg.RawFlag;
			trimmed_raw.Trim().ToLower();
			auto trimmed=arg.Flag;
			if (!trimmed.IsNull()) trimmed->Trim().ToLower();
			for (auto & s : help_args) if (
				(trimmed_raw==s) ||
				(
					!trimmed.IsNull() &&
					(*trimmed==s)
				)
			) {
			
				help=true;
				
				StdOut << help_string << Newline;
				
				return;
			
			}
			
			//	Wasn't help, try and parse for
			//	real
			if (!options.Add(std::move(arg))) {
			
				//	Error parsing arguments
			
				StdOut << error_parsing << Newline;
				
				error=true;
			
			}
		
		});
		
		//	Abort if help or error
		if (help) return EXIT_SUCCESS;
		if (error) return EXIT_FAILURE;
	
		//	Configure application
		CLIApp app(
			std::move(options.Packets),
			std::move(options.Keys)
		);
		
		//	Run application
		app();
	
	} catch (const std::exception & e) {
	
		try {
		
			StdOut << "ERROR: " << e.what() << Newline;
		
		} catch (...) {	}
		
		return EXIT_FAILURE;
	
	} catch (...) {
	
		try {
	
			StdOut << "ERROR" << Newline;
			
		} catch (...) {	}
	
		return EXIT_FAILURE;
	
	}
	
	return EXIT_SUCCESS;

}

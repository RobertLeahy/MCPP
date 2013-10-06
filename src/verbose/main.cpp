#include <rleahylib/rleahylib.hpp>
#include <command/command.hpp>
#include <op/op.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <utility>


class ParseResult {


	public:
	
	
		ParseResult () noexcept : AllPackets(false), AllKeys(false) {	}
	
	
		Vector<
			Tuple<
				Byte,
				ProtocolDirection
			>
		> Packets;
		Vector<String> Keys;
		bool AllPackets;
		bool AllKeys;


};


static const Regex parse_regex("[^\\s]+");
static const Regex hex_regex("0x([\\da-fA-F]+)");
static const String all_packets("all-packets");
static const String all_keys("all-keys");


static ParseResult parse (const String & str) {

	ParseResult retr;

	//	Loop once for each word
	for (auto & match : parse_regex.Matches(str)) {
	
		//	All packets?
		if (match.Value()==all_packets) {
		
			//	YES
			
			retr.AllPackets=true;
			
			continue;
		
		}
		
		//	All keys?
		if (match.Value()==all_keys) {
		
			//	YES
			
			retr.AllKeys=true;
			
			continue;
		
		}
	
		Byte id;
		ProtocolDirection direction=ProtocolDirection::Both;
		
		//	If this match a valid
		//	packet?
		if (match.Value().ToInteger(&id)) {
		
			//	YES
			
			retr.Packets.EmplaceBack(id,direction);
			
			continue;
		
		}
		
		//	Is this match a valid
		//	packet in hexadecimal format?
		auto hex=hex_regex.Match(match.Value());
		if (
			hex.Success() &&
			hex[1].Value().ToInteger(&id,16)
		) {
		
			//	YES
			
			retr.Packets.EmplaceBack(id,direction);
			
			continue;
		
		}
		
		//	We're forced to assume that it's a
		//	key
		retr.Keys.Add(match.Value());
	
	}
	
	return retr;

}


static const String verbose_identifier("verbose");
static const String verbose_summary("Enables debug logging.");
static const String verbose_help(
	"Syntax: /verbose [all-packets] [all-keys] [<key>...] [<packet>...]\n"
	"Enables debug logging.\n"
	"If no arguments are provided, toggles debug logging on.  Whether anything will be debug logged is indeterminate.\n"
	"If \"all-packets\" is specified, sets all packets to be logged.\n"
	"If \"all-keys\" is specified, all keys will be logged.\n"
	"All keys specified will be logged.\n"
	"All packets specified will be logged.\n"
	"Note that if all packets or keys, a certain packet, or a certain key is specified, but debug logging has not been enabled, nothing will be logged."
);


static inline bool check_impl (const SmartPointer<Client> & client) {

	return Ops::Get().IsOp(client->GetUsername());

}


static inline void execute_impl (const String & args, bool verbose) {

	String trimmed=args.Trim();
	
	auto & server=Server::Get();
	
	//	If there were no arguments,
	//	simply toggle debug logging
	if (trimmed.Size()==0) {
	
		server.SetDebug(verbose);
		
		return;
	
	}
	
	//	There are arguments -- parse
	auto parsed=parse(trimmed);
	
	//	Toggle debugging on all packets
	//	if appropriate
	if (parsed.AllPackets) server.SetDebugAllPackets(verbose);
	
	//	Toggle debugging on all keys
	//	if appropriate
	if (parsed.AllKeys) server.SetVerboseAll(verbose);
	
	//	Toggle debugging for all
	//	appropriate packets
	for (auto & t : parsed.Packets) if (verbose) server.SetDebugPacket(
		t.Item<0>(),
		t.Item<1>()
	);
	else server.UnsetDebugPacket(t.Item<0>());
	
	//	Toggle debugging for all
	//	appropriate keys
	for (auto & key : parsed.Keys) if (verbose) server.SetVerbose(std::move(key));
	else server.UnsetVerbose(key);

}


class Verbose : public Command {


	public:
	
	
		virtual const String & Identifier () const noexcept override {
		
			return verbose_identifier;
		
		}
		
		
		virtual const String & Summary () const noexcept override {
		
			return verbose_summary;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return verbose_help;
		
		}
		
		
		virtual bool Check (SmartPointer<Client> client) const override {
		
			return check_impl(client);
		
		}
		
		
		virtual bool Execute (SmartPointer<Client>, const String & args, ChatMessage &) override {
		
			execute_impl(args,true);
		
			return true;
		
		}


};


static const String quiet_identifier("quiet");
static const String quiet_summary("Disables debug logging.");
static const String quiet_help(
	"Syntax: /quiet [all-packets] [all-keys] [<key>...] [<packet>...]\n"
	"Disables debug logging.\n"
	"If no arguments are provided, toggles debug logging off.\n"
	"If \"all-packets\" is specified, disables logging of all packets.  Packets individually specified to be logged will still be logged, however.\n"
	"If \"all-keys\" is specified, disables logging of all keys.  Keys individually specified to be logged will still be logged, however.\n"
	"All keys specified will no longer be logged.\n"
	"All packets specified will no longer be logged."
);


class Quiet : public Command {


	public:
	
	
		virtual const String & Identifier () const noexcept override {
		
			return quiet_identifier;
		
		}
		
		
		virtual const String & Summary () const noexcept override {
		
			return quiet_summary;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return quiet_help;
		
		}
		
		
		virtual bool Check (SmartPointer<Client> client) const override {
		
			return check_impl(client);
		
		}
		
		
		virtual bool Execute (SmartPointer<Client>, const String & args, ChatMessage &) override {
		
			execute_impl(args,false);
			
			return true;
		
		}


};


static const String name("Verbose Command");
static const Word priority=1;


class VerboseInstaller : public Module {


	private:
	
	
		Verbose verbose;
		Quiet quiet;


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			auto & commands=Commands::Get();
			
			commands.Add(&verbose);
			commands.Add(&quiet);
		
		}


};


INSTALL_MODULE(VerboseInstaller)

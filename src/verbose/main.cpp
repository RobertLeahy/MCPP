#include <rleahylib/rleahylib.hpp>
#include <command/command.hpp>
#include <permissions/permissions.hpp>
#include <mod.hpp>
#include <packet.hpp>
#include <server.hpp>
#include <variant.hpp>
#include <utility>


using namespace MCPP;


static const String name("Verbose Command");
static const Word priority=1;
static const String verbose_identifier("verbose");
static const String quiet_identifier("quiet");
static const String all_packets("-all-packets");
static const String all_keys("-all-keys");
static const String verbose_summary("Enables various logging/debugging features.");
static const String quiet_summary("Disables various logging/debugging features.");
static const String enables("enables");
static const String disables("disables");
static const String help(
	"Syntax: /{0} [-all-packets] [-all-keys] [<key>|<id> <state> <direction>] ...\n"
	"If called with no arguments, {1} debug logging.\n"
	"If called with \"-all-packets\", {1} debug logging for every packet.\n"
	"If called with \"-all-keys\", {1} debug logging for every key.\n"
	"Also {1} debug logging for all specified individual keys and packets, where a packet is a 3-tuple consisting of an ID, protocol state, and protocol direction."
);
static const String autocomplete_regex("^{0}");


//	Parsing regexes
static const Regex hex("^0x(.+)$");
static const Regex handshake(
	"^h(?:andshak(?:e|ing))?$",
	RegexOptions().SetIgnoreCase()
);
static const Regex play(
	"^p(?:lay)?$",
	RegexOptions().SetIgnoreCase()
);
static const Regex status(
	"^s(?:tatus)?$",
	RegexOptions().SetIgnoreCase()
);
static const Regex login(
	"^l(?:ogin)?$",
	RegexOptions().SetIgnoreCase()
);
static const Regex clientbound(
	"^c(?:lient(?:bound)?)?$",
	RegexOptions().SetIgnoreCase()
);
static const Regex serverbound(
	"^s(?:erver(?:bound)?)?$",
	RegexOptions().SetIgnoreCase()
);
static const Regex both(
	"^b(?:oth)?$",
	RegexOptions().SetIgnoreCase()
);


class Verbose : public Module, public Command {


	private:
	
	
		typedef Tuple<UInt32,ProtocolState,ProtocolDirection> packet_type;
	
	
		class Toggle {
		
		
			public:
			
			
				bool AllKeys;
				bool AllPackets;
				Vector<
					Variant<
						packet_type,
						String
					>
				> Items;
		
		
		};
	
	
		class ParseResult {
		
			
			public:
			
			
				bool Enable;
				Nullable<Toggle> Info;
			
		
		};


		static bool is_verbose (const String & identifier) {
		
			return identifier==verbose_identifier;
		
		}
		
		
		static Nullable<ProtocolDirection> get_direction (const String & str) {
		
			if (clientbound.IsMatch(str)) return ProtocolDirection::Clientbound;
			if (serverbound.IsMatch(str)) return ProtocolDirection::Serverbound;
			if (both.IsMatch(str)) return ProtocolDirection::Both;
			
			return Nullable<ProtocolDirection>();
		
		}
		
		
		static Nullable<ProtocolState> get_state (const String & str) {
		
			if (handshake.IsMatch(str)) return ProtocolState::Handshaking;
			if (play.IsMatch(str)) return ProtocolState::Play;
			if (status.IsMatch(str)) return ProtocolState::Status;
			if (login.IsMatch(str)) return ProtocolState::Login;
			
			return Nullable<ProtocolState>();
		
		}
		
		
		static Nullable<packet_type> get_packet (String * & begin, const String * end) {
		
			//	Attempt to get a packet ID
			UInt32 id;
			if (begin->ToInteger(&id)) {
			
				++begin;
			
			} else {
			
				auto match=hex.Match(*begin);
				
				if (
					match.Success() &&
					match[1].Value().ToInteger(&id,16)
				) ++begin;
				else return Nullable<packet_type>();
			
			}
			
			if (begin==end) return Nullable<packet_type>();
			
			//	Get protocol state
			auto state=get_state(*(begin++));
			if (
				state.IsNull() ||
				(begin==end)
			) return Nullable<packet_type>();
			
			//	Get protocol direction
			auto direction=get_direction(*(begin++));
			if (direction.IsNull()) return Nullable<packet_type>();
			
			return packet_type(
				id,
				*state,
				*direction
			);
		
		}
		
		
		static Variant<packet_type,String> get (String * & begin, const String * end) {
		
			//	Attempt to parse a packet
			String * before=begin;
			auto packet=get_packet(begin,end);
			if (packet.IsNull()) {
			
				//	If the iterator was advanced,
				//	that means there was an unsuccessful
				//	parse attempt, which means a syntax
				//	error
				if (before!=begin) return Variant<packet_type,String>();
				
				//	Otherwise we just continue
			
			} else {
			
				//	Packet was parsed, return it
				return std::move(*packet);
			
			}
			
			//	Take a key
			return std::move(*(begin++));
		
		}
		
		
		static Nullable<ParseResult> parse (CommandEvent event) {
		
			ParseResult result;
			result.Enable=is_verbose(event.Identifier);
			
			//	If there are no arguments, return at once
			if (event.Arguments.Count()==0) return result;
			
			Toggle toggle;
			
			auto begin=event.Arguments.begin();
			auto end=event.Arguments.end();
			
			//	Parse leading flags if present
			for (;begin!=end;++begin) {
			
				if (
					!toggle.AllKeys &&
					(*begin==all_keys)
				) {
				
					toggle.AllKeys=true;
					
					continue;
				
				}
				
				if (
					!toggle.AllPackets &&
					(*begin==all_packets)
				) {
				
					toggle.AllPackets=true;
					
					continue;
				
				}
				
				break;
				
			}
			
			//	Get keys/packets
			while (begin!=end) {
			
				auto item=get(begin,end);
				
				if (item.IsNull()) return Nullable<ParseResult>();
				
				toggle.Items.Add(std::move(item));
			
			}
			
			result.Info.Construct(std::move(toggle));
			
			return result;
		
		}
		
		
		static void debug_toggle (bool enable, ChatMessage &) {
		
			Server::Get().SetDebug(enable);
		
		}
		
		
		static void specific_toggle (ParseResult result, ChatMessage &) {
		
			auto & info=*result.Info;
			auto & server=Server::Get();
			
			//	Set all keys as appropriate
			if (info.AllKeys) server.SetVerboseAll(result.Enable);
			//	Set all packets as appropriate
			if (info.AllPackets) server.SetDebugAllPackets(result.Enable);
			
			//	Loop for specific keys/packets
			for (auto & item : info.Items) {
			
				if (item.Is<packet_type>()) {
				
					//	Enable or disable a specific
					//	packet type
				
					auto & packet=item.Get<packet_type>();
					auto & id=packet.Item<0>();
					auto & state=packet.Item<1>();
					auto & direction=packet.Item<2>();
					
					if (result.Enable) server.SetDebugPacket(id,state,direction);
					else server.UnsetDebugPacket(id,state,direction);
					
				} else {
				
					//	Enable a disable a specific
					//	key
				
					auto & key=item.Get<String>();
					
					if (result.Enable) server.SetVerbose(std::move(key));
					else server.UnsetVerbose(std::move(key));
				
				}
			
			}
		
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
				verbose_identifier,
				this
			);
			commands.Add(
				quiet_identifier,
				this
			);
		
		}
		
		
		virtual void Summary (const String & identifier, ChatMessage & message) override {
		
			message << (is_verbose(identifier) ? verbose_summary : quiet_summary);
		
		}
		
		
		virtual void Help (const String & identifier, ChatMessage & message) override {
		
			message << String::Format(
				help,
				identifier,
				is_verbose(identifier) ? enables : disables
			);
		
		}
		
		
		virtual Vector<String> AutoComplete (const CommandEvent & event) override {
		
			Vector<String> retr;
			
			//	If there are more than two arguments,
			//	we can't make any suggestions
			if (event.Arguments.Count()>2) return retr;
			
			//	If there are no arguments we can
			//	suggest both arguments
			if (event.Arguments.Count()==0) {
			
				retr.Add(all_packets);
				retr.Add(all_keys);
				
				return retr;
			
			}
			
			//	Make a regex
			Regex regex(
				String::Format(
					autocomplete_regex,
					Regex::Escape(
						event.Arguments[event.Arguments.Count()-1]
					)
				)
			);
			
			//	Attempt to make suggestions
			if (
				regex.IsMatch(all_keys) &&
				!(
					(event.Arguments.Count()==2) &&
					(event.Arguments[0]==all_keys)
				)
			) retr.Add(all_keys);
			if (
				regex.IsMatch(all_packets) &&
				!(
					(event.Arguments.Count()==2) &&
					(event.Arguments[0]==all_packets)
				)
			) retr.Add(all_packets);
			
			return retr;
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			return event.Issuer.IsNull() ? true : Permissions::Get().GetUser(event.Issuer).Check(verbose_identifier);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
			
			//	Parse arguments and fail on bad
			//	syntax
			auto result=parse(std::move(event));
			if (result.IsNull()) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			retr.Status=CommandStatus::Success;
			
			//	Branch depending on whether debug
			//	mode is being toggled, or if only
			//	certain keys/packets are being
			//	toggled
			if (result->Info.IsNull()) debug_toggle(
				result->Enable,
				retr.Message
			);
			else specific_toggle(
				std::move(*result),
				retr.Message
			);
			
			return retr;
		
		}


};


INSTALL_MODULE(Verbose)

#include <info/info.hpp>
#include <format.hpp>
#include <packet.hpp>
#include <server.hpp>


using namespace MCPP;


static const String name("MCPP Information");
static const Word priority=1;
static const String identifier("mcpp");
static const String help("Display information about mcpp.dll.");
static const String mcpp_banner("MINECRAFT++:");
static const String compiled_by_template("Compiled by {0} on {1}");
static const String minecraft_compat("Compatible with Minecraft {0} (protocol version {1})");


class MCPPInfo : public Module, public InformationProvider {


	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			Information::Get().Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			message	<<	ChatStyle::Bold
					<<	mcpp_banner
					<<	ChatFormat::Pop
					<<	Newline
					<<	String::Format(
							compiled_by_template,
							Server::CompiledWith,
							Server::BuildDate
						)
					<<	Newline
					<<	String::Format(
							minecraft_compat,
							FormatVersion(
								MinecraftMajorVersion,
								MinecraftMinorVersion,
								MinecraftPatch
							),
							ProtocolVersion
						);
		
		}


};


INSTALL_MODULE(MCPPInfo)

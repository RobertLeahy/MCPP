#include <info/info.hpp>
#include <op/op.hpp>
#include <singleton.hpp>
#include <utility>
#include <algorithm>


using namespace MCPP;


namespace MCPP {


	static const String name("Information Provider");
	static const String identifier("info");
	static const String summary("Provides information about various server internals.");
	static const Word priority=1;
	static const String info_banner="====INFORMATION====";
	static const String log_prepend("Information Provider: ");
	static const String mods_dir("info_mods");
	
	
	static String help;
	
	
	static const Regex split("\\s+");
	
	
	Word Information::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & Information::Name () const noexcept {
	
		return name;
	
	}
	
	
	const String & Information::Identifier () const noexcept {
	
		return identifier;
	
	}
	
	
	const String & Information::Help () const noexcept {
	
		return help;
	
	}
	
	
	const String & Information::Summary () const noexcept {
	
		return summary;
	
	}
	
	
	bool Information::Check (SmartPointer<Client> client) const {
	
		return Ops::Get().IsOp(client->GetUsername());
	
	}
	
	
	Vector<InformationProvider *> Information::retrieve (const Regex & regex) {
	
		Vector<InformationProvider *> retr;
		
		for (auto * ip : providers)
		if (regex.IsMatch(ip->Identifier()))
		retr.Add(ip);
		
		return retr;
	
	}
	
	
	InformationProvider * Information::retrieve (const String & name) {
	
		for (auto * ip : providers)
		if (name==ip->Identifier())
		return ip;
		
		return nullptr;
	
	}
	
	
	Vector<String> Information::AutoComplete (const String & args_str) const {
	
		Vector<String> retr;
		
		auto args=split.Split(args_str);
		
		if (args.Count()<=1)
		for (auto * ip : (
			(args.Count()==0)
				?	providers
				:	const_cast<Information *>(this)->retrieve(
						Regex(
							String::Format(
								"^{0}",
								Regex::Escape(
									args[0]
								)
							)
						)
					)
		)) retr.Add(ip->Identifier());
		
		return retr;
	
	}
	
	
	bool Information::Execute (SmartPointer<Client> client, const String & args_str, ChatMessage & message) {
	
		//	If there's no client, do nothing
		if (client.IsNull()) return true;
		
		auto args=split.Split(args_str);
		
		//	If there's no argument, or more
		//	than one, that's a syntax error
		if (args.Count()!=1) return false;
		
		//	Attempt to retrieve the appropriate
		//	information provider.  If there
		//	isn't one, that's a syntax error
		auto * ip=retrieve(args[0]);
		if (ip==nullptr) return false;
		
		//	Prepare a chat message
		message	<< ChatStyle::Bold
				<< ChatStyle::Yellow
				<< info_banner
				<< ChatFormat::Pop
				<< ChatFormat::Pop
				<< Newline;
				
		//	Get rest of chat message from
		//	provider
		ip->Execute(message);
		
		return true;
	
	}
	
	
	void Information::Install () {
		
		//	Install ourselves into the command
		//	module
		Commands::Get().Add(this);
	
	}
	
	
	void Information::Add (InformationProvider * provider) {
	
		if (provider!=nullptr) {
		
			//	Insert sorted
		
			//	Find insertion point
			Word i=0;
			for (
				;
				(i<providers.Count()) &&
				(providers[i]->Identifier()<provider->Identifier());
				++i
			);
			
			//	Insert
			providers.Insert(
				provider,
				i
			);
			
			//	Rebuild help
			//
			//	CONSIDER OPTIMIZING
			help=String();
			for (const auto * ip : providers) {
			
				if (help.Size()!=0) help << Newline;
				
				help << "/info " << ip->Identifier() << " - " << ip->Help();
			
			}
			
		}
	
	}
	
	
	static Singleton<Information> singleton;
	
	
	Information & Information::Get () noexcept {
	
		return singleton.Get();
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(singleton.Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

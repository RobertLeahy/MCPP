#include <info/info.hpp>
#include <permissions/permissions.hpp>
#include <singleton.hpp>


using namespace MCPP;


namespace MCPP {


	static const String name("Information Provider");
	static const String identifier("info");
	//	The permission which, when granted, allows
	//	users to invoke /info
	static const String permission("info");
	static const String summary("Provides information about various server internals.");
	static const Word priority=1;
	static const String info_banner("====INFORMATION====");
	static const String help("Syntax: ");
	static const String syntax("<provider>");
	static const String providers_banner("Providers:");
	static const String separator(" - ");
	static const String arg_separator(" ");
	
	
	InformationProvider * Information::get (const String & identifier) {
	
		auto iter=map.find(identifier);
		
		return (iter==map.end()) ? nullptr : iter->second;
	
	}
	
	
	static Singleton<Information> singleton;
	
	
	Information & Information::Get () noexcept {
	
		return singleton.Get();
	
	}
	
	
	Word Information::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void Information::Install () {
	
		Commands::Get().Add(identifier,this);
	
	}
	
	
	const String & Information::Name () const noexcept {
	
		return name;
	
	}
	
	
	void Information::Summary (const String &, ChatMessage & message) {
	
		message << summary;
	
	}
	
	
	void Information::Help (const String &, ChatMessage & message) {
	
		message	<<	help
				<<	ChatStyle::Bold
				<<	"/"
				<<	identifier
				<<	arg_separator
				<<	syntax
				<<	Newline
				<<	providers_banner
				<<	ChatFormat::Pop;
				
		//	Loop over providers and print name
		//	and summary for each
		for (auto ptr : list) message	<<	Newline
										<<	ChatStyle::Bold
										<<	ptr->Identifier()
										<<	ChatFormat::Pop
										<<	separator
										<<	ptr->Help();
	
	}
	
	
	Vector<String> Information::AutoComplete (const CommandEvent & event) {
	
		Vector<String> retr;
		
		//	How we proceed depends on the number of
		//	arguments given.
		//
		//	If zero arguments are given, our suggestion
		//	is any of the providers that we have installed
		//
		//	If one argument is given, our suggestion is any
		//	of the providers whose identifiers start with that
		//	string.
		//
		//	If more than one argument is given, we can't
		//	suggest anything, since this command takes only
		//	one argument
		switch (event.Arguments.Count()) {
		
			case 0:
				for (auto ptr : list) retr.Add(ptr->Identifier());
				break;
			case 1:{
			
				//	Create an appropriate regular
				//	expression
				Regex regex(
					String::Format(
						"^{0}",
						Regex::Escape(
							event.Arguments[0]
						)
					)
				);
				
				//	Loop and find matches
				for (auto ptr : list) if (regex.IsMatch(ptr->Identifier())) retr.Add(ptr->Identifier());
			
			}break;
			default:break;
		
		}
		
		return retr;
	
	}
	
	
	bool Information::Check (const CommandEvent & event) {
	
		//	Null client pointer means that this
		//	is issued by the console, which has
		//	all permissions
		if (event.Issuer.IsNull()) return true;
		
		//	Check to see if this user is granted
		//	the appropriate permission
		return Permissions::Get().GetUser(event.Issuer).Check(permission);
	
	}
	
	
	CommandResult Information::Execute (CommandEvent event) {
	
		CommandResult retr;
		
		//	Exactly one argument must be provided
		if (event.Arguments.Count()!=1) {
		
			retr.Status=CommandStatus::SyntaxError;
			
			return retr;
		
		}
		
		//	Attempt to get the associated information provider
		auto ptr=get(event.Arguments[0]);
		
		if (ptr==nullptr) {
		
			retr.Status=CommandStatus::DoesNotExist;
			
			return retr;
		
		}
		
		//	Execute
		
		retr.Message	<<	ChatStyle::Yellow
						<<	ChatStyle::Bold
						<<	info_banner
						<<	ChatFormat::Pop
						<<	ChatFormat::Pop
						<<	Newline;
						
		ptr->Execute(retr.Message);
		
		retr.Status=CommandStatus::Success;
		
		return retr;
	
	}
	
	
	void Information::Add (InformationProvider * provider) {
	
		//	If a null pointer was passed in,
		//	do nothing
		if (provider==nullptr) return;
		
		//	Attempt to insert into map
		
		auto iter=map.find(provider->Identifier());
		
		//	If a provider by this name exists
		//	in the map, replace it, otherwise
		//	create a new entry in the map
		if (iter==map.end()) {
		
			auto pair=map.emplace(
				provider->Identifier(),
				provider
			);
			
			iter=pair.first;
		
		} else {
		
			iter->second=provider;
		
		}
		
		//	Attempt to insert into list in sorted
		//	order.  If that fails, roll back and
		//	remove from map
		try {
		
			Word loc=0;
			for (
				;
				(loc<list.Count()) &&
				(list[loc]->Identifier()<provider->Identifier());
				++loc
			);
			
			list.Insert(provider,loc);
		
		} catch (...) {
		
			map.erase(iter);
		
			throw;
		
		}
	
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

#include <info/info.hpp>
#include <op/op.hpp>
#include <utility>
#include <algorithm>


namespace MCPP {


	static const String name("Information Provider");
	static const String identifier("info");
	static const String summary("Provides information about various server internals.");
	static const Word priority=1;
	static const String info_banner="====INFORMATION====";
	static const String log_prepend("Information Provider: ");
	static const String mods_dir("info_mods");
	
	
	static String help;
	static Nullable<ModuleLoader> mods;
	
	
	static const Regex split("\\s+");
	
	
	InformationModule::InformationModule () {
	
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
	
	
	InformationModule::~InformationModule () noexcept {
	
		mods->Unload();
	
	}
	
	
	Word InformationModule::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & InformationModule::Name () const noexcept {
	
		return name;
	
	}
	
	
	const String & InformationModule::Identifier () const noexcept {
	
		return identifier;
	
	}
	
	
	const String & InformationModule::Help () const noexcept {
	
		return help;
	
	}
	
	
	const String & InformationModule::Summary () const noexcept {
	
		return summary;
	
	}
	
	
	bool InformationModule::Check (SmartPointer<Client> client) const {
	
		return Ops->IsOp(client->GetUsername());
	
	}
	
	
	Vector<InformationProvider *> InformationModule::retrieve (const Regex & regex) {
	
		Vector<InformationProvider *> retr;
		
		for (auto * ip : providers)
		if (regex.IsMatch(ip->Identifier()))
		retr.Add(ip);
		
		return retr;
	
	}
	
	
	InformationProvider * InformationModule::retrieve (const String & name) {
	
		for (auto * ip : providers)
		if (name==ip->Identifier())
		return ip;
		
		return nullptr;
	
	}
	
	
	Vector<String> InformationModule::AutoComplete (const String & args_str) const {
	
		Vector<String> retr;
		
		auto args=split.Split(args_str);
		
		if (args.Count()<=1)
		for (auto * ip : (
			(args.Count()==0)
				?	providers
				:	const_cast<InformationModule *>(this)->retrieve(
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
	
	
	bool InformationModule::Execute (SmartPointer<Client> client, const String & args_str) {
	
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
		ChatMessage message;
		message.AddRecipients(std::move(client));
		message	<< ChatStyle::Bold
				<< ChatStyle::Yellow
				<< info_banner
				<< ChatFormat::Pop
				<< ChatFormat::Pop
				<< Newline;
				
		//	Get rest of chat message from
		//	provider
		ip->Execute(message);
		
		//	Send message
		Chat->Send(message);
		
		return true;
	
	}
	
	
	void InformationModule::Install () {
	
		//	Get mods
		mods->Load();
		
		//	Install ourselves into the command
		//	module
		Commands->Add(this);
		
		//	Install mods
		mods->Install();
		
		//	Sort commands so they appear
		//	in alphabetical order in
		//	calls to help
		std::sort(
			providers.begin(),
			providers.end(),
			[] (const InformationProvider * a, const InformationProvider * b) {
			
				return a->Identifier()<b->Identifier();
			
			}
		);
		
		//	Build help
		for (const auto * ip : providers) {
		
			if (help.Size()!=0) help << Newline;
			
			help << "/info " << ip->Identifier() << " - " << ip->Help();
		
		}
	
	}
	
	
	void InformationModule::Add (InformationProvider * provider) {
	
		if (provider!=nullptr) providers.Add(provider);
	
	}
	
	
	Nullable<InformationModule> Information;


}


extern "C" {


	Module * Load () {
	
		try {
		
			if (Information.IsNull()) {
			
				mods.Destroy();
				
				Information.Construct();
			
			}
			
			return &(*Information);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		Information.Destroy();
		
		if (!mods.IsNull()) mods->Unload();
	
	}
	
	
	void Cleanup () {
	
		mods.Destroy();
	
	}


}

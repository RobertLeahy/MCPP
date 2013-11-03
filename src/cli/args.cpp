#include <cli/cli.hpp>
#include <utility>


namespace MCPP {


	InvalidArg::InvalidArg (Word which) noexcept : which(which) {	}


	Word InvalidArg::Which () const noexcept {

		return which;

	}


	Args::Args (std::unordered_map<String,Vector<String>> args) noexcept : args(std::move(args)) {	}


	bool Args::IsSet (const String & arg) const {

		return args.count(arg)!=0;

	}


	const Vector<String> * Args::Get (const String & arg) const {

		auto iter=args.find(arg);
		
		if (iter==args.end()) return nullptr;
		
		return &(iter->second);

	}


	static const Regex arg_regex("^(?:\\-|\\/)(.+)$");


	Args Args::Parse (const Vector<const String> & args) {

		std::unordered_map<
			String,
			Vector<String>
		> retr;
		
		for (Word i=1;i<args.Count();++i) {
		
			auto match=arg_regex.Match(args[i]);
			
			//	If this isn't a properly-formatted
			//	argument, throw
			if (!match.Success()) throw InvalidArg(i);
			
			//	Extract all arguments attached to
			//	this argument
			Vector<String> assoc;
			while (
				((i+1)<args.Count()) &&
				!arg_regex.IsMatch(args[i+1])
			) assoc.Add(args[++i]);
			
			String arg(match[1].Value().ToLower());
			
			//	If this argument is already present,
			//	merge the attached arguments, otherwise
			//	insert
			auto iter=retr.find(arg);
			
			if (iter==retr.end()) retr.emplace(
				std::move(arg),
				std::move(assoc)
			);
			else for (auto & a : assoc) iter->second.Add(std::move(a));
		
		}
		
		return Args(std::move(retr));

	}
	
	
}

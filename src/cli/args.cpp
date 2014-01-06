#include <cli/cli.hpp>
#include <utility>


namespace MCPP {
	
	
	static const Regex regex("^\\s*(?:\\-|\\/)(.*)$");


	static Nullable<CommandLineArgument> get (const String * & begin, const String * end) {
	
		Nullable<CommandLineArgument> retr;
		
		//	If there's nothing left to parse,
		//	return nothing
		if (begin==end) return retr;
		
		retr.Construct();
		
		//	Record raw flag
		retr->RawFlag=*begin;
		
		//	Try and parse flag
		auto match=regex.Match(*begin);
		++begin;
		if (!match.Success()) return retr;
		
		//	Store parsed flag
		retr->Flag.Construct(match[1].Value());
		
		//	Get all arguments which follow
		for (
			;
			(begin!=end) &&
			!regex.IsMatch(*begin);
			++begin
		) retr->Arguments.Add(*begin);
		
		return retr;
	
	}


	void ParseCommandLineArguments (const Vector<const String> & args, std::function<void (CommandLineArgument)> callback) {
	
		//	If there are no arguments to
		//	parse, just return
		if (args.Count()<=1) return;
		
		Nullable<CommandLineArgument> arg;
		auto begin=args.begin()+1;
		while (!(arg=get(begin,args.end())).IsNull()) callback(std::move(*arg));
	
	}
	
	
}

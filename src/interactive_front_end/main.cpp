#include <common.hpp>
#include <rleahylib/main.hpp>


static void Help () {

	StdOut	<< Newline << "====MINECRAFT++ INTERACTIVE FRONT-END HELP====" << Newline << Newline
			<< "Modify server behaviour:" << Newline << Newline
			<< "-q, -quiet, -nolog, /q, /quiet, /nolog" << Newline
			<< "\tDoes not output the server log to the console" << Newline
			<< "-v, -verbose, /v, /verbose" << Newline
			<< "\tRuns the server in verbose mode" << Newline;

}


static const Regex help_regex(
	"^[\\/\\-](?:\\?|h(?:elp)?)$",
	RegexOptions().SetIgnoreCase()
);
static const Regex verbose_regex(
	"^[\\/\\-]v(?:erbose)?$",
	RegexOptions().SetIgnoreCase()
);
static const Regex nolog_regex(
	"^[\\/\\-](?:q(?:uiet)?|nolog)$",
	RegexOptions().SetIgnoreCase()
);
static const Regex newline_regex(
	"^",
	RegexOptions().SetMultiline()
);
static Mutex console_lock;


int Main (const Vector<const String> & args) {

	try {
	
		//	Check to see if the user requested
		//	help
		for (const String & s : args) {
		
			if (help_regex.IsMatch(s)) {
			
				Help();
				
				return EXIT_SUCCESS;
			
			}
		
		}
		
		//	Create the server
		RunningServer.Construct();
		
		try {
		
			bool log=true;
		
			//	Check command-line arguments
			for (const String & s : args) {
			
				if (verbose_regex.IsMatch(s)) RunningServer->ProtocolAnalysis=true;
				
				if (nolog_regex.IsMatch(s)) log=false;
			
			}
			
			//	Hook our logging into the server
			//	if applicable
			if (log) RunningServer->OnLog.Add([] (const String & log, Service::LogType type) {
			
				String replacement("[LOG]");
				
				//	We want to prepend a
				//	description of the log
				//	type, but not if it's
				//	Service::LogType::Information,
				//	since basically every log
				//	entry has that
				if (type!=Service::LogType::Information) {
				
					replacement << " [" << DataProvider::GetLogType(type).ToUpper() << "]: ";
				
				} else {
				
					replacement << ": ";
				
				}
				
				String output(newline_regex.Replace(
					log,
					RegexReplacement::Escape(replacement)
				));
			
				console_lock.Execute([&] () {	StdOut << output << Newline;	});
			
			});
			
			//	Start the server
			RunningServer->StartInteractive(args);
			
			StdIn.ReadLine();
		
		} catch (...) {
		
			RunningServer.Destroy();
			
			throw;
		
		}
		
		RunningServer.Destroy();
	
	} catch (...) {
	
		return EXIT_FAILURE;
	
	}
	
	return EXIT_SUCCESS;

}

#include <rleahylib/rleahylib.hpp>
#include <rleahylib/main.hpp>
#include <cli/cli.hpp>
#include <server.hpp>
#include <atomic>
#include <cstdlib>
#include <exception>


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


class CLIApp {


	private:
	
	
		CLI cli;
		Nullable<ServerWrapper> wrapper;
		std::atomic<bool> do_restart;
	
	
	public:
	
	
		CLIApp (const Vector<const String> & args) {
		
			//	TODO: Get configuration
		
		}
		
		
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
				
				//	Start the server
				server.Start();
				
				//	Wait as the server runs
				if (cli.Wait()==CLI::ShutdownReason::Panic) std::abort();
				
				//	Shutdown server
				Server::Destroy();
				
			} while (do_restart);
		
		}


};


//	Bootstraps the application
int Main (const Vector<const String> & args) {

	try {
	
		//	Configure application
		CLIApp app(args);
		
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

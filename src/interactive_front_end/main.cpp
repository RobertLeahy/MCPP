#include <rleahylib/rleahylib.hpp>
#include <rleahylib/main.hpp>
#include <data_provider.hpp>
#include <server.hpp>
#include <thread_pool.hpp>
#include <cstdlib>
#include <utility>
#include "args.hpp"
#include "date_time.hpp"
#include "console.hpp"


enum class Action {

	None,
	Restart,
	Stop

};


//	Constants

static const Word history_max_default=1024;
static const Vector<String> help_args={
	"?",
	"h",
	"help"
};
static const String help(
	"====MINECRAFT++ INTERACTIVE FRONT-END====\n"
	"\n"
	"Invokes the Minecraft++ server in an interactive console window.\n"
	"\n"
	"====HELP====\n"
	"\n"
	"-v, -verbose args...\n"
	"\tThe packets (for hexademical notation) or keys (for strings)\n"
	"\tgiven as args will emit verbose, debug output.\n"
	"\n"
	"-verbose-all\n"
	"\tAll keys are verbose.\n"
	"\n"
	"-verbose-all-packets\n"
	"\tAll packets are verbose.\n"
	"\n"
	"-n number\n"
	"\tThe number of log/chat log lines to keep as history.\n"
	"\n"
	"-c\n"
	"\tThe log display starts on the chat log."
);
static const String log_template("[{0}] [{1}]: {2}");
static const String msg_template("[{0}]: {1}");
static const String chat_to_template("[{0}] {1} => {2}: {3}");
static const String chat_global_template("[{0}] {1}: {3}");
static const String recipient_separator(", ");
static const String show_chat("/chat");
static const String show_log("/log");
static const String history_max_arg("n");
static const String chat_start_arg("c");
static const Regex is_command(
	"^\\/(.*)$",
	RegexOptions().SetSingleline()
);
static const String no_command_interpreter("Server does not provide a command interpreter");
static const String error_during_command("Error while executing command");
static const String server_not_running("Server is not running");
static const String no_chat_provider("Server does not provider a chat provider");


//	Line buffers

static Vector<String> log_history;
static Vector<String> chat_log_history;
static bool is_chat=false;
static Mutex history_lock;

//	Console I/O Handler

static Nullable<Console> console;
static Nullable<ThreadPool> pool;

//	Shutdown synchronization

static Action action=Action::None;
static Mutex lock;
static CondVar wait;

//	Maximum number of lines to keep
//	in history

static Word history_max;

//	Server running or stopped?

bool running=false;
Mutex running_lock;


static bool handle_interrupt (Console::InterruptType type) {

	if (type==Console::InterruptType::Break) return true;
	
	lock.Acquire();
	action=Action::Stop;
	wait.WakeAll();
	lock.Release();
	
	return true;

}


static void write_msg (const String & msg) {

	String str(String::Format(
		msg_template,
		GetDateTime(),
		msg
	));
	
	history_lock.Execute([&] () {
	
		if (chat_log_history.Count()==history_max) chat_log_history.Delete(0);
		chat_log_history.Add(str);
		if (log_history.Count()==history_max) log_history.Delete(0);
		log_history.Add(str);
		
		console->WriteLine(std::move(str));
	
	});

}


static void handle_input (String str) noexcept {

	try {
	
		if (str==show_log) {
		
			//	Show the log
			
			history_lock.Execute([&] () {
			
				if (is_chat) {
				
					console->Clear();
					
					console->WriteLines(log_history);
					
					is_chat=false;
				
				}
			
			});
		
		} else if (str==show_chat) {
		
			//	Show the chat log
			
			history_lock.Execute([&] () {
			
				if (!is_chat) {
				
					console->Clear();
					
					console->WriteLines(chat_log_history);
					
					is_chat=true;
				
				}
			
			});
		
		} else {
		
			running_lock.Execute([&] () mutable {
			
				//	Make sure server is running
				if (!running) {
				
					write_msg(server_not_running);
					
					return;
					
				};
		
				//	Is this a command?
				auto match=is_command.Match(str);
				if (match.Success()) {
				
					CommandInterpreter * interpreter;
					
					try {
					
						interpreter=&(Server::Get().GetCommandInterpreter());
						
					} catch (...) {
					
						write_msg(no_command_interpreter);
						
						return;
					
					}
					
					Nullable<String> msg;
					
					try {
					
						msg=(*interpreter)(match[1].Value());
					
					} catch (...) {
					
						write_msg(error_during_command);
						
						return;
					
					}
					
					if (!msg.IsNull()) write_msg(*msg);
				
				} else {
				
					//	This is a broadcast chat
					//	message
					
					ChatProvider * provider;
					
					try {
					
						provider=&(Server::Get().GetChatProvider());
					
					} catch (...) {
					
						write_msg(no_chat_provider);
						
						return;
					
					}
					
					provider->Send(str);
				
				}
				
			});
		
		}
	
	} catch (...) {
	
		std::abort();
	
	}

}


static void handle_log (const String & log, Service::LogType type) noexcept {

	try {

		String str(String::Format(
			log_template,
			GetDateTime(),
			DataProvider::GetLogType(type).ToUpper(),
			log
		));

		history_lock.Execute([&] () {
		
			if (log_history.Count()==history_max) log_history.Delete(0);
			log_history.Add(str);
			
			if (!is_chat) console->WriteLine(std::move(str));
		
		});
		
	} catch (...) {
	
		std::abort();
	
	}

}


static void handle_chat_log (const String & from, const Vector<String> & to, const String & message) noexcept {

	try {
	
		String to_str;
		for (const auto & s : to) {
		
			if (to.Size()==0) to_str << recipient_separator;
			
			to_str << s;
		
		}
	
		String str(String::Format(
			(to.Count()==0) ? chat_global_template : chat_to_template,
			GetDateTime(),
			from,
			to_str,
			message
		));
		
		history_lock.Execute([&] () {
		
			if (chat_log_history.Count()==history_max) chat_log_history.Delete(0);
			chat_log_history.Add(str);
			
			if (is_chat) console->WriteLine(std::move(str));
		
		});
	
	} catch (...) {
	
		std::abort();
	
	}

}


static void get_history_max (const Args & args) {

	auto * opts=args.Get(history_max_arg);
	
	//	Go through all arguments with
	//	the appropriate flag from last
	//	to first, and if any of them
	//	are a valid integer, choose it
	//	as the maximum history size
	if (opts!=nullptr)
	for (Word i=opts->Count();(i--)>0;)
	if ((*opts)[i].ToInteger(&history_max))
	return;
	
	//	No valid argument, go with
	//	the default
	
	history_max=history_max_default;

}


static void get_settings (const Args & args) {

	//	Maximum entries in log histories
	get_history_max(args);
	
	//	Start on chat log vs. regular
	//	log
	if (args.IsSet(chat_start_arg)) is_chat=true;

}


static void configure (const Server & server, const Args & args) {

	

}


int Main (const Vector<const String> & args) {

	//	Parse arguments
	Args options(Args::Parse(args));
	
	//	Set globals based on arguments
	get_settings(options);
	
	//	Create console I/O handler
	console.Construct(
		handle_input,
		history_max,
		[] (std::exception_ptr) {	std::abort();	}
	);
	
	//	Help?
	for (const auto & h : help_args) if (options.IsSet(h)) {
	
		console->WriteLine(help);
		
		console->Wait();
		
		return EXIT_SUCCESS;
	
	}
	
	//	Install interrupt handler
	Console::SetHandler(handle_interrupt);
	
	//	Create thread pool
	pool.Construct(
		1,
		std::abort
	);
	
	restart:
	
	//	Create server
	auto & server=Server::Get();
	
	//	Configure server
	configure(server,options);
	
	//	Install handlers
	server.OnLog.Add([] (const String & log, Service::LogType type) {	pool->Enqueue(handle_log,log,type);	});
	server.OnChatLog.Add([] (
		const String & from,
		const Vector<String> & to,
		const String & message,
		const Nullable<String> &
	) {	pool->Enqueue(handle_chat_log,from,to,message);	});
	server.OnShutdown.Add([] () {
	
		running_lock.Acquire();
		running=false;
		running_lock.Release();
	
		lock.Acquire();
		action=Action::Stop;
		wait.WakeAll();
		lock.Release();
	
	});
	server.OnRestart=[] () {
	
		lock.Acquire();
		action=Action::Restart;
		wait.WakeAll();
		lock.Release();
	
	};
	
	//	Start server
	running_lock.Execute([&] () {
	
		server.Start();
		running=true;
		
	});
	
	//	Wait for shutdown
	lock.Acquire();
	while (action==Action::None) wait.Sleep(lock);
	Action curr=action;
	lock.Release();
	
	running_lock.Acquire();
	running=false;
	running_lock.Release();
	
	//	Shutdown
	
	Server::Destroy();
	
	//	Reset action in case we
	//	restart
	lock.Acquire();
	action=Action::None;
	lock.Release();
	
	console->Wait();
	
	if (curr==Action::Restart) goto restart;
	
	console.Destroy();

	return EXIT_SUCCESS;

}

#include <cli/cli.hpp>
#include <data_provider.hpp>
#include <utility>


using namespace MCPP::CLIImpl;


namespace MCPP {


	static const Word output_max=256;


	CLI::CLI () : provider(nullptr), reason(ShutdownReason::None), chat_selected(false) {
	
		//	Attach CTRL+C handler
		Console::SetHandler([this] (Console::InterruptType type) mutable {
		
			switch (type) {
			
				//	On CTRL+C and stop, stop
				case Console::InterruptType::Interrupt:
				case Console::InterruptType::Stop:
					stop(ShutdownReason::Stop);
					break;
				//	Ignore all other interrupts
				default:
					break;
			
			}
			
			//	Unconditionally succeed
			return true;
		
		});
	
		//	Start up the command line interface
		console.Construct(
			[this] (String str) mutable {	input(std::move(str));	},
			output_max,
			[this] (std::exception_ptr ex) mutable {	panic(ex);	}
		);
	
	}
	
	
	CLI::~CLI () noexcept {
	
		//	Remove handler
		try {
		
			Console::UnsetHandler();
			
		} catch (...) {	}
	
		//	Shut down the command line interface
		console.Destroy();
	
	}
	
	
	void CLI::clear_provider () noexcept {
	
		//	Clear old provider if it exists
		
		if (provider!=nullptr) provider->Clear();
		
		provider=nullptr;
	
	}
	
	
	void CLI::SetProvider (CLIProvider * provider) {
	
		lock.Execute([&] () mutable {
		
			//	Clear old provider
			clear_provider();
			
			//	Don't do anything if new provider is
			//	null -- avoid SIGSEGV
			if (provider==nullptr) return;
			
			this->provider=provider;
			
			//	Set our handlers
			provider->Set(
				[this] (const String & str, Service::LogType type) mutable {
				
					log(str,type);
				
				},
				[this] (const String & from, const Vector<String> & to, const String & msg, const Nullable<String> & notes) mutable {
			
					chat_log(from,to,msg,notes);
				
				}
			);
		
		});
	
	}
	
	
	void CLI::ClearProvider () noexcept {
	
		lock.Execute([&] () mutable {	clear_provider();	});
	
	}
	
	
	void CLI::panic (std::exception_ptr) noexcept {
	
		stop(ShutdownReason::Panic);
	
	}
	
	
	void CLI::update_history (String str, Vector<String> & history) {
	
		//	Prune if getting too long
		if (history.Count()==output_max) history.Delete(0);
	
		//	Add
		history.Add(std::move(str));
	
	}
	
	
	void CLI::output (String str, OutputType type) {
	
		try {
		
			history_lock.Execute([&] () mutable {
			
				//	Update histories
				if (
					(type==OutputType::Chat) ||
					(type==OutputType::Both)
				) update_history(str,chat_history);
				if (
					(type==OutputType::Log) ||
					(type==OutputType::Both)
				) update_history(str,log_history);
				
				//	Write if applicable
				if (
					(type==OutputType::Both) ||
					((type==OutputType::Chat)==chat_selected)
				) console->WriteLine(std::move(str));
			
			});
			
		} catch (...) {
		
			panic(std::current_exception());
			
			throw;
		
		}
	
	}
	
	
	void CLI::switch_to (const Vector<String> & history) {
	
		console->Clear();
		
		console->WriteLines(history);
	
	}
	
	
	void CLI::switch_display (bool chat) {
	
		try {
		
			history_lock.Execute([&] () mutable {
			
				//	If the correct output is already
				//	selected, do nothing
				if (chat_selected==chat) return;
				
				//	Otherwise, switch to the correct
				//	display
				
				chat_selected=chat;
				
				switch_to(chat ? chat_history : log_history);
			
			});
		
		} catch (...) {
		
			panic(std::current_exception());
			
			throw;
		
		}
	
	}
	
	
	static const String to_chat("chat");
	static const String to_log("log");
	
	
	bool CLI::command (String in) {
	
		//	Get rid of leading/trailing
		//	whitespace
		in.Trim();
		//	Convert to all lowercase
		in.ToLower();
		
		//	If it's the to chat command,
		//	switch
		if (in==to_chat) {
		
			switch_display(true);
			
			return true;
			
		}
		
		//	If it's the to log command,
		//	switch
		if (in==to_log) {
		
			switch_display(false);
			
			return true;
		
		}
		
		//	We can't handle this command,
		//	fail out
		return false;
	
	}
	
	
	static const Regex parse_command(
		"^\\/(.*)$",
		RegexOptions().SetSingleline()
	);
	
	
	void CLI::input (String in) {
	
		try {
	
			//	Is the input empty?
			//	If so ignore
			if (in.Size()==0) return;
			
			//	Is the input a command?
			//	If so, is it a command we handle?
			//	If so, return at once
			auto match=parse_command.Match(in);
			if (
				match.Success() &&
				command(match[1].Value())
			) return;
			
			//	We can't handle this input, pass it
			//	through
			lock.Execute([&] () mutable {
			
				//	If there's no provider, we're done
				if (provider==nullptr) return;
				
				//	Get the provider to handle this
				(*provider)(std::move(in));
			
			});
			
		} catch (...) {
		
			panic(std::current_exception());
			
			throw;
		
		}
	
	}
	
	
	void CLI::stop (ShutdownReason reason) noexcept {
	
		lock.Execute([&] () mutable {
		
			this->reason=reason;
			
			if (provider!=nullptr) {
			
				provider->Clear();	
				provider=nullptr;
			
			}
			
			wait.WakeAll();
		
		});
	
	}
	
	
	static const String recipient_separator(",");
	static const String chat_to_template("[{0}] {1} => {2}: {3}");
	static const String chat_global_template("[{0}] {1}: {3}");
	
	
	void CLI::chat_log (const String & from, const Vector<String> & to, const String & msg, const Nullable<String> &) {
	
		try {
		
			String to_str;
			for (const auto & s : to) {
			
				if (to.Size()==0) to_str << recipient_separator;
				
				to_str << s;
			
			}
			
			output(
				String::Format(
					(to.Count()==0) ? chat_global_template : chat_to_template,
					GetDateTime(),
					from,
					to_str,
					msg
				),
				OutputType::Chat
			);
		
		} catch (...) {
		
			panic(std::current_exception());
			
			throw;
		
		}
	
	}
	
	
	static const String log_template("[{0}] [{1}]: {2}");
	
	
	void CLI::log (const String & str, Service::LogType type) {
	
		try {
		
			output(
				String::Format(
					log_template,
					GetDateTime(),
					DataProvider::GetLogType(type).ToUpper(),
					str
				),
				OutputType::Log
			);
		
		} catch (...) {
		
			panic(std::current_exception());
			
			throw;
		
		}
	
	}
	
	
	CLI::ShutdownReason CLI::Wait () const noexcept {
	
		return lock.Execute([&] () {
		
			while (reason==ShutdownReason::None) wait.Sleep(lock);
		
			return reason;
		
		});
	
	}
	
	
	CLI::ShutdownReason CLI::GetState () const noexcept {
	
		return lock.Execute([&] () {	return reason;	});
	
	}
	
	
	void CLI::Shutdown () noexcept {
	
		stop(ShutdownReason::Stop);
	
	}
	
	
	void CLI::WriteLine (String str) {
	
		output(std::move(str),OutputType::Both);
	
	}
	

}

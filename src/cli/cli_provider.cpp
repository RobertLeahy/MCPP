#include <cli/cli.hpp>
#include <utility>


namespace MCPP {


	//	Default implementation does nothing
	void CLIProvider::operator () (String) {	}
	
	
	void CLIProvider::Set (LogType log, ChatLogType chat_log) noexcept {
	
		lock.Execute([&] () mutable {
		
			this->log=std::move(log);
			this->chat_log=std::move(chat_log);
		
		});
	
	}
	
	
	void CLIProvider::Clear () noexcept {
	
		lock.Execute([&] () mutable {
		
			log=LogType();
			chat_log=ChatLogType();
		
		});
	
	}
	
	
	void CLIProvider::WriteLog (const String & str, Service::LogType type) const {
	
		lock.Execute([&] () {	if (log) log(str,type);	});
	
	}
	
	
	void CLIProvider::WriteChatLog (const String & from, const Vector<String> & to, const String & msg, const Nullable<String> & notes) const {
	
		lock.Execute([&] () {	if (chat_log) chat_log(from,to,msg,notes);	});
	
	}


}

#include <command/command.hpp>


namespace MCPP {


	Vector<String> Command::AutoComplete (const CommandEvent &) {
	
		return Vector<String>();
	
	}
	
	
	bool Command::Check (const CommandEvent &) {
	
		return true;
	
	}


}

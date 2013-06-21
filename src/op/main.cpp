#include <op/op.hpp>
#include <utility>


namespace MCPP {


	static const String name("Op Support");
	static const Word priority=1;
	static const String key("op");
	
	
	Word OpModule::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & OpModule::Name () const noexcept {
	
		return name;
	
	}
	
	
	void OpModule::Install () {
	
		//	Build the op table
		
		for (auto & op : RunningServer->Data().GetValues(key)) {
		
			if (!op.IsNull()) ops.insert(op->ToLower());
		
		}
	
	}
	
	
	bool OpModule::IsOp (String username) {
	
		username.ToLower();
	
		return lock.Read([&] () {	return ops.find(username)!=ops.end();	});
	
	}
	
	
	void OpModule::DeOp (String username) {
	
		username.ToLower();
	
		lock.Write([&] () {
		
			//	Remove from the op table
			ops.erase(username);
			
			//	Remove from the backing store
			RunningServer->Data().DeleteValues(key,username);
		
		});
	
	}
	
	
	void OpModule::Op (String username) {
	
		username.ToLower();
	
		lock.Write([&] () {
		
			//	Attempt insertion
			auto pair=ops.insert(username);
			
			//	If the person is already an
			//	op, we don't want to add
			//	them to the backing store
			//	again
			if (pair.second) RunningServer->Data().InsertValue(key,username);
		
		});
	
	}
	
	
	Vector<String> OpModule::List () const {
	
		return lock.Read([&] () {
		
			//	Prepare a sufficiently-sized
			//	buffer
			Vector<String> list(ops.size());
			
			//	Populate
			for (auto & s : ops) list.Add(s);
			
			return list;
		
		});
	
	}
	
	
	Nullable<OpModule> Ops;


}


extern "C" {


	Module * Load () {
	
		if (Ops.IsNull()) try {
		
			Ops.Construct();
			
			return &(*Ops);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		if (!Ops.IsNull()) Ops.Destroy();
	
	}


}

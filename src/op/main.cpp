#include <op/op.hpp>
#include <singleton.hpp>
#include <utility>


namespace MCPP {


	static const String name("Op Support");
	static const Word priority=1;
	static const String key("op");
	
	
	Word Ops::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & Ops::Name () const noexcept {
	
		return name;
	
	}
	
	
	void Ops::Install () {
	
		//	Build the op table
		
		for (auto & op : Server::Get().Data().GetValues(key)) ops.insert(op.ToLower());
	
	}
	
	
	bool Ops::IsOp (String username) {
	
		username.ToLower();
	
		return lock.Read([&] () {	return ops.find(username)!=ops.end();	});
	
	}
	
	
	void Ops::DeOp (String username) {
	
		username.ToLower();
	
		lock.Write([&] () {
		
			//	Remove from the op table
			ops.erase(username);
			
			//	Remove from the backing store
			Server::Get().Data().DeleteValues(key,username);
		
		});
	
	}
	
	
	void Ops::Op (String username) {
	
		username.ToLower();
	
		lock.Write([&] () {
		
			//	Attempt insertion
			auto pair=ops.insert(username);
			
			//	If the person is already an
			//	op, we don't want to add
			//	them to the backing store
			//	again
			if (pair.second) Server::Get().Data().InsertValue(key,username);
		
		});
	
	}
	
	
	Vector<String> Ops::List () const {
	
		return lock.Read([&] () {
		
			//	Prepare a sufficiently-sized
			//	buffer
			Vector<String> list(ops.size());
			
			//	Populate
			for (auto & s : ops) list.Add(s);
			
			return list;
		
		});
	
	}
	
	
	static Singleton<Ops> singleton;
	
	
	Ops & Ops::Get () noexcept {
	
		return singleton.Get();
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(singleton.Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

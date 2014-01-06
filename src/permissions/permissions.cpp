#include <permissions/permissions.hpp>
#include <save/save.hpp>
#include <serializer.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <cstring>
#include <exception>
#include <type_traits>


using namespace MCPP;
using namespace MCPP::PermissionsImpl;


namespace MCPP {


	static const Word priority=1;
	static const String name("Permissions");
	
	
	static const String save_key("permissions");
	static const String error_parsing("Error parsing permissions tables: \"{0}\" at byte {1}");
	static const String debug_key("permissions");
	
	
	bool Permissions::is_verbose () {
	
		return Server::Get().IsVerbose(debug_key);
	
	}
	
	
	bool Permissions::check (const PermissionsSet & set, const String & permission, std::unordered_set<String> & checked) const {
	
		if (set.Individual.Contains(permission)) return true;
		
		for (auto & group : set.Groups) {
		
			//	Consider the case where group "foo"
			//	includes group "bar", and group "bar"
			//	includes group "foo".  Without some way
			//	to track which groups we've recursively
			//	checked we'd recurse until a stack
			//	overflow occurred.
			//
			//	Therefore before considering any group,
			//	we consult the checked set, and if the
			//	group has already been checked, we skip
			//	it
			if (checked.count(group)!=0) continue;
			
			//	Make sure we don't check this group
			//	again
			checked.insert(group);
			
			auto iter=groups.find(group);
			
			//	Skip groups that don't exist
			if (iter==groups.end()) continue;
			
			//	If a given group doesn't contain the
			//	permission-in-question, that doesn't mean
			//	that we should fail at once, we have to
			//	check ALL groups
			if (check(iter->second,permission,checked)) return true;
		
		}
		
		return false;
	
	}
	
	
	bool Permissions::check (const PermissionsSet & set, const String & permission) const {
	
		std::unordered_set<String> checked;
		
		return check(set,permission,checked);
	
	}
	
	
	bool Permissions::check_all (const PermissionsSet & set, Set<String> & s, std::unordered_set<String> & checked) const {
	
		//	Add all permissions of this set into
		//	the set of effective permissions we've
		//	built for this user thus far
		s.Union(set.Individual);
		
		//	If the built permissions set is full
		//	with nothing removed from it, the user
		//	or group effectively has all permissions,
		//	and we're done
		if (s.Full() && (s.Count()==0)) return true;
		
		//	Loop and consider each group attached to
		//	this user or group
		for (auto & group : set.Groups) {
		
			//	Skip groups already checked to avoid
			//	looping infinitely over a cycle
			if (checked.count(group)!=0) continue;
			
			//	Make sure this group gets skipped when
			//	and/or if it's encountered again
			checked.insert(group);
			
			auto iter=groups.find(group);
			
			//	If a group doesn't exist, skip it
			if (iter==groups.end()) continue;
			
			//	Recurse on this group
			if (check_all(iter->second,s,checked)) return true;
		
		}
		
		return false;
	
	}
	
	
	bool Permissions::check_all (const PermissionsSet & set) const {
	
		Set<String> s;
		std::unordered_set<String> checked;
		
		return check_all(set,s,checked);
	
	}
	
	
	bool Permissions::is_member (const PermissionsSet & set, const String & group, std::unordered_set<String> & checked) const {
	
		if (set.Groups.count(group)!=0) return true;
		
		for (auto & g : set.Groups) {
		
			//	Do not infinitely recurse on a
			//	cycle
			if (checked.count(g)!=0) continue;
			
			//	Ensure this group is not checked
			//	again further down the recursion
			checked.insert(g);
			
			auto iter=groups.find(g);
			
			//	Skip groups that don't exist
			if (iter==groups.end()) continue;
			
			//	Recurse
			if (is_member(iter->second,group,checked)) return true;
		
		}
		
		return false;
	
	}
	
	
	bool Permissions::is_member (const PermissionsSet & set, const String & group) const {
	
		std::unordered_set<String> checked;
		
		return is_member(set,group,checked);
	
	}
	
	
	template <typename T>
	class Serializer<Set<T>> {
	
	
		private:
		
		
			typedef typename std::decay<T>::type type;
	
	
		public:
		
		
			static Set<T> FromBytes (const Byte * & begin, const Byte * end) {
			
				auto full=Serializer<bool>::FromBytes(begin,end);
				Set<T> retr(full);
			
				auto len=Serializer<UInt32>::FromBytes(begin,end);
				
				for (UInt32 i=0;i<len;++i) {
				
					auto obj=Serializer<type>::FromBytes(begin,end);
					
					if (full) retr.Remove(std::move(obj));
					else retr.Add(std::move(obj));
				
				}
				
				return retr;
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const Set<T> & obj) {
			
				Serializer<bool>::ToBytes(buffer,obj.Full());
				
				Serializer<UInt32>::ToBytes(
					buffer,
					static_cast<UInt32>(SafeWord(obj.Count()))
				);
				
				for (const auto & o : obj) Serializer<type>::ToBytes(buffer,o);
			
			}
	
	
	};
	
	
	template <>
	class Serializer<PermissionsSet> {
	
	
		public:
		
		
			static PermissionsSet FromBytes (const Byte * & begin, const Byte * end) {
			
				auto groups=Serializer<std::unordered_set<String>>::FromBytes(begin,end);
				return PermissionsSet{
					Serializer<Set<String>>::FromBytes(begin,end),
					std::move(groups)
				};
			
			}
			
			
			static void ToBytes (Vector<Byte> & buffer, const PermissionsSet & obj) {
			
				Serializer<std::unordered_set<String>>::ToBytes(buffer,obj.Groups);
				Serializer<Set<String>>::ToBytes(buffer,obj.Individual);
			
			}
	
	
	};
	
	
	void Permissions::load () {
		
		auto buffer=ByteBuffer::Load(save_key);
		
		//	If nothing was loaded, end at once
		if (buffer.Count()==0) return;
		
		std::unordered_map<String,PermissionsSet> groups;
		std::unordered_map<String,PermissionsSet> users;
		try {
		
			groups=buffer.FromBytes<decltype(groups)>();
			users=buffer.FromBytes<decltype(users)>();
		
		} catch (const ByteBufferError & e) {
		
			//	Log problem
			Server::Get().WriteLog(
				String::Format(
					error_parsing,
					e.what(),
					e.Where()
				),
				Service::LogType::Error
			);
			
			return;
		
		}
		
		//	Move results into this object
		this->users=std::move(users);
		this->groups=std::move(groups);
	
	}
	
	
	void Permissions::save () const {
	
		ByteBuffer buffer;
	
		lock.Read([&] () {
		
			buffer.ToBytes(groups);
			buffer.ToBytes(users);
		
		});
		
		buffer.Save(save_key);
	
	}


	static Singleton<Permissions> singleton;


	Permissions & Permissions::Get () noexcept {
	
		return singleton.Get();
	
	}
	
	
	static const char * invalid_client="Invalid client";
	
	
	PermissionsHandle Permissions::GetUser (const SmartPointer<Client> & client) {
	
		//	Check for invalid client
		if (
			client.IsNull() ||
			(client->GetState()!=ProtocolState::Play)
		) throw std::invalid_argument(invalid_client);
		
		return PermissionsHandle(
			*this,
			false,
			client->GetUsername()
		);
	
	}
	
	
	PermissionsHandle Permissions::GetUser (String username) {
	
		return PermissionsHandle(
			*this,
			false,
			std::move(username)
		);
	
	}
	
	
	PermissionsHandle Permissions::GetGroup (String name) {
	
		return PermissionsHandle(
			*this,
			true,
			std::move(name)
		);
	
	}
	
	
	PermissionsTables Permissions::GetTables () const {
	
		return lock.Read([&] () {
		
			Vector<PermissionsTableEntry> users(this->users.size());
			for (auto & pair : this->users) users.EmplaceBack(
				pair.first,
				pair.second
			);
			
			Vector<PermissionsTableEntry> groups(this->groups.size());
			for (auto & pair : this->groups) groups.EmplaceBack(
				pair.first,
				pair.second
			);
			
			return PermissionsTables{
				std::move(users),
				std::move(groups)
			};
		
		});
	
	}
	
	
	Word Permissions::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & Permissions::Name () const noexcept {
	
		return name;
	
	}
	
	
	void Permissions::Install () {
		
		//	Load permissions tables
		load();
		
		//	Subscribe to save loop
		SaveManager::Get().Add([this] () {	save();	});
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(Permissions::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

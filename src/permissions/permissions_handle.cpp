#include <permissions/permissions.hpp>
#include <server.hpp>


using namespace MCPP::PermissionsImpl;


namespace MCPP {


	static const String grant_log("Granting {0} \"{1}\" permission \"{2}\"");
	static const String grant_all_log("Granting {0} \"{1}\" all permissions");
	static const String revoke_log("Revoking {0} \"{1}\" permission \"{2}\"");
	static const String revoke_all_log("Revoking {0} \"{1}\" all permissions");
	static const String check_log("Checked {0} \"{1}\" for permission \"{2}\": {3}");
	static const String check_all_log("Checked {0} \"{1}\" for all permissions: {2}");
	static const String is_member_log("Checked {0} \"{1}\" for membership in group \"{2}\": {3}");
	static const String add_log("Adding group \"{2}\" to {0} \"{1}\"");
	static const String remove_log("Removing group \"{2}\" from {0} \"{1}\"");
	static const String true_string("true");
	static const String false_string("false");
	static const String group_string("group");
	static const String user_string("user");
	
	
	static bool is_empty (const PermissionsSet & set) noexcept {
	
		return (
			!set.Individual.Full() &&
			(set.Individual.Count()==0) &&
			(set.Groups.size()==0)
		);
	
	}
	
	
	std::unordered_map<String,PermissionsSet> & PermissionsHandle::map () noexcept {
	
		return group ? permissions.groups : permissions.users;
	
	}
	
	
	const std::unordered_map<String,PermissionsSet> & PermissionsHandle::map () const noexcept {
	
		return group ? permissions.groups : permissions.users;
	
	}
	
	
	template <typename... Args>
	void PermissionsHandle::log (const String & temp, const Args &... args) const {
	
		if (permissions.is_verbose()) Server::Get().WriteLog(
			String::Format(
				temp,
				group ? group_string : user_string,
				name,
				args...
			),
			Service::LogType::Debug
		);
	
	}
	
	
	std::unordered_map<String,PermissionsSet>::iterator PermissionsHandle::create () {
	
		auto & map=this->map();
		
		auto iter=map.find(name);
		
		if (iter==map.end()) {
		
			auto pair=map.emplace(name,PermissionsSet());
			
			iter=std::move(pair.first);
		
		}
		
		return iter;
	
	}
	
	
	PermissionsHandle::PermissionsHandle (Permissions & permissions, bool group, String name) : group(group), name(std::move(name)), permissions(permissions) {
	
		//	If a user, normalize username
		//	to lower case
		if (!group) this->name.ToLower();
	
	}
	
	
	bool PermissionsHandle::IsGroup () const noexcept {
	
		return group;
	
	}
	
	
	const String & PermissionsHandle::Name () const noexcept {
	
		return name;
	
	}
	
	
	bool PermissionsHandle::Check (const String & permission) const {
	
		bool result=permissions.lock.Read([&] () {
		
			auto & map=this->map();
		
			auto iter=map.find(name);
			
			return (iter==map.end()) ? false : permissions.check(iter->second,permission);
		
		});
		
		log(check_log,permission,result ? true_string : false_string);
		
		return result;
	
	}
	
	
	bool PermissionsHandle::CheckAll () const {
	
		bool result=permissions.lock.Read([&] () {
		
			auto & map=this->map();
			
			auto iter=map.find(name);
			
			return (iter==map.end()) ? false : permissions.check_all(iter->second);
		
		});
		
		log(check_all_log,result ? true_string : false_string);
		
		return result;
	
	}
	
	
	bool PermissionsHandle::IsMember (const String & group) const {
	
		bool result=permissions.lock.Read([&] () {
		
			auto & map=this->map();
			
			auto iter=map.find(name);
			
			return (iter==map.end()) ? false : permissions.is_member(iter->second,group);
		
		});
		
		log(is_member_log,group,result ? true_string : false_string);
		
		return result;
	
	}
	
	
	void PermissionsHandle::Grant (String permission) {
	
		log(grant_log,permission);
		
		permissions.lock.Write([&] () mutable {	create()->second.Individual.Add(std::move(permission));	});
	
	}
	
	
	void PermissionsHandle::Grant () {
	
		log(grant_all_log);
		
		permissions.lock.Write([&] () mutable {	create()->second.Individual.Fill();	});
	
	}
	
	
	void PermissionsHandle::Revoke (String permission) {
	
		log(revoke_log,permission);
		
		permissions.lock.Write([&] () mutable {
		
			auto & map=this->map();
		
			auto iter=map.find(name);
			
			if (iter==map.end()) return;
			
			iter->second.Individual.Remove(std::move(permission));
			
			if (is_empty(iter->second)) map.erase(iter);
		
		});
	
	}
	
	
	void PermissionsHandle::Revoke () {
	
		log(revoke_all_log);
		
		permissions.lock.Write([&] () mutable {
		
			auto & map=this->map();
			
			auto iter=map.find(name);
			
			if (iter==map.end()) return;
			
			iter->second.Individual.Empty();
			
			if (is_empty(iter->second)) map.erase(iter);
		
		});
	
	}
	
	
	void PermissionsHandle::Add (String group) {
	
		log(add_log,group);
		
		permissions.lock.Write([&] () mutable {	create()->second.Groups.insert(std::move(group));	});
	
	}
	
	
	void PermissionsHandle::Remove (const String & group) {
	
		log(remove_log,group);
		
		permissions.lock.Write([&] () mutable {
		
			auto & map=this->map();
			
			auto iter=map.find(name);
			
			if (iter==map.end()) return;
			
			iter->second.Groups.erase(group);
			
			if (is_empty(iter->second)) map.erase(iter);
		
		});
	
	}
	
	
	PermissionsTableEntry PermissionsHandle::Get () const {
	
		return permissions.lock.Read([&] () {
		
			auto & map=this->map();
			
			auto iter=map.find(name);
			
			return (iter==map.end()) ? PermissionsTableEntry() : PermissionsTableEntry(iter->first,iter->second);
		
		});
	
	}

	
}

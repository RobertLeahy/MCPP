#include <permissions/permissions.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <cstring>
#include <exception>


using namespace MCPP;
using namespace MCPP::PermissionsImpl;


namespace MCPP {


	static const Word priority=1;
	static const String name("Permissions");
	
	
	static const String save_frequency_key("permissions_save_frequency");
	static const Word save_frequency_default=120000;
	static const String save_key("permissions");
	static const String error_parsing("Error parsing permissions tables: \"{0}\" at byte {1}");
	static const String debug_key("permissions");
	static const String save_log("Saved permissions tables - took {0}ns - next in {1}ms");
	
	
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
	
	
	class PermissionsParserError : public std::exception {
	
	
		public:
		
		
			Word Where;
			const String & What;
			
			
			PermissionsParserError (Word where, const String & what) : Where(where), What(what) {	}
	
	
	};
	
	
	class PermissionsParserResult {
	
	
		public:
		
		
			std::unordered_map<String,PermissionsSet> Users;
			std::unordered_map<String,PermissionsSet> Groups;
	
	
	};
	
	
	static const String & insufficient_bytes="Insufficient bytes";
	static const String & invalid_boolean="Invalid boolean value";
	
	
	class PermissionsParser {
	
	
		private:
		
		
			const Byte * start;
			const Byte * begin;
			const Byte * end;
			Vector<Byte> buffer;
			
			
			Word loc () const noexcept {
			
				return static_cast<Word>(begin-start);
			
			}
			
			
			[[noreturn]]
			void raise () const {
			
				throw PermissionsParserError(
					loc(),
					insufficient_bytes
				);
			
			}
			
			
			void check (Word len) const {
			
				if (static_cast<Word>(end-begin)<len) raise();
			
			}
			
			
			bool get_boolean () {
			
				//	Make sure we can actually extract
				//	a byte
				if (begin==end) raise();
				
				//	Make sure next byte is a valid
				//	boolean value
				if (!(
					(*begin==0) ||
					(*begin==1)
				)) throw PermissionsParserError(
					loc(),
					invalid_boolean
				);
				
				bool retr=*begin==1;
				
				++begin;
				
				return retr;
			
			}
			
			
			Word get_length () {
			
				//	Make sure we can actually extract
				//	a 32-bit unsigned integer
				check(sizeof(UInt32));
				
				UInt32 val;
				std::memcpy(&val,begin,sizeof(val));
				begin+=sizeof(val);
				
				//	Fix endianness if necessary
				if (!Endianness::IsBigEndian<UInt32>()) Endianness::FixEndianness(&val);
				
				return val;
			
			}
			
			
			String get_string () {
			
				//	Get the length of the string (in code units)
				auto len=get_length();
				
				//	Check to make sure there's enough bytes
				//	to extract
				check(len);
				
				//	Decode
				auto retr=UTF8().Decode(
					begin,
					begin+len
				);
				
				begin+=len;
				
				return retr;
			
			}
			
			
			PermissionsSet get_permissions_set () {
			
				PermissionsSet retr;
				
				//	Get number of groups
				Word groups=get_length();
				//	Loop and get each of those groups
				for (Word i=0;i<groups;++i) retr.Groups.insert(get_string());
				
				//	Is the set default full or
				//	default empty?
				bool full=get_boolean();
				
				//	If the set is full, fill it
				if (full) retr.Individual.Fill();
				
				//	Get number of permissions added to
				//	or removed from the set
				Word individual=get_length();
				//	Loop and get each of them
				for (Word i=0;i<individual;++i) {
				
					if (full) retr.Individual.Remove(get_string());
					else retr.Individual.Add(get_string());
				
				}
				
				return retr;
			
			}
			
			
			void write (bool bln) {
			
				buffer.Add(bln ? 1 : 0);
			
			}
			
			
			void write (Word len) {
			
				union {
					UInt32 val;
					Byte buffer [sizeof(val)];
				};
				val=static_cast<UInt32>(SafeWord(len));
				
				if (!Endianness::IsBigEndian<Word>()) Endianness::FixEndianness(&val);
				
				for (auto b : buffer) this->buffer.Add(b);
			
			}
			
			
			void write (const String & str) {
			
				auto encoded=UTF8().Encode(str);
				
				write(encoded.Count());
				
				for (auto b : encoded) buffer.Add(b);
			
			}
			
			
			void write (const PermissionsSet & set) {
			
				//	Write groups
				write(set.Groups.size());
				for (auto & str : set.Groups) write(str);
				
				//	Write set
				write(set.Individual.Full());
				write(set.Individual.Count());
				for (auto & str : set.Individual) write(str);
			
			}
			
			
			void write (const std::unordered_map<String,PermissionsSet> & map) {
			
				write(map.size());
				
				for (auto & pair : map) {
				
					write(pair.first);
					write(pair.second);
				
				}
			
			}
			
			
		public:
		
		
			PermissionsParserResult operator () (const Byte * begin, const Byte * end) {
			
				this->start=begin;
				this->begin=begin;
				this->end=end;
				
				PermissionsParserResult retr;
				
				//	Get the number of groups
				Word groups=get_length();
				//	Loop and get each of those groups
				for (Word i=0;i<groups;++i) {
				
					//	Get the name
					auto name=get_string();
					
					//	Get the associated permissions
					//	set
					auto set=get_permissions_set();
					
					//	Add to set
					retr.Groups.emplace(
						std::move(name),
						std::move(set)
					);
				
				}
				
				//	Get the number of users
				Word users=get_length();
				//	Loop and get each of those users
				for (Word i=0;i<users;++i) {
				
					//	Get the name
					auto name=get_string();
					//	All usernames are all lowercase
					name.ToLower();
					
					//	Get the associated permissions
					//	set
					auto set=get_permissions_set();
					
					//	Add to set
					retr.Users.emplace(
						std::move(name),
						std::move(set)
					);
				
				}
				
				//	Done!
				return retr;
			
			}
			
			
			Vector<Byte> operator () (const std::unordered_map<String,PermissionsSet> & users, const std::unordered_map<String,PermissionsSet> & groups) {
				
				write(groups);
				write(users);
				
				return std::move(buffer);
			
			}
	
	
	};
	
	
	void Permissions::load () {
	
		//	Permissions are saved as follows
		//	(all big endian):
		//
		//	1.	An unsigned 32-bit integer
		//		specifying the number of groups
		//	2.	A number of groups equal to
		//		the number given.
		//	3.	An unsigned 32-bit integer
		//		specifying the number of users.
		//	4.	A number of users equal to the
		//		number given.
		//
		//	Users and groups are stored as follows:
		//
		//	1.	Name.
		//	2.	Number of associated groups.
		//	3.	Names of associated groups.
		//	4.	Whether the set is full or not.
		//	5.	Number of permissions either in the
		//		set, or removed from it.
		//	6.	Names of all permissions either in
		//		the set (if set is not full) or removed
		//		from the set (if the set is full).
		//
		//	Strings are serialized as:
		//
		//	1.	Length (in code units).
		//	2.	UTF-8 encoding.
		
		auto & server=Server::Get();
	
		//	Attempt to get data from backing store
		auto buffer=server.Data().GetBinary(save_key);
		
		//	If nothing was loaded, proceed
		if (buffer.IsNull()) return;
		
		PermissionsParserResult result;
		try {
		
			result=PermissionsParser()(
				buffer->begin(),
				buffer->end()
			);
		
		} catch (const PermissionsParserError & e) {
		
			//	Log problem
			server.WriteLog(
				String::Format(
					error_parsing,
					e.What,
					e.Where
				),
				Service::LogType::Error
			);
			
			return;
		
		}
		
		//	Move results into this object
		users=std::move(result.Users);
		groups=std::move(result.Groups);
	
	}
	
	
	void Permissions::save () const {
	
		//	Read lock while we save
		auto buffer=lock.Read([&] () {	return PermissionsParser()(users,groups);	});
		
		//	Write
		Server::Get().Data().SaveBinary(save_key,buffer.begin(),buffer.Count());
	
	}
	
	
	void Permissions::save_loop () const {
	
		auto & server=Server::Get();
	
		try {
		
			Timer timer(Timer::CreateAndStart());
	
			//	Save
			save();
			
			auto elapsed=timer.ElapsedNanoseconds();
			
			//	Debug log if applicable
			if (is_verbose()) server.WriteLog(
				String::Format(
					save_log,
					elapsed,
					save_frequency
				),
				Service::LogType::Debug
			);
			
			//	Loop
			server.Pool().Enqueue(
				save_frequency,
				[this] () {	save_loop();	}
			);
			
		} catch (...) {
		
			//	Panic on throw
			
			try {
			
				server.Panic(std::current_exception());
				
			} catch (...) {	}
			
			throw;
		
		}
	
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
	
		auto & server=Server::Get();
		
		//	Hook into shutdown event (to save
		//	permissions tables)
		server.OnShutdown.Add([this] () {	save();	});
		
		//	Load settings
		save_frequency=server.Data().GetSetting(save_frequency_key,save_frequency_default);
		
		//	Load permissions tables
		load();
		
		//	Begin save loop
		server.Pool().Enqueue(
			save_frequency,
			[this] () {	save_loop();	}
		);
	
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

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <client.hpp>
#include <hash.hpp>
#include <mod.hpp>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>


namespace MCPP {


	/**
	 *	\cond
	 */


	namespace PermissionsImpl {
	
	
		template <typename T>
		class Set {
		
		
			private:
			
			
				//	Whether the set is full, and
				//	the set represents elements
				//	removed from it, or the set is
				//	empty, and the set represents
				//	elements added to it
				bool full;
				//	Items either added to or removed
				//	from the set
				std::unordered_set<T> set;
				
				
				void raw_union (const Set & other) {
				
					//	Union is accomplished by simply
					//	looping and attempting to insert
					//	each element from the other set,
					//	the objects already in this set
					//	will not be inserted, and therefore
					//	the result will be the union
					//	of this and other
					for (auto & obj : other.set) set.insert(obj);
				
				}
				
				
				void raw_intersect (const Set & other) {
				
					//	Intersect is accomplished by simply
					//	looping and looking up each element
					//	in this set in the other set.  If
					//	an element is in only one set it's
					//	not in the intersection, and therefore
					//	is erased
					for (auto begin=set.begin();begin!=set.end();++begin)
					if (other.set.count(*begin)==0) {
					
						//	Avoid iterator invalidation
						auto curr=begin;
						++begin;
						
						//	Delete
						set.erase(curr);
						
						//	Make sure we didn't hit the
						//	end
						if (begin==set.end()) break;
					
					}
				
				}
				
				
				void raw_relative_complement (const Set & other, bool of_this) {
				
					//	The goal is to get all the elements
					//	from either other or this into this
					//	set that ARE NOT in the opposite set
					
					//	Create a new set
					std::unordered_set<T> result;
					
					//	Loop and only add elements which
					//	are not in the set we're taking the
					//	relative complement of
					for (auto & obj : (of_this ? other.set : set))
					if ((of_this ? set.count(obj) : other.set.count(obj))==0)
					result.insert(obj);
					
					//	Replace this set with the result
					set=std::move(result);
				
				}
				
				
			public:
			
			
				Set (bool full=false) noexcept : full(full) {	}
				
				
				void Add (const T & obj) {
				
					if (full) set.erase(obj);
					else set.insert(obj);
				
				}
				
				
				void Add (T && obj) {
				
					if (full) set.erase(obj);
					else set.insert(std::move(obj));
				
				}
				
				
				void Remove (const T & obj) {
				
					if (full) set.insert(obj);
					else set.erase(obj);
				
				}
				
				
				void Remove (T && obj) {
				
					if (full) set.insert(std::move(obj));
					else set.erase(obj);
				
				}
				
				
				bool Contains (const T & obj) const {
				
					bool contains=set.count(obj)!=0;
					
					return full ? !contains : contains;
				
				}
				
				
				void Fill () noexcept {
				
					set.clear();
					
					full=true;
				
				}
				
				
				void Empty () noexcept {
				
					set.clear();
					
					full=false;
				
				}
				
				
				void Complement () {
				
					full=!full;
				
				}
				
				
				void Union (const Set & other) {
				
					if (full) {
					
						if (other.full) {
						
							//	If both sets are full, their
							//	sets represent the elements that
							//	they do NOT contain.  Therefore
							//	the intersection of those two
							//	sets -- i.e. the elements that
							//	neither of them contain -- should
							//	be placed in the set of the
							//	resulting set
							
							raw_intersect(other);
						
						} else {
						
							//	If this set is full, but the other
							//	set isn't, the result is this set
							//	(which contains all elements),
							//	minus the elements removed from this
							//	set, but NOT added back in, in the
							//	other set, i.e. the result is
							//	the relative complement of B in
							//	A, where A is this set, and B is 
							//	the other set, i.e. all elements in
							//	this set BUT NOT in the other set,
							//	i.e. all elements removed from "all"
							//	by this set, BUT NOT added back in
							//	by the other set
							
							raw_relative_complement(other,false);
						
						}
					
					} else if (other.full) {
					
						//	If this set isn't full, but the other
						//	set is, the logic from the opposite case
						//	above applies in reverse, with the
						//	additional step of "filling" this set
					
						raw_relative_complement(other,true);
						
						full=true;
					
					} else {
					
						//	The simple case is when neither set is
						//	full, and a plain union is all that's
						//	needed
					
						raw_union(other);
					
					}
				
				}
				
				
				void Intersect (const Set & other) {
				
					if (full) {
					
						if (other.full) {
						
							//	If both sets are full, the elements
							//	that are common between them is
							//	obtained by unioning the elements
							//	removed from them
							
							raw_union(other);
						
						} else {
						
							//	If one set is full, and the other
							//	isn't, the result is all elements that
							//	are in the empty set, but not in the
							//	full set, since elements in the full
							//	set are explicitly removed (and therefore
							//	shouldn't appear in the intersection)
							
							raw_relative_complement(other,true);
							
							//	After an intersection, the resulting set
							//	contains a very particular set of elements,
							//	and therefore is no longer full
							full=false;
						
						}
					
					} else if (other.full) {
					
						//	See rationale above
					
						raw_relative_complement(other,false);
					
					} else {
					
						//	The simple case is when neither set is
						//	full, and a plain intersection is all
						//	that's needed
					
						raw_intersect(other);
					
					}
				
				}
				
				
				auto begin () const noexcept -> decltype(set.begin()) {
				
					return set.begin();
				
				}
				
				
				auto end () const noexcept -> decltype(set.end()) {
				
					return set.end();
				
				}
				
				
				bool Full () const noexcept {
				
					return full;
				
				}
				
				
				Word Count () const noexcept {
				
					return set.size();
				
				}
		
		
		};
		
		
		class PermissionsSet {
		
		
			public:
			
			
				Set<String> Individual;
				std::unordered_set<String> Groups;
		
		
		};
	

	}
	
	
	class Permissions;
	
	
	/**
	 *	\endcond
	 */
	 
	 
	/**
	 *	Contains the permissions for a single
	 *	user or group.
	 */
	class PermissionsTableEntry {
	
	
		public:
		
		
			/**
			 *	\cond
			 */
		
		
			PermissionsTableEntry () noexcept;
			PermissionsTableEntry (String, const PermissionsImpl::PermissionsSet &);
			
			
			/**
			 *	\endcond
			 */
	
		
			/**
			 *	The name of the user or group.
			 */
			String Name;
			/**
			 *	Whether the user or group's individual permissions,
			 *	i.e. the permissions associated exclusively with
			 *	them, and not any attached group, should be considered
			 *	to encompass all permissions, or no permissions by
			 *	default.
			 */
			bool Full;
			/**
			 *	The permissions which separate this user or group's
			 *	permissions from its default.  For example, if Full is
			 *	\em false, meaning that the default is for this user
			 *	or group to have no permissions, then all permissions
			 *	in Difference would be permissions that the user has.
			 *	If Full is \em true, meaning that the default is for
			 *	this user or group to have all permissions, then all
			 *	permissions in Difference would be permissions that the
			 *	user has been denied.
			 */
			Vector<String> Difference;
			/**
			 *	Groups that this user or group is a member of, and whose
			 *	permissions this user or group therefore inherits.
			 */
			Vector<String> Groups;
	
	
	};
	
	
	/**
	 *	Contains the permission tables associated
	 *	with a particular instance of the Permissions
	 *	class at a certain instant in time.
	 */
	class PermissionsTables {
	
	
		public:
		
		
			/**
			 *	Entries for all users with entries.
			 */
			Vector<PermissionsTableEntry> Users;
			/**
			 *	Entries for all groups with entries.
			 */
			Vector<PermissionsTableEntry> Groups;
	
	
	};
	 
	 
	/**
	 *	A handle to the permissions of a certain
	 *	user or group.
	 */
	class PermissionsHandle {
	
	
		friend class Permissions;
	
	
		private:
		
		
			//	Whether this represents
			//	a group or not
			bool group;
			//	The name of the group or
			//	user
			String name;
			//	A reference to the Permissions
			//	object which this object is a
			//	handle to
			Permissions & permissions;
			
			
			std::unordered_map<String,PermissionsImpl::PermissionsSet> & map () noexcept;
			const std::unordered_map<String,PermissionsImpl::PermissionsSet> & map () const noexcept;
			template <typename... Args>
			void log (const String &, const Args &...) const;
			std::unordered_map<String,PermissionsImpl::PermissionsSet>::iterator create ();
			
			
			PermissionsHandle (Permissions &, bool, String);
			
			
		public:
		
		
			/**
			 *	Determines whether this is a handle
			 *	to a group or to a user.
			 *
			 *	\return
			 *		\em true if this is a handle to
			 *		a group, \em false otherwise.
			 */
			bool IsGroup () const noexcept;
			/**
			 *	Determines the name of the user or
			 *	group this is a handle to.
			 *
			 *	\return
			 *		The name associated with this
			 *		handle.
			 */
			const String & Name () const noexcept;
		
		
			/**
			 *	Checks to see if this user or group
			 *	is granted a certain permission.
			 *
			 *	\param [in] permission
			 *		The permission to check for.
			 *
			 *	\return
			 *		\em true if this user or group
			 *		is granted \permission, \em false
			 *		otherwise.
			 */
			bool Check (const String & permission) const;
			/**
			 *	Checks to see if this user or group
			 *	has been granted all permissions.
			 *
			 *	\return
			 *		\em true if this user or group
			 *		has all permissions, \em false
			 *		otherwise.
			 */
			bool CheckAll () const;
			/**
			 *	Checks to see if this user or group is
			 *	a member of a certain group.
			 *
			 *	\param [in] group
			 *		The group to check for.
			 *
			 *	\return
			 *		\em true if this user or group is
			 *		in \em group, \em false otherwise.
			 */
			bool IsMember (const String & group) const;
			
			
			/**
			 *	Grants this user or group a permission.
			 *
			 *	\param [in] permission
			 *		The permission to grant.
			 */
			void Grant (String permission);
			/**
			 *	Grants this user or group all permissions.
			 */
			void Grant ();
			/**
			 *	Revokes a permission from this user or
			 *	group.
			 *
			 *	\param [in] permission
			 *		The permission to revoke.
			 */
			void Revoke (String permission);
			/**
			 *	Revokes all permissions from this user
			 *	or group.
			 */
			void Revoke ();
			/**
			 *	Adds a group to this user or group.
			 *
			 *	\param [in] group
			 *		The name of the group to add.
			 */
			void Add (String group);
			/**
			 *	Removes a group from this user or
			 *	group.
			 *
			 *	\param [in] group
			 *		The name of the group to remove.
			 */
			void Remove (const String & group);
			
			
			/**
			 *	Retrieves the permissions table entry
			 *	associated with this user or group.
			 *
			 *	\return
			 *		A permissions table entry.
			 */
			PermissionsTableEntry Get () const;
	
	
	};
	
	
	/**
	 *	Tracks the permissions that have been
	 *	granted certain users, and allows
	 *	permissions to be granted and revoked.
	 */
	class Permissions : public Module {
	
	
		friend class PermissionsHandle;
	
	
		private:
		
		
			std::unordered_map<String,PermissionsImpl::PermissionsSet> users;
			std::unordered_map<String,PermissionsImpl::PermissionsSet> groups;
			
			
			mutable RWLock lock;
			
			
			static bool is_verbose ();
			
			
			bool check (const PermissionsImpl::PermissionsSet &, const String &, std::unordered_set<String> &) const;
			bool check (const PermissionsImpl::PermissionsSet &, const String &) const;
			static bool full (const PermissionsImpl::PermissionsSet &) noexcept;
			bool check_all (const PermissionsImpl::PermissionsSet &, PermissionsImpl::Set<String> &, std::unordered_set<String> &) const;
			bool check_all (const PermissionsImpl::PermissionsSet &) const;
			bool is_member (const PermissionsImpl::PermissionsSet &, const String &, std::unordered_set<String> &) const;
			bool is_member (const PermissionsImpl::PermissionsSet &, const String &) const;
			
			
			void load ();
			void save () const;
			
			
		public:
		
		
			/**
			 *	Gets a valid instance of this class.
			 *
			 *	\return
			 *		A reference to a valid instance of
			 *		this class.
			 */
			static Permissions & Get () noexcept;
		
		
			/**
			 *	Gets a handle through which the permissions
			 *	of a user may be manipulated.
			 *
			 *	\except std::invalid_argument
			 *		Thrown when \em client is null, or when
			 *		it represents a client that is not authenticated.
			 *
			 *	\param [in] client
			 *		The client-in-question.
			 *
			 *	\return
			 *		A handle through which the permissions of
			 *		\em client may be manipulated.
			 */
			PermissionsHandle GetUser (const SmartPointer<Client> & client);
			/**
			 *	Gets a handle through which the permissions
			 *	of a user may be manipulated.
			 *
			 *	\param [in] username
			 *		The username-in-question.
			 *
			 *	\return
			 *		A handle through which the permissions of
			 *		\em username may be manipulated.
			 */
			PermissionsHandle GetUser (String username);
			/**
			 *	Gets a handle through which the permissions
			 *	of a group may be manipulated.
			 *
			 *	\param [in] name
			 *		The name of the group-in-question.
			 *
			 *	\return
			 *		A handle through which the permissions of
			 *		\em name may be manipulated.
			 */
			PermissionsHandle GetGroup (String name);
			
			
			/**
			 *	Gets all permissions tables.
			 *
			 *	\return
			 *		All permissions tables.
			 */
			PermissionsTables GetTables () const;
			
			
			/**
			 *	\cond
			 */
			 
			 
			virtual Word Priority () const noexcept override;
			virtual const String & Name () const noexcept override;
			virtual void Install () override;
			 
			 
			/**
			 *	\endcond
			 */
	
	
	};


}

	
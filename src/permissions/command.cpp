#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <command/command.hpp>
#include <permissions/permissions.hpp>
#include <mod.hpp>
#include <algorithm>


using namespace MCPP;


static const Word priority=1;
static const String name("Permissions Commands");
static const String add_identifier("addgroup");
static const String remove_identifier("removegroup");
static const String grant_identifier("grant");
static const String revoke_identifier("revoke");
static const String all_suffix("_all");
static const String identifiers []={
	grant_identifier,
	revoke_identifier,
	add_identifier,
	remove_identifier
};
static const String group_arg("-g");
static const String all_arg("-a");
static const String quiet_arg("-q");


class PermissionsCommand : public Module, public Command {


	private:
	
	
		class Arguments {
		
		
			public:
			
			
				bool All;
				bool Group;
				bool Quiet;
				String Name;
				Vector<String> Args;
		
		
		};
		
		
		static Nullable<Arguments> parse (Vector<String> args) {
		
			Nullable<Arguments> retr;
		
			if (args.Count()==0) return retr;
			
			retr.Construct();
			retr->All=false;
			retr->Group=false;
			retr->Quiet=false;
			bool first=true;
			bool complete=false;
			for (auto & s : args) {
			
				if (first) {
				
					if (s==group_arg) {
					
						if (retr->Group) break;
					
						retr->Group=true;
					
					} else if (s==all_arg) {
					
						if (retr->All) break;
					
						retr->All=true;
						
					} else if (s==quiet_arg) {
					
						if (retr->Quiet) break;
						
						retr->Quiet=true;
					
					} else {
					
						first=false;
						
						retr->Name=std::move(s);
						
						if (retr->All) complete=true;
						
					}
				
				} else if (retr->All) {
				
					break;
					
				} else {
				
					retr->Args.Add(std::move(s));
					
					complete=true;
				
				}
			
			}
			
			if (!complete) retr.Destroy();
			
			return retr;
		
		}
		
		
		enum class Type {
		
			Add,
			Remove,
			Grant,
			Revoke
		
		};
		
		
		static Type get_type (const String & str) noexcept {
		
			if (str==add_identifier) return Type::Add;
			if (str==remove_identifier) return Type::Remove;
			if (str==grant_identifier) return Type::Grant;
			return Type::Revoke;
		
		}
		
		
		static bool check_syntax (const String & identifier, const Arguments & args) {
		
			switch (get_type(identifier)) {
			
				case Type::Add:
				case Type::Remove:
					if (
						args.All ||
						(args.Args.Count()!=1)
					) return false;
					break;
					
				default:break;
			
			}
			
			return true;
		
		}
		
		
		static bool check_permissions (const String & identifier, const Arguments & args, const PermissionsHandle & handle) {
			
			//	If a user is granted the _all permission,
			//	they can always proceed
			if (handle.Check(identifier+all_suffix)) return true;
			
			//	If the user isn't granted the particular
			//	permission, they can never proceed
			if (!handle.Check(identifier)) return false;
			
			//	Otherwise check particular conditions
			switch (get_type(identifier)) {
			
				case Type::Add:
				case Type::Remove:
				
					//	User must be a member of the
					//	target group
					if (!handle.IsMember(args.Args[0])) return false;
					
					break;
					
				default:
				
					//	If we're granting/revoking all
					//	permissions, user must themselves
					//	have all permissions
					//
					//	Since we short-circuit out above
					//	if the user has a certain permission,
					//	getting here means the user does NOT
					//	have all permissions
					if (args.All) return false;
					
					//	Check each permission -- user
					//	must have them all
					for (auto & perm : args.Args) if (!handle.Check(perm)) return false;
					
					break;
			
			}
			
			return true;
		
		}
		
		
		static ChatMessage add_remove (PermissionsHandle handle, Arguments args, bool add) {
		
			ChatMessage retr;
			ChatMessage send;
			
			if (add) send << ChatStyle::BrightGreen << "Adding to ";
			else send << ChatStyle::Red << "Removing from ";
			
			send	<<	(handle.IsGroup() ? "group" : "user")
					<<	" "
					<<	ChatStyle::Bold
					<<	handle.Name()
					<<	ChatFormat::Pop
					<<	" group "
					<<	ChatStyle::Bold
					<<	args.Args[0];
					
			if (add) handle.Add(std::move(args.Args[0]));
			else handle.Remove(std::move(args.Args[0]));
			
			if (args.Quiet) retr=std::move(send);
			else Chat::Get().Send(send);
			
			return retr;
		
		}
		
		
		static ChatMessage add (PermissionsHandle handle, Arguments args) {
		
			return add_remove(
				std::move(handle),
				std::move(args),
				true
			);
		
		}
		
		
		static ChatMessage remove (PermissionsHandle handle, Arguments args) {
		
			return add_remove(
				std::move(handle),
				std::move(args),
				false
			);
		
		}
		
		
		static ChatMessage grant_revoke (PermissionsHandle handle, Arguments args, bool grant) {
		
			ChatMessage retr;
			ChatMessage send;
			
			std::sort(args.Args.begin(),args.Args.end());
			
			if (grant) send << ChatStyle::BrightGreen << "Granting";
			else send << ChatStyle::Red << "Revoking from";
			
			send	<<	" "
					<<	(handle.IsGroup() ? "group" : "user")
					<<	" "
					<<	ChatStyle::Bold
					<<	handle.Name()
					<<	ChatFormat::Pop
					<<	" ";
			
			if (args.All) {
			
				send	<<	ChatStyle::Bold
						<<	"all permissions";
						
				handle.Revoke();
			
			} else {
			
				send	<<	"permissions: "
						<<	ChatStyle::Bold;
			
				bool first=true;
				for (auto & s : args.Args) {

					if (first) first=false;
					else send << ", ";
					
					send << s;
					
					if (grant) handle.Grant(std::move(s));
					else handle.Revoke(std::move(s));
				
				}
			
			}
			
			if (args.Quiet) retr=std::move(send);
			else Chat::Get().Send(send);
			
			return retr;
		
		}
		
		
		static ChatMessage grant (PermissionsHandle handle, Arguments args) {
		
			return grant_revoke(
				std::move(handle),
				std::move(args),
				true
			);
		
		}
		
		
		static ChatMessage revoke (PermissionsHandle handle, Arguments args) {
		
			return grant_revoke(
				std::move(handle),
				std::move(args),
				false
			);
		
		}


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			auto & commands=Commands::Get();
		
			for (auto & i : identifiers) commands.Add(i,this);
		
		}
		
		
		virtual void Summary (const String & identifier, ChatMessage & message) override {
		
			switch (get_type(identifier)) {
			
				case Type::Add:
					message << "Adds groups to users and groups.";
					break;
				case Type::Remove:
					message << "Removes groups from users and groups.";
					break;
				case Type::Grant:
					message << "Grants permissions.";
					break;
				case Type::Revoke:
				default:
					message << "Revokes permissions.";
					break;
			
			}
		
		}
		
		
		virtual void Help (const String & identifier, ChatMessage & message) override {
		
			message	<<	"Syntax: "
					<<	ChatStyle::Bold
					<<	"/"
					<<	identifier
					<<	" [-g] ";
					
			auto type=get_type(identifier);
			switch (type) {
			
				case Type::Add:
				case Type::Remove:
					message	<<	"<user/group name> <group name>"
							<<	ChatFormat::Pop
							<<	Newline
							<<	((type==Type::Add) ? "Adds" : "Removes")
							<<	" the requested group "
							<<	((type==Type::Add) ? "to" : "from")
							<<	" the given user or group.  The resulting permissions of the user- or group-in-question "
								"will be "
							<<	(
									(type==Type::Add)
										?	"a union of the user or group's previous permissions, and the permissions "
											"associated with the group being added."
										:	"the permissions that user or group has as a result of permissions specifically "
											"granted them, or granted to a group which is still associated with them."
								);
							
					break;
				case Type::Grant:
				case Type::Revoke:
				default:
					message	<<	"[-a] <user/group name> <permissions>"
							<<	ChatFormat::Pop
							<<	Newline
							<<	((type==Type::Grant) ? "Grants" : "Revokes")
							<<	" requested permissions.  "
								"If permissions are added to a group which does not exist, it will be created.  "
								"The -a flag specifies that all permissions should be "
							<<	((type==Type::Grant) ? "granted" : "revoked")
							<<	" and must not be specified alongside a list of permissions.";
					break;
					
			}
			
			message	<<	"  The -g flag specifies that a group is being named, rather than a user.";
		
		}
		
		
		virtual bool Check (const CommandEvent & event) override {
		
			//	Console is always permitted
			if (event.Issuer.IsNull()) return true;
			
			auto handle=Permissions::Get().GetUser(event.Issuer);
			
			return (
				handle.Check(event.Identifier) ||
				handle.Check(event.Identifier+all_suffix)
			);
		
		}
		
		
		virtual CommandResult Execute (CommandEvent event) override {
		
			CommandResult retr;
			
			auto args=parse(std::move(event.Arguments));
			
			//	Make sure the arguments are valid
			if (args.IsNull() || !check_syntax(event.Identifier,*args)) {
			
				retr.Status=CommandStatus::SyntaxError;
				
				return retr;
			
			}
			
			//	Get the permissions handle for this
			//	user or group
			auto & permissions=Permissions::Get();
			auto handle=args->Group ? permissions.GetGroup(std::move(args->Name)) : permissions.GetUser(std::move(args->Name));
		
			//	Check permissions
			if (!(event.Issuer.IsNull() || check_permissions(
				event.Identifier,
				*args,
				permissions.GetUser(event.Issuer)
			))) {
			
				retr.Status=CommandStatus::Forbidden;
				
				return retr;
			
			}
			
			retr.Status=CommandStatus::Success;
			
			//	Invoke
			switch (get_type(event.Identifier)) {
			
				case Type::Add:
					retr.Message=add(std::move(handle),std::move(*args));
					break;
				case Type::Remove:
					retr.Message=remove(std::move(handle),std::move(*args));
					break;
				case Type::Grant:
					retr.Message=grant(std::move(handle),std::move(*args));
					break;
				case Type::Revoke:
				default:
					retr.Message=revoke(std::move(handle),std::move(*args));
					break;
				
			}
			
			return retr;
		
		}


};


INSTALL_MODULE(PermissionsCommand)

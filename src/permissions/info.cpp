#include <chat/chat.hpp>
#include <info/info.hpp>
#include <permissions/permissions.hpp>
#include <mod.hpp>
#include <algorithm>
#include <utility>


using namespace MCPP;


static const String name("Permissions Information Provider");
static const Word priority=1;
static const String identifier("permissions");
static const String help("Displays permissions tables.");
static const String permissions_banner("PERMISSIONS TABLES:");
static const String users_banner("USERS:");
static const String groups_banner("GROUPS:");
static const String permissions_label("Permissions: ");
static const String groups_label("Groups: ");
static const String all("ALL");
static const String minus(" - ");
static const String separator(", ");


class PermissionsInfoProvider : public Module, public InformationProvider {


	private:
	
	
		static void render (Vector<String> vec, ChatMessage & message) {
		
			std::sort(vec.begin(),vec.end());
			
			bool first=true;
			for (auto & str : vec) {
			
				if (first) first=false;
				else message << separator;
				
				message << str;
			
			}
		
		}
	
	
		static void render (Vector<PermissionsTableEntry> entries, ChatMessage & message) {
		
			//	Sort entries
			std::sort(
				entries.begin(),
				entries.end(),
				[] (const PermissionsTableEntry & a, const PermissionsTableEntry & b) {	return a.Name<b.Name;	}
			);
			
			//	Loop over entries
			for (auto & entry : entries) {
			
				message	<<	Newline
						<<	ChatStyle::Bold
						<<	entry.Name
						<<	Newline
						<<	permissions_label
						<<	ChatFormat::Pop;
				
				if (entry.Full) {
				
					message << all;
					
					if (entry.Difference.Count()!=0) message << minus;
				
				}
				
				render(std::move(entry.Difference),message);
				
				message	<<	Newline
						<<	ChatStyle::Bold
						<<	groups_label
						<<	ChatFormat::Pop;
						
				render(std::move(entry.Groups),message);
			
			}
		
		}


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			Information::Get().Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			message	<<	ChatStyle::Bold
					<<	permissions_banner
					<<	ChatFormat::Pop;
		
			//	Fetch permissions tables
			auto tables=Permissions::Get().GetTables();
			
			//	USERS
			
			message	<<	Newline
					<<	ChatStyle::Bold
					<<	users_banner
					<<	ChatFormat::Pop;
					
			render(std::move(tables.Users),message);
			
			//	GROUPS
			
			message	<<	Newline
					<<	ChatStyle::Bold
					<<	groups_banner
					<<	ChatFormat::Pop;
					
			render(std::move(tables.Groups),message);
		
		}


};


INSTALL_MODULE(PermissionsInfoProvider)

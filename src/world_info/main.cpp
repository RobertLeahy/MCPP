#include <common.hpp>
#include <world/world.hpp>
#include <chat/chat.hpp>
#include <op/op.hpp>
#include <algorithm>
#include <utility>


static const Word priority=2;
static const String name("World Chat Information Provider");
static const Regex world_regex(
	"^\\/info\\s+world\\s*$",
	RegexOptions().SetIgnoreCase()
);
static const String info_banner("====INFORMATION====");
static const String world_banner("WORLD:");
static const String not_an_op_error(" You must be an operator to issue that command");
static const String generated("Columns Generated");
static const String populated("Columns Populated");
static const String loaded("Columns Loaded");
static const String saved("Columns Saved");
static const String generate_time("Time Spent Generating");
static const String load_time("Time Spent Loading");
static const String populate_time("Time Spent Populating");
static const String save_time("Time Spent Saving");
static const String column_banner("COLUMNS:");
static const String ns("{0}ns");
static const String coord_template("({0},{1})");
static const String state_label("State: ");
static const String sep(", ");
static const String unloaded_str("Unloaded");
static const String generating_str("Generating");
static const String generated_str("Generated");
static const String populating_str("Populating");
static const String populated_str("Populated");
static const String clients_label("Clients: ");
static const String interested_label("Interested: ");


static inline void make_entry (const String & label, const String & body, ChatMessage & message) {

	message	<<	Newline
			<<	ChatStyle::Bold
			<<	label
			<<	": "
			<<	ChatFormat::Pop
			<<	body;

}


class WorldInfo : public Module {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	Grab previous handler for chaining
			ChatHandler prev(std::move(Chat->Chat));
			
			//	Install ourselves
			Chat->Chat=[=] (SmartPointer<Client> client, const String & message) {
			
				//	The following conditions must be
				//	met to proceed:
				//
				//	1.	The message must be "/info world"
				//		or something similar.
				//	2.	The client sending the message must
				//		be an op.
				if (world_regex.IsMatch(message)) {
				
					//	Create message
					ChatMessage message;
					message.AddRecipients(client);
				
					if (!Ops->IsOp(client->GetUsername())) {
					
						//	User is not an op, cannot issue
						//	this command, send an error
					
						message	<<	ChatStyle::Red
								<<	ChatFormat::Label
								<<	ChatFormat::LabelSeparator
								<<	not_an_op_error;
								
						Chat->Send(message);
					
						return;
					
					}
					
					//	Get information
					auto info=World->GetInfo();
					
					//	Sort columns
					std::sort(
						info.Columns.begin(),
						info.Columns.end(),
						[] (const ColumnInfo & first, const ColumnInfo & second) {
						
							return (
								//	Order by dimension first
								(first.ID.Dimension<=second.ID.Dimension) &&
								//	Then by X coordinate
								(first.ID.X<=second.ID.X) &&
								//	Last by Z coordinate
								(first.ID.Z<=second.ID.Z)
							);
						
						}
					);
					
					//	Build message
					message	<<	ChatStyle::Bold
							<<	ChatStyle::Yellow
							<<	info_banner
							<<	ChatFormat::Pop
							<<	Newline
							<<	world_banner
							<<	ChatFormat::Pop;
				
					//	Statistics
					make_entry(
						loaded,
						info.Loaded,
						message
					);
					make_entry(
						saved,
						info.Saved,
						message
					);
					make_entry(
						generated,
						info.Generated,
						message
					);
					make_entry(
						populated,
						info.Populated,
						message
					);
					make_entry(
						load_time,
						String::Format(
							ns,
							info.LoadTime
						),
						message
					);
					make_entry(
						save_time,
						String::Format(
							ns,
							info.SaveTime
						),
						message
					);
					make_entry(
						generate_time,
						String::Format(
							ns,
							info.GenerateTime
						),
						message
					);
					make_entry(
						populate_time,
						String::Format(
							ns,
							info.PopulateTime
						),
						message
					);
					
					//	Column info
					message	<<	Newline
							<<	ChatStyle::Bold
							<<	column_banner
							<<	ChatFormat::Pop;
							
					for (const auto & cinfo : info.Columns) {
					
						//	Get dimension name
						auto dname=World->GetDimensionName(
							cinfo.ID.Dimension
						);
						
						String d(
							dname.IsNull()
								?	String(cinfo.ID.Dimension)
								:	std::move(*dname)
						);
					
						message	<<	Newline
								<<	ChatStyle::Bold
								<<	d
								<<	" "
								<<	String::Format(
										coord_template,
										cinfo.ID.X,
										cinfo.ID.Z
									)
								<<	": "
								<<	ChatFormat::Pop
								<<	state_label;
								
						//	Translate ColumnState to human readable
						//	form
						String state;
						switch (cinfo.State) {
						
							case ColumnState::Unloaded:
								state=unloaded_str;
								break;
							case ColumnState::Generating:
								state=generating_str;
								break;
							case ColumnState::Generated:
								state=generated_str;
								break;
							case ColumnState::Populating:
								state=populating_str;
								break;
							case ColumnState::Populated:
							default:
								state=populated_str;
								break;
						
						}
						
						message	<<	state
								<<	sep
								<<	clients_label
								<<	String(cinfo.Clients.Count())
								<<	sep
								<<	interested_label
								<<	cinfo.Interested;
					
					}
					
					//	Send
					Chat->Send(message);
					
					//	Stop processing this chat message
					return;
				
				}
				
				//	Chain if applicable
				if (prev) prev(
					std::move(client),
					message
				);
			
			};
		
		}


};


static Nullable<WorldInfo> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

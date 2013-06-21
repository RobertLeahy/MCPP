#include <common.hpp>
#include <chat/chat.hpp>
#include <op/op.hpp>
#include <utility>
#include <algorithm>


static const Word priority=2;
static const String name("Chat Information Provider");
static const Regex info_regex(
	"^\\/info\\s+(\\w+)$",
	RegexOptions().SetIgnoreCase()
);
static const String pool_arg("pool");
static const String client_arg("clients");
static const String ops_arg("ops");
static const String info_banner("§e§l====INFORMATION====§r");
static const String info_entry("{0}: {1}");
static const String info_separator(", ");
static const String info_label("§l{0}:§r {1}");
static const String yes("Yes");
static const String no("No");
static const String not_an_op_error(
	"§c§l"	//	Bold and red
	"SERVER:"	//	Server error message
	"§r§c"	//	Not-bold and red
	" You must be an operator to issue that command"
);


//	THREAD POOL INFO
static const String pool_banner("§lMAIN SERVER THREAD POOL:§r");
static const String pool_stats("Pool Statistics");
static const String pool_running("Running");
static const String pool_queued("Queued");
static const String pool_scheduled("Scheduled");
static const String worker_template("Worker ID {0} Statistics");
static const String worker_executed("Tasks Executed");
static const String worker_failed("Tasks Failed");
static const String worker_running("Running");
static const String worker_running_template("{0}ns");


//	CLIENT INFO
static const String clients_banner("§lCONNECTED CLIENTS:§r");
static const String client_template("{0}:{1}");
static const String client_authenticated("Authenticated");
static const String client_username("Username");
static const String client_connected("Connected For");
static const String client_sent("Bytes Sent");
static const String client_received("Bytes Received");
static const String client_latency_template("{0}ms");
static const String client_latency("Latency");


//	OPS INFO
static const String ops_banner("§lSERVER OPERATORS:§r");


class Info : public Module {


	private:
	
	
		static String pool_info () {
		
			//	Get data about the thread pool
			auto info=RunningServer->Pool().GetInfo();
			
			String output(info_banner);
			output << Newline << pool_banner << Newline;
			
			//	Pool stats
			String about_pool;
			about_pool
				//	Running tasks
				<<	String::Format(
						info_entry,
						pool_running,
						info.Running
					)
				<<	info_separator
				//	Queued tasks
				<<	String::Format(
						info_entry,
						pool_queued,
						info.Queued
					)
				<<	info_separator
				//	Scheduled tasks
				<<	String::Format(
						info_entry,
						pool_scheduled,
						info.Scheduled
					);
					
			output << String::Format(
				info_label,
				pool_stats,
				std::move(about_pool)
			);
			
			//	Worker stats
			for (Word i=0;i<info.WorkerInfo.Count();++i) {
			
				auto & w=info.WorkerInfo[i];
				
				String about_worker;
				about_worker
					//	Tasks executed
					<<	String::Format(
							info_entry,
							worker_executed,
							w.TaskCount
						)
					<<	info_separator
					//	Tasks failed
					<<	String::Format(
							info_entry,
							worker_failed,
							w.Failed
						)
					<<	info_separator
					//	Time spent running
					<<	String::Format(
							info_entry,
							worker_running,
							String::Format(
								worker_running_template,
								w.Running
							)
						);
						
				output << Newline << String::Format(
					info_label,
					String::Format(
						worker_template,
						i
					),
					std::move(about_worker)
				);
			
			}
			
			return output;
		
		}
		
		
		static inline String zero_pad (String pad, Word num) {
		
			while (pad.Count()<num) pad=String("0")+pad;
			
			return pad;
		
		}
		
		
		static inline String connected_format (Word milliseconds) {
		
			String time;
			time
				//	Hours
				<<	zero_pad(
						String(milliseconds/(1000*60*60)),
						2
					)
				<<	":"
				//	Minutes
				<<	zero_pad(
						String((milliseconds%(1000*60*60))/(1000*60)),
						2
					)
				<<	":"
				//	Seconds
				<<	zero_pad(
						String((milliseconds%(1000*60))/1000),
						2
					)
				<<	"."
				//	Milliseconds
				<<	zero_pad(
						String(milliseconds%1000),
						3
					);
					
			return time;
		
		}
		
		
		static String client_info () {
		
			String output(info_banner);
			output << Newline << clients_banner;
			
			RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
			
				auto state=client->GetState();
			
				String about_client;
				about_client
					//	Authenticated?
					<<	String::Format(
							info_entry,
							client_authenticated,
							(state==ClientState::Authenticated) ? yes : no
						)
					<<	info_separator;
					
				if (state==ClientState::Authenticated) about_client
					<<	String::Format(
							info_entry,
							client_username,
							client->GetUsername()
						)
					<<	info_separator;
				
				about_client
					//	Time connected
					<<	String::Format(
							info_entry,
							client_connected,
							connected_format(
								client->Connected()
							)
						)
					<<	info_separator
					//	Latency
					<<	String::Format(
							info_entry,
							client_latency,
							String::Format(
								client_latency_template,
								static_cast<Word>(client->Ping)
							)
						)
					<<	info_separator
					//	Bytes sent
					<<	String::Format(
							info_entry,
							client_sent,
							client->Sent()
						)
					<<	info_separator
					//	Bytes received
					<<	String::Format(
							info_entry,
							client_received,
							client->Received()
						);
				
				output << Newline << String::Format(
					info_label,
					String::Format(
						client_template,
						client->IP(),
						client->Port()
					),
					std::move(about_client)
				);
			
			});
			
			return output;
		
		}
		
		
		static String ops_info () {
		
			//	Get the ops list
			auto ops=Ops->List();
			
			//	Sort it
			std::sort(ops.begin(),ops.end());
			
			//	Build it
			String output(info_banner);
			output << Newline << ops_banner;
			
			for (auto & s : ops) output << Newline << s;
			
			return output;
		
		}


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	Grab previous handler for
			//	chaining
			ChatHandler prev(std::move(Chat->Chat));
			
			//	Install ourselves
			Chat->Chat=[=] (SmartPointer<Client> client, const String & message) {
			
				//	Attempt to match against our
				//	regex
				auto match=info_regex.Match(message);
				
				//	If there's a match, attempt to
				//	handle this
				if (match.Success()) {
				
					//	The user has to be an operator
					//	to issue these commands
					if (!Ops->IsOp(client->GetUsername())) {
					
						Chat->Send(
							client,
							not_an_op_error
						);
					
						return;
					
					}
				
					//	Get the argument
					String arg=match[1].Value().ToLower();
					
					//	Thread pool information
					if (arg==pool_arg) {
					
						Chat->Send(
							client,
							ChatModule::Sanitize(
								pool_info(),
								false
							)
						);
						
						return;
					
					//	Client information
					} else if (arg==client_arg) {
					
						Chat->Send(
							client,
							ChatModule::Sanitize(
								client_info(),
								false
							)
						);
						
						return;
					
					//	Ops information
					} else if (arg==ops_arg) {
					
						Chat->Send(
							client,
							ChatModule::Sanitize(
								ops_info(),
								false
							)
						);
					
						return;
					
					}
				
				}
			
				//	Chain if applicable
				if (prev) prev(
					std::move(client),
					message
				);
			
			};
		
		}


};


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		if (mod_ptr==nullptr) try {
		
			mod_ptr=new Info();
		
		} catch (...) {	}
		
		return mod_ptr;
	
	}
	
	
	void Unload () {
	
		if (mod_ptr!=nullptr) {
		
			delete mod_ptr;
			
			mod_ptr=nullptr;
		
		}
	
	}


}

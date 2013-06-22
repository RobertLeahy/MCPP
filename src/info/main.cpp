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
static const String dp_arg("dp");
static const String info_banner("====INFORMATION====");
static const String info_separator(", ");
static const String yes("Yes");
static const String no("No");
static const String not_an_op_error(" You must be an operator to issue that command");


//	THREAD POOL INFO
static const String pool_banner("MAIN SERVER THREAD POOL:");
static const String pool_stats("Pool Information");
static const String pool_running("Running");
static const String pool_queued("Queued");
static const String pool_scheduled("Scheduled");
static const String worker_template("Worker ID {0} Information");
static const String worker_executed("Tasks Executed");
static const String worker_failed("Tasks Failed");
static const String worker_running("Running");
static const String worker_running_template("{0}ns");
static const String worker_avg("Average Task");


//	CLIENT INFO
static const String clients_banner("CONNECTED CLIENTS:");
static const String client_template("{0}:{1}");
static const String client_authenticated("Authenticated");
static const String client_username("Username");
static const String client_connected("Connected For");
static const String client_sent("Bytes Sent");
static const String client_received("Bytes Received");
static const String client_latency_template("{0}ms");
static const String client_latency("Latency");


//	OPS INFO
static const String ops_banner("SERVER OPERATORS:");


//	DATA PROVIDER INFO
static const String dp_banner("DATA PROVIDER:");


class Info : public Module {


	private:
	
	
		static ChatMessage dp_info () {
		
			ChatMessage message;
			
			//	Get data about the data provider
			auto info=RunningServer->Data().GetInfo();
			
			//	Sort the key/value pairs
			std::sort(
				info.Item<1>().begin(),
				info.Item<1>().end(),
				[] (const Tuple<String,String> & a, const Tuple<String,String> & b) {
			
					return a.Item<0>()<b.Item<0>();
					
				}
			);
			
			message	<<	ChatStyle::Bold
					<<	ChatColour::Yellow
					<<	info_banner
					<<	ChatFormat::PopColour
					<<	Newline
					<<	dp_banner
					<<	Newline
					<<	info.Item<0>()
					<<	":"
					<<	ChatFormat::PopStyle;
					
			for (const auto & t : info.Item<1>()) {
			
				message	<<	Newline
						<<	ChatStyle::Bold
						<<	t.Item<0>()
						<<	": "
						<<	ChatFormat::PopStyle
						<<	t.Item<1>();
			
			}
			
			return message;
		
		}
	
	
		static ChatMessage pool_info () {
		
			//	Get data about the thread pool
			auto info=RunningServer->Pool().GetInfo();
			
			ChatMessage message;
			message	<<	ChatStyle::Bold
					<<	ChatColour::Yellow
					<<	info_banner
					<<	ChatFormat::PopColour
					<<	Newline
					//	THREAD POOL STATISTICS
					<<	pool_stats
					<<	": "
					<<	ChatFormat::PopStyle
					//	Running tasks
					<<	pool_running
					<<	": "
					<<	info.Running
					<<	info_separator
					//	Queued tasks
					<<	pool_queued
					<<	": "
					<<	info.Queued
					<<	info_separator
					//	Scheduled tasks
					<<	pool_scheduled
					<<	": "
					<<	info.Scheduled;
					
			//	Worker stats
			for (Word i=0;i<info.WorkerInfo.Count();++i) {
			
				auto & w=info.WorkerInfo[i];
				
				message	<<	Newline
						<<	ChatStyle::Bold
						<<	String::Format(
								worker_template,
								i
							)
						<<	": "
						<<	ChatFormat::PopStyle
						//	Tasks executed
						<<	worker_executed
						<<	": "
						<<	w.TaskCount
						<<	info_separator
						//	Tasks failed
						<<	worker_failed
						<<	": "
						<<	w.Failed
						<<	info_separator
						//	Time spent running
						<<	worker_running
						<<	": "
						<<	String::Format(
								worker_running_template,
								w.Running
							)
						<<	info_separator
						//	Average time per task
						<<	worker_avg
						<<	": "
						<<	String::Format(
								worker_running_template,
								w.Running/w.TaskCount
							);
			
			}
			
			return message;
		
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
		
		
		static ChatMessage client_info () {
		
			ChatMessage message;
			message	<<	ChatStyle::Bold
					<<	ChatColour::Yellow
					<<	info_banner
					<<	ChatFormat::PopColour
					<<	Newline
					<<	clients_banner
					<<	ChatFormat::PopStyle;
					
			RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
			
				auto state=client->GetState();
				
				message	<<	Newline
						<<	ChatStyle::Bold
						<<	String::Format(
								client_template,
								client->IP(),
								client->Port()
							)
						<<	": "
						<<	ChatFormat::PopStyle
						//	Authenticated?
						<<	client_authenticated
						<<	": "
						<<	((state==ClientState::Authenticated) ? yes : no)
						<<	info_separator
						//	Time connected
						<<	client_connected
						<<	": "
						<<	connected_format(client->Connected())
						<<	info_separator
						//	Latency
						<<	client_latency
						<<	": "
						<<	String::Format(
								client_latency_template,
								static_cast<Word>(client->Ping)
							)
						<<	info_separator
						//	Bytes sent
						<<	client_sent
						<<	": "
						<<	client->Sent()
						<<	info_separator
						//	Bytes received
						<<	client_received
						<<	": "
						<<	client->Received();
			
			});
			
			return message;
		
		}
		
		
		static ChatMessage ops_info () {
		
			//	Get the ops list
			auto ops=Ops->List();
			
			//	Sort it
			std::sort(ops.begin(),ops.end());
			
			//	Build it
			ChatMessage message;
			message	<<	ChatStyle::Bold
					<<	ChatColour::Yellow
					<<	info_banner
					<<	ChatFormat::PopColour
					<<	Newline
					<<	ops_banner
					<<	ChatFormat::PopStyle;
					
			for (auto & s : ops) message << Newline << s;
			
			return message;
		
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
				
					String username(client->GetUsername());
				
					//	The user has to be an operator
					//	to issue these commands
					if (!Ops->IsOp(username)) {
					
						ChatMessage message;
						message.To.Add(username);
						message	<<	ChatColour::Red
								<<	ChatFormat::Label
								<<	ChatFormat::LabelSeparator
								<<	not_an_op_error;
								
						Chat->Send(message);
					
						return;
					
					}
				
					//	Get the argument
					String arg=match[1].Value().ToLower();
					
					Nullable<ChatMessage> message;
					
					//	Thread pool information
					if (arg==pool_arg) {
					
						message=pool_info();
					
					//	Client information
					} else if (arg==client_arg) {
					
						message=client_info();
					
					//	Ops information
					} else if (arg==ops_arg) {
					
						message=ops_info();
					
					} else if (arg==dp_arg) {
					
						message=dp_info();
					
					}
					
					if (!message.IsNull()) {
					
						message->To.Add(username);
						
						Chat->Send(*message);
						
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

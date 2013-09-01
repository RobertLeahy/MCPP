#include <info/info.hpp>


static const String name("Thread Pool Information");
static const Word priority=1;
static const String identifier("pool");
static const String help("Display information about the server's pool of worker threads.");


static const String pool_template("Running: {0}, Queued: {1}, Scheduled: {2}");
static const String worker_template("Executed: {0}, Failed: {1}, Running: {2}ns, Average Per: {3}ns");
static const String pool_banner("MAIN SERVER THREAD POOL:");
static const String worker_label_template("Worker ID {0} Information: ");
static const String pool_stats("Pool Information: ");


class PoolInfo : public Module, public InformationProvider {


	public:
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			Information->Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			//	Get data about the thread pool
			auto info=RunningServer->Pool().GetInfo();
			
			message	<<	ChatStyle::Bold
					<<	pool_banner
					<<	ChatFormat::Pop
					<<	Newline
					<<	pool_stats
					<<	ChatFormat::Pop
					<<	String::Format(
							pool_template,
							info.Running,
							info.Queued,
							info.Scheduled
						);
					
			//	Stats for each worker
			for (Word i=0;i<info.WorkerInfo.Count();++i) {
			
				auto & w=info.WorkerInfo[i];
				
				message	<<	Newline
						<<	ChatStyle::Bold
						<<	String::Format(
								worker_label_template,
								i
							)
						<<	ChatFormat::Pop
						<<	String::Format(
								worker_template,
								w.TaskCount,
								w.Failed,
								w.Running,
								//	Guard against divide by zero
								(w.TaskCount==0) ? 0 : (w.Running/w.TaskCount)
							);
			
			}
		
		}


};


static Nullable<PoolInfo> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

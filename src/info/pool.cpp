#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <info/info.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <thread_pool.hpp>


using namespace MCPP;


static const String name("Thread Pool Information");
static const Word priority=1;
static const String identifier("pool");
static const String help("Displays information about the server's pool of worker threads.");


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
		
			Information::Get().Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			//	Get data about the thread pool
			auto info=Server::Get().Pool().GetInfo();
			
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


INSTALL_MODULE(PoolInfo)

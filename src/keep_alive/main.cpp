#include <common.hpp>
#include <utility>


static const String name("Keep alive support and connection timeouts");
static const String timeout("Timeout of {0}ms exceeded");
static const Word priority=1;
static const Word timeout_milliseconds=60000;
static const Word keep_alive_milliseconds=5000;


class KeepAlive : public Module {


	private:
	
	
		Mutex lock;
		CondVar wait;
		Thread worker;
		bool stop;
		Random<Int32> generator;
		Barrier barrier;
		
		
		static void thread_func (void * ptr) noexcept {
		
			try {
			
				reinterpret_cast<KeepAlive *>(ptr)->thread_func_impl();
			
			} catch (...) {
			
				RunningServer->Panic();
			
			}
		
		}
		
		
		void thread_func_impl () {
		
			barrier.Enter();
			
			try {
		
				//	Loop until stopped
				for (;;) {
				
					if (lock.Execute([&] () {
					
						wait.Sleep(
							lock,
							(timeout_milliseconds<keep_alive_milliseconds)
								?	timeout_milliseconds
								:	keep_alive_milliseconds
						);
						
						return stop;
					
					})) break;
					
					RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
					
						//	Check idle time
						Word inactive=client->Inactive();
						if (inactive>=timeout_milliseconds) {

							client->Disconnect(
								String::Format(
									timeout,
									inactive
								)
							);
							
							//	Next client
							return;
						
						}
					
						//	Send keep alive if applicable
						if (client->GetState()==ClientState::Authenticated) {
						
							Packet keep_alive;
							keep_alive.SetType<PacketTypeMap<0x00>>();
							keep_alive.Retrieve<Int32>(0)=generator();
							
							client->Send(std::move(keep_alive));
						
						}
					
					});
				
				}
				
			} catch (...) {
			
				barrier.Enter();
				
				throw;
			
			}
			
			barrier.Enter();
			
		}


	public:
	
	
		KeepAlive () : stop(false), barrier(2) {
			
			//	Start worker, it'll wait on barrier
			worker=Thread(
				thread_func,
				this
			);
		
		}
	
	
		virtual ~KeepAlive () noexcept override {
		
			lock.Execute([&] () {
			
				stop=true;
				
				wait.WakeAll();
			
			});
			
			//	Wait
			barrier.Enter();
			
			worker.Join();
		
		}
	
	
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	Release the worker
			barrier.Enter();
			
			//	Install a dummy handler for
			//	0x00 so even if the router
			//	is set to disconnect the client
			//	on packets with no handler,
			//	we can still gracefully handle
			//	incoming keep alives (which
			//	we just ignore).
			
			//	Grab old handler (if any)
			PacketHandler prev(std::move(RunningServer->Router[0x00]));
			
			RunningServer->Router[0x00]=[=] (SmartPointer<Client> conn, Packet packet) {
			
				//	Chain through if applicable
				if (prev) prev(
					std::move(conn),
					std::move(packet)
				);
			
			};
		
		}


};


static Module * mod_ptr=nullptr;


extern "C" {


	Module * Load () {
	
		if (mod_ptr==nullptr) try {
		
			mod_ptr=new KeepAlive();
		
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

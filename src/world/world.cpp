#include <world/world.hpp>
#include <utility>


namespace MCPP {


	static const String name("Minecraft World Loading/Generation/Population/Saving");
	static const Word priority=1;
	static const String mods_dir("world_mods");
	static const String log_prepend("World: ");
	static const String unload_interval_setting("world_unload_interval");
	static const Word unload_interval_default=60000;	//	Once a minute
	
	
	static Nullable<ModuleLoader> mods;
	
	
	Nullable<WorldContainer> World;
	
	
	Word WorldContainer::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	const String & WorldContainer::Name () const noexcept {
	
		return name;
	
	}
	
	
	WorldContainer::WorldContainer () {
	
		generated=0;
		loaded=0;
		populated=0;
		generate_time=0;
		load_time=0;
		populate_time=0;
		
		//	Fire up the mod loader
		mods.Construct(
			Path::Combine(
				Path::GetPath(
					File::GetCurrentExecutableFileName()
				),
				mods_dir
			),
			[] (const String & message, Service::LogType type) {
			
				String log(log_prepend);
				log << message;
				
				RunningServer->WriteLog(
					log,
					type
				);
			
			}
		);
	
	}
	
	
	static inline void client_load (SmartPointer<Client> client, ColumnID id, const Column * column) {
	
		typedef PacketTypeMap<0x33> pt;
		
		Packet packet;
		packet.SetType<pt>();
		
		packet.Retrieve<pt,0>()=id.X;
		packet.Retrieve<pt,1>()=id.Z;
		packet.Retrieve<pt,2>()=column;
		
		client->Send(packet);
	
	}
	
	
	static inline void client_unload (SmartPointer<Client> client, ColumnID id) {
	
		typedef PacketTypeMap<0x33> pt;
		
		Packet packet;
		packet.SetType<pt>();
		
		packet.Retrieve<pt,0>()=id.X;
		packet.Retrieve<pt,1>()=id.Z;
		packet.Retrieve<pt,2>()=nullptr;
		
		client->Send(packet);
	
	}
	
	
	static inline Byte set_nibble (Byte b, Byte s, bool high) noexcept {
	
		return high ? ((b&15)|(s<<4)) : ((b&240)|(s&15));
	
	}
	
	
	static inline Byte get_nibble (Byte b, bool high) noexcept {
	
		return high ? (b>>4) : (b&15);
	
	}
	
	
	static inline void send_block_change (BlockID id, Block block, SmartPointer<ColumnContainer> column) {
	
		try {
		
			typedef PacketTypeMap<0x35> pt;
			
			Packet packet;
			packet.SetType<pt>();
			
			packet.Retrieve<pt,0>()=id.X;
			packet.Retrieve<pt,1>()=static_cast<Byte>(id.Y);
			packet.Retrieve<pt,2>()=id.Z;
			packet.Retrieve<pt,3>()=block.Type;
			packet.Retrieve<pt,4>()=block.Metadata;
		
			for (auto & client : column->Clients) {
			
				try {
				
					const_cast<Client *>(
						static_cast<const Client *>(
							client
						)
					)->Send(packet);
				
				} catch (...) {
				
					//	TODO: Something useful
				
				}
			
			}
		
		} catch (...) {
		
			//	TODO: Something useful
		
		}
	
	}
	
	
	static inline void set_block_impl (Word offset, Word nibble_offset, bool high, Block block, SmartPointer<ColumnContainer> column) {
	
		//	Calculate type and add
		Byte type=static_cast<Byte>(block.Type&255);
		Byte add=static_cast<Byte>(block.Type>>8);
		
		//	Set block type
		column->Storage.Data[offset]=type;
		//	Set block metadata
		Word meta_offset=(16*16*256)+nibble_offset;
		column->Storage.Data[meta_offset]=set_nibble(
			column->Storage.Data[meta_offset],
			block.Metadata,
			high
		);
		//	Set light
		Word light_offset=(16*24*256)+nibble_offset;
		column->Storage.Data[light_offset]=set_nibble(
			column->Storage.Data[light_offset],
			block.Light,
			high
		);
		//	Set skylight
		Word skylight_offset=(16*16*256*2)+nibble_offset;
		column->Storage.Data[skylight_offset]=set_nibble(
			column->Storage.Data[skylight_offset],
			block.Skylight,
			high
		);
		//	Set add
		Word add_offset=(16*20*256*2)+nibble_offset;
		column->Storage.Data[add_offset]=set_nibble(
			column->Storage.Data[add_offset],
			add,
			high
		);
	
	}
	
	
	inline bool WorldContainer::set_block (BlockID id, Block block, SmartPointer<ColumnContainer> column) {
	
		Word offset=id.Offset();
		
		Word nibble_offset=offset/2;
		
		bool high=(offset%2)==0;
		
		column->Lock.Acquire();
		
		try {
		
			//	Fire event
			if (
				//	Only fire event if column
				//	is populated
				column->Storage.Populated &&
				!OnSet(id,block)
			) {
			
				column->Lock.Release();
				
				return false;
			
			}
		
		} catch (...) {
		
			column->Lock.Release();
			
			return false;
		
		}
		
		column->Dirty=true;
		
		try {
		
			set_block_impl(
				offset,
				nibble_offset,
				high,
				block,
				column
			);
			
			//	Send block change to clients
			//	who have the column in question
			if (column->Storage.Populated) {
			
				send_block_change(
					id,
					block,
					column
				);
			
			}
			
		} catch (...) {
		
			column->Lock.Release();
			
			throw;
		
		}
		
		column->Lock.Release();
		
		return true;
	
	}
	
	
	static inline Block get_block_impl (Word offset, Word nibble_offset, bool high, SmartPointer<ColumnContainer> column) noexcept {
	
		Block block;
		
		Byte type=column->Storage.Data[offset];
		
		block.Metadata=get_nibble(
			column->Storage.Data[(16*16*256)+nibble_offset],
			high
		);
		
		block.Light=get_nibble(
			column->Storage.Data[(16*24*256)+nibble_offset],
			high
		);
		
		block.Skylight=get_nibble(
			column->Storage.Data[(16*16*256*2)+nibble_offset],
			high
		);
		
		Byte add=get_nibble(
			column->Storage.Data[(16*20*256*2)+nibble_offset],
			high
		);
		
		block.Type=(static_cast<UInt16>(add)<<8)|static_cast<UInt16>(type);
		
		return block;
	
	}
	
	
	static inline Block get_block (BlockID id, SmartPointer<ColumnContainer> column) noexcept {
	
		Word offset=id.Offset();
		
		Word nibble_offset=offset/2;
		
		bool high=(offset%2)==0;
		
		column->Lock.Acquire();
		
		auto block=get_block_impl(
			offset,
			nibble_offset,
			high,
			column
		);
		
		column->Lock.Release();
		
		return block;
	
	}
	
	
	bool WorldContainer::SetBlock (BlockID id, Block block, bool release) {
	
		//	Retrieve/load/generate the column
		//	that this block resides in
		auto column=load(
			id.ContainedBy(),
			false	//	Column doesn't have to be populated
		);
		
		bool success=false;
		try {
		
			success=set_block(id,block,column);
			
		} catch (...) {	}
		
		//	Calling load automatically acquires
		//	interest, so release that
		--column->Interested;
		
		//	Release interest if the caller requests
		//	it
		if (release) --column->Interested;
		
		return success;
	
	}
	
	
	Block WorldContainer::GetBlock (BlockID id, bool acquire) {
	
		//	Retrieve/load/generate the column
		//	that this block resides in
		auto column=load(
			id.ContainedBy(),
			false	//	Column doesn't have to be populated
		);
		
		auto block=get_block(id,column);
		
		//	Calling load automatically acquires
		//	interest, so release that, unless
		//	the caller wants to acquire interest
		if (!acquire) --column->Interested;
		
		return block;
	
	}
	
	
	SmartPointer<ColumnContainer> WorldContainer::load (ColumnID id, bool populate) {
	
		SmartPointer<ColumnContainer> column;
		bool added=false;
		
		//	Check to see if the column exists
		
		world_lock.Acquire();
		
		auto iter=world.find(id);
		
		if (iter==world.end()) {
		
			column=SmartPointer<ColumnContainer>::Make();
			
			world.emplace(id,column);
			
			column->Processing=true;
			
			added=true;
		
		} else {
		
			column=iter->second;
			
			++column->Interested;
		
		}
		
		world_lock.Release();
		
		//	Decide whether to generate, wait,
		//	or continue
		column->Lock.Acquire();
		
		bool generate=false;
		
		//	If the column is being processed,
		//	we need to wait until it's generated
		//	before we proceed
		//
		//	However, we need to be aware that
		//	if we added the column it is always
		//	marked as processing.
		if (column->Processing && !added) {
		
			while (!column->Generated) column->Wait.Sleep(column->Lock);
			
		//	Otherwise, generate the column
		} else {
		
			column->Processing=true;
			
			generate=true;
		
		}
		
		column->Lock.Release();
		
		//	Generate/load if necessary
		if (generate) {
		
			//	Check the backing store to see
			//	if the column is present
			
			Timer timer=Timer::CreateAndStart();
			
			bool loaded=RunningServer->Data().LoadColumn(
				id.X,
				id.Z,
				id.Dimension,
				&column->Storage
			);
			
			timer.Stop();
			
			if (loaded) {
			
				load_time+=timer.ElapsedNanoseconds();
				++loaded;
			
			} else {
		
				Timer timer=Timer::CreateAndStart();
			
				//	Get the provider
				const Provider * provider=nullptr;
				
				//	Attempt to look up a provider
				//	for this dimension/world type
				auto dim_iter=providers.find(id.Dimension);
				if (dim_iter!=providers.end()) {
				
					auto type_iter=dim_iter->second.find(world_type);
					if (type_iter!=dim_iter->second.end()) {
					
						provider=&type_iter->second;
					
					}
				
				}
				
				//	If no provider specifically for this
				//	dimension and world type was found,
				//	attempt to fall back onto the default
				if (provider==nullptr) {
				
					auto default_iter=default_providers.find(id.Dimension);
					if (default_iter!=default_providers.end()) {
					
						provider=&default_iter->second;
					
					}
					
				}
				
				//	If there's no provider, ERROR
				if (provider==nullptr) {
				
					//	TODO: Something sensible
				
				}
				
				//	Generate
				(*provider)(id,column->Storage);
				
				generate_time+=timer.ElapsedNanoseconds();
				++generated;
				
			}
		
			//	We're finished generating,
			//	inform waiting threads
			column->Lock.Acquire();
			
			column->Dirty=true;
			column->Generated=true;
			
			//	If there are waiting clients,
			//	unconditionally populate the
			//	column
			if (column->Clients.size()!=0) populate=true;
			else column->Processing=false;
			
			column->Wait.WakeAll();
			column->Lock.Release();
		
		}
		
		//	Proceed only if population
		//	has been requested
		if (populate) {
		
			//	Check to see if we need to populate
			column->Lock.Acquire();
			
			//	If another thread is processing,
			//	wait for it to finish
			while (column->Processing) column->Wait.Sleep(column->Lock);
			
			//	If the column is not populated
			//	populate it.
			if (!column->Storage.Populated) column->Processing=true;
			else populate=false;
			
			column->Lock.Release();
			
			//	If the column needs to be populated,
			//	populate it
			if (populate) {
			
				Timer timer=Timer::CreateAndStart();
			
				//	Populate
				for (const auto & t : populators) {
				
					try {
					
						t.Item<1>()(id);
					
					} catch (...) {	}
				
				}
				
				populate_time+=timer.ElapsedNanoseconds();
				++populated;
			
				//	We're finished populating, lock
				//	the column to finish up
				column->Lock.Acquire();
				
				column->Processing=false;
				column->Storage.Populated=true;
				column->Dirty=true;
				
				//	Send the column to all waiting clients
				//	(if any)
				for (auto & client : column->Clients) {
				
					try {
				
						client_load(client,id,&column->Storage);
				
					} catch (...) {
					
						//	TODO: Something sensible
					
					}
				
				}
				
				//	Wake all waiting threads
				column->Wait.WakeAll();
				column->Lock.Release();
			
			}
			
		}
		
		return column;
	
	}
	
	
	void WorldContainer::Add (ColumnID id, SmartPointer<Client> client) {
	
		bool populate=false;
	
		world_lock.Acquire();
		
		try {
		
			//	Add to the map of clients and
			//	chunks
			auto iter=client_map.find(client);
			if (iter==client_map.end()) iter=client_map.emplace(
				client,
				std::unordered_set<ColumnID>()
			).first;
			if (iter->second.find(id)!=iter->second.end()) {
			
				world_lock.Release();
				
				return;
			
			}
			iter->second.insert(id);
			
			//	Does the column the client needs
			//	exist?
			auto world_iter=world.find(id);
			
			if (world_iter==world.end()) {
		
				//	NO
			
				//	Create the column, add the client to it
				//	and flag it to be populated
				
				world_iter=world.emplace(
					id,
					SmartPointer<ColumnContainer>::Make()
				).first;
				
				world_iter->second->Clients.insert(client);
				
				populate=true;
			
			} else {
			
				//	YES
			
				world_iter->second->Lock.Acquire();
				
				try {
				
					//	Is the column already populated?
					if (world_iter->second->Storage.Populated) {
					
						//	YES
					
						//	Send it to the client
						client_load(
							client,
							id,
							&world_iter->second->Storage
						);
					
					} else {
					
						//	NO
					
						//	Will it be?
						//
						//	If not, flag it for population
						if (!world_iter->second->Processing) populate=true;
					
					}
					
					//	Add the client to the column
					world_iter->second->Clients.insert(client);
				
				} catch (...) {
				
					world_iter->second->Lock.Release();
					
					throw;
				
				}
				
				world_iter->second->Lock.Release();
			
			}

		} catch (...) {
		
			world_lock.Release();
			
			throw;
		
		}
		
		world_lock.Release();
		
		//	Populate/generate/load the column
		//	if necessary
		if (populate) --load(id,true)->Interested;
	
	}
	
	
	void WorldContainer::Remove (ColumnID id, SmartPointer<Client> client) {
	
		world_lock.Acquire();
		
		//	Does the client even have the column?
		auto iter=client_map.find(client);
		if (iter==client_map.end()) {
		
			world_lock.Release();
			
			return;
		
		}
		auto inner_iter=iter->second.find(id);
		if (inner_iter==iter->second.end()) {
		
			world_lock.Release();
			
			return;
		
		}
		
		//	YES
		
		//	Remove references to the column
		iter->second.erase(inner_iter);
		client_map.erase(iter);
		
		//	Find the column that this client
		//	has
		auto world_iter=world.find(id);
		if (world_iter==world.end()) {
		
			world_lock.Release();
			
			return;
		
		}
		
		world_lock.Release();
		
		world_iter->second->Lock.Acquire();
		
		//	Erase this client from the list
		//	of clients who have or want
		//	this column
		world_iter->second->Clients.erase(client);
		
		//	If this column is populated, we
		//	must unload it from this client
		bool unload=world_iter->second->Storage.Populated;
		
		world_iter->second->Lock.Release();
		
		//	If we must unload, do so
		if (unload) client_unload(
			std::move(client),
			id
		);
	
	}
	
	
	void WorldContainer::Add (SByte dimension, String world_type, Provider provider, bool is_default) {
	
		//	Attempt to retrieve the map of
		//	world types to providers for this
		//	dimension
		auto iter=providers.find(dimension);
		
		//	If that map does not exist, create
		//	it
		if (iter==providers.end()) {
		
			iter=providers.emplace(
				dimension,
				std::unordered_map<String,Provider>()
			).first;
		
		}
		
		//	Does a provider with this world
		//	type already exist?
		auto type_iter=iter->second.find(world_type);
		
		if (type_iter==iter->second.end()) {
		
			//	NO
			
			//	Insert it
			iter->second.emplace(
				std::move(world_type),
				provider
			);
		
		} else {
		
			//	YES
			
			//	Overwrite it
			type_iter->second=provider;
		
		}
		
		//	Is this a default provider?
		if (is_default) {
		
			//	YES
			
			//	Is there already a default
			//	provider for this dimension?
			auto iter=default_providers.find(dimension);
			
			if (iter==default_providers.end()) {
			
				//	NO
				
				//	Insert it
				default_providers.emplace(
					dimension,
					std::move(provider)
				);
			
			} else {
			
				//	YES
				
				//	Overwrite it
				iter->second=std::move(provider);
			
			}
		
		}
	
	}
	
	
	void WorldContainer::Add (Word priority, Populator populator) {
	
		//	Find the insertion point
		for (Word i=0;i<populators.Count();++i) {
		
			//	If this is the position,
			//	insert
			if (populators[i].Item<0>()>priority) {
			
				populators.Emplace(
					i,
					priority,
					std::move(populator)
				);
			
				return;
			
			}
		
		}
		
		//	We failed to find the insertion point,
		//	so emplace at the back
		populators.EmplaceBack(
			priority,
			std::move(populator)
		);
	
	}
	
	
	ColumnState WorldContainer::GetColumnState (ColumnID id, bool acquire) noexcept {
	
		world_lock.Acquire();
		
		auto iter=world.find(id);
		
		//	Unloaded
		if (iter==world.end()) {
		
			world_lock.Release();
			
			return ColumnState::Unloaded;
		
		}
		
		auto column=iter->second;
		
		//	Prevent the column from being
		//	unloaded while we examine it
		++column->Interested;
		
		world_lock.Release();
		
		column->Lock.Acquire();
		
		ColumnState result=(
			column->Processing
				?	(
						column->Generated
							?	ColumnState::Populating
							:	ColumnState::Generating
					)
				:	(
						!column->Generated
							?	ColumnState::Unloaded
							:	(
									!column->Storage.Populated
										?	ColumnState::Generated
										:	ColumnState::Populated
								)
					)
		);
		
		column->Lock.Release();
		
		//	Release interest if necessary
		if (
			!acquire ||
			(result==ColumnState::Unloaded)
		) --column->Interested;
		
		return result;
	
	}
	
	
	void WorldContainer::EndInterest (ColumnID id) noexcept {
	
		world_lock.Acquire();
		
		auto iter=world.find(id);
		
		if (iter!=world.end()) --iter->second->Interested;
		
		world_lock.Release();
	
	}
	
	
	void WorldContainer::Install () {
	
		//	Get mods
		mods->Load();
		
		//	Get the interval at which
		//	we should check columns for
		//	unloading and save them
		auto interval=RunningServer->Data().GetSetting(unload_interval_setting);
		if (
			interval.IsNull() ||
			!interval->ToInteger(&unload_interval)
		) unload_interval=unload_interval_default;
		
		save_callback=[=] () {
		
			//	Acquire world lock
			world_lock.Acquire();
			
			try {
			
				//	A list of columns we have
				//	to unload
				Vector<ColumnID> unload;
				
				//	Loop over all loaded columns
				for (auto & pair : world) {
				
					//	Save if appropriate
					pair.second->Lock.Acquire();
					
					//	Immediately move on if the
					//	column is being processed
					if (pair.second->Processing) {
					
						pair.second->Lock.Release();
						
						continue;
					
					}
					
					if (pair.second->Dirty) {
					
						pair.second->Lock.Release();
					
						RunningServer->Data().SaveColumn(
							pair.first.X,
							pair.first.Z,
							pair.first.Dimension,
							pair.second->Storage,
							[=] (Int32, Int32, SByte) mutable {
							
								pair.second->Lock.Acquire();
								
								return true;
							
							},
							[=] (Int32, Int32, SByte, bool success) mutable {
							
								//	Saved -- no longer dirty if save
								//	succeeded
								pair.second->Dirty=!success;
								pair.second->Lock.Release();
								
								//	Check to see if we can unload
								if (success) {
								
									//	Acquire locks
									world_lock.Acquire();
									pair.second->Lock.Acquire();
									
									//	We can unload if:
									//
									//	-	No clients have the column
									//	-	There is no interest in the
									//		column
									//	-	The column is not dirty
									if (
										(pair.second->Clients.size()==0) &&
										(pair.second->Interested==0) &&
										!pair.second->Dirty
									) world.erase(pair.first);
									
									pair.second->Lock.Release();
									world_lock.Release();
								
								}
							
							}
						);
						
					} else {
					
						pair.second->Lock.Release();
						
						//	Attempt to unload
						
						//	Acquire locks
						world_lock.Acquire();
						pair.second->Lock.Acquire();
						
						//	We can unload if:
						//
						//	-	No clients have the column
						//	-	There is no interest in the
						//		column
						//	-	The column is not dirty
						if (
							(pair.second->Clients.size()==0) &&
							(pair.second->Interested==0) &&
							!pair.second->Dirty
						) world.erase(pair.first);
						
						pair.second->Lock.Release();
						world_lock.Release();
					
					}
				
				}
			
			} catch (...) {
			
				world_lock.Release();
				
				throw;
			
			}
			
			world_lock.Release();
			
			//	Continue the loop
			RunningServer->Pool().Enqueue(
				unload_interval,
				save_callback
			);
		
		};
		
		//	Start the loop
		RunningServer->Pool().Enqueue(
			unload_interval,
			save_callback
		);
		
		//	Install mods
		mods->Install();
	
	}


}


extern "C" {

	
	Module * Load () {
	
		try {
		
			if (World.IsNull()) {
			
				//	Make sure the mod loader
				//	is uninitialized
				mods.Destroy();
				
				//	Create world container
				World.Construct();
			
			}
			
			return &(*World);
		
		} catch (...) {	}
		
		return nullptr;
	
	}
	
	
	void Unload () {
	
		World.Destroy();
		
		if (!mods.IsNull()) mods->Unload();
	
	}
	
	
	void Cleanup () {
	
		mods.Destroy();
	
	}
	

}

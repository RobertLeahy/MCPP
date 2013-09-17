#include <world/world.hpp>
#include <limits>
#include <cstring>


namespace MCPP {


	constexpr Word ColumnContainer::Size;


	ColumnID ColumnContainer::ID () const noexcept {
	
		return id;
	
	}


	ColumnContainer::ColumnContainer (ColumnID id) noexcept : Populated(false), id(id), target(ColumnState::Loading), sent(false), dirty(false) {
	
		curr=static_cast<Word>(ColumnState::Loading);
		interest=0;
	
	}


	bool ColumnContainer::SetState (ColumnState target, bool dirty, ThreadPool & pool) noexcept {
	
		lock.Acquire();
		
		//	Set dirty flag appropriately
		if (dirty) this->dirty=true;
		
		//	Set populated flag if appropriate
		if (target==ColumnState::Populated) Populated=true;
		
		//	Set state to target
		curr=static_cast<Word>(target);
		
		//	Find all pending asynchronous
		//	tasks and enqueue them
		for (Word i=0;i<pending.Count();) {
		
			if (static_cast<Word>(pending[i].Item<0>())<=static_cast<Word>(target)) {
			
				//	Enqueue callback
				
				try {
				
					pool.Enqueue(
						std::move(
							pending[i].Item<1>()
						)
					);
				
				//	If an exception is thrown, we
				//	MUST wake up the blocked threads,
				//	and then we can throw
				} catch (...) {
				
					wait.WakeAll();
					
					lock.Release();
					
					throw;
				
				}
				
				//	Delete callback
				pending.Delete(i);
				
				//	Avoid increment
				continue;
			
			}
			
			++i;
		
		}
		
		//	Determine return value
		//	(i.e. whether processing is
		//	finished or not)
		bool retr=target==this->target;
		
		//	Wake all threads waiting on the
		//	condvar
		wait.WakeAll();
		
		lock.Release();
		
		return retr;
	
	}


	bool ColumnContainer::WaitUntil (ColumnState target) noexcept {
	
		//	Do not acquire lock, atomically check
		//	current state
		Word t=static_cast<Word>(target);
		
		Word c=static_cast<Word>(curr);
		
		//	If the current state is as much or more
		//	than what we require, return at once
		if (t<=c) return true;
		
		lock.Acquire();
		
		//	Reload in case state has
		//	changed
		c=static_cast<Word>(curr);
		//	Recheck in case state has
		//	changed
		if (t<=c) {
		
			lock.Release();
			
			return true;
		
		}
		
		//	Load target state
		Word tt=static_cast<Word>(this->target);
		
		//	Check to see if this column is
		//	being processed
		bool retr;
		if (c==tt) {
		
			//	The current and target states are
			//	identical, which means that no
			//	processing is currently occurring
			//
			//	Therefore we must inform the caller
			//	that they must do the processing
			
			//	Set the target
			this->target=target;
			
			retr=false;
		
		} else {
		
			//	The column is currently being processed
			
			//	Is the level of processing we desire
			//	more than what is currently planned?
			//
			//	If so update the target
			if (t>tt) this->target=target;
			
			retr=true;
			
			//	Wait for processing to reach the
			//	appropriate point
			while (static_cast<Word>(curr)<t) wait.Sleep(lock);
		
		}
		
		lock.Release();
		
		return retr;
	
	}
	
	
	bool ColumnContainer::Check (ColumnState target) noexcept {
	
		//	Do not lock, atomically check
		//	current state
		Word t=static_cast<Word>(target);
		
		Word c=static_cast<Word>(curr);
		
		//	If the current state is as much
		//	or more than what we require
		//	return true at once
		if (t<=c) return true;
		
		lock.Acquire();
		
		//	Reload in case the state changed
		c=static_cast<Word>(curr);
		//	Recheck in case the state has
		//	changed
		if (t<=c) {
		
			lock.Release();
			
			return true;
		
		}
		
		//	Update target state
		this->target=target;
		
		//	Inform caller they must
		//	process
		return false;
	
	}
	
	
	bool ColumnContainer::InvokeWhen (ColumnState target, std::function<void ()> callback) {
	
		//	Do not lock, atomically check
		//	current state
		Word t=static_cast<Word>(target);
		
		Word c=static_cast<Word>(curr);
		
		//	If the current state is as much
		//	or more than what we require
		//	execute the callback at once
		if (t<=c) {
		
			callback();
			
			return true;
			
		}
		
		lock.Acquire();
		
		//	Reload in case the state
		//	changed
		c=static_cast<Word>(curr);
		//	Recheck in case the state
		//	has changed
		if (t<=c) {
		
			lock.Release();
			
			callback();
			
			return true;
		
		}
		
		//	We can't dispatch the callback presently,
		//	it will have to be dispatched in the future
		//	after some processing has been done
		//
		//	Put it into the pending queue to be dispatched
		//	when the requested processing is done
		try {
		
			pending.EmplaceBack(
				target,
				std::move(callback)
			);
		
		} catch (...) {
		
			//	We're in a consistent state,
			//	so we just release the lock and
			//	rethrow
			
			lock.Release();
			
			throw;
		
		}
		
		Word tt=static_cast<Word>(this->target);
		
		//	Check to see if this column is
		//	being processed
		bool retr;
		if (c==tt) {
		
			//	The column is not being processed
			//	as the current and target states
			//	are identical
			//
			//	Therefore we must inform the caller
			//	that they should do the required
			//	processing
			
			//	Update the target state
			this->target=target;
			
			retr=false;
		
		} else {
		
			//	The column is currently being
			//	processed
		
			//	Is the level of processing we
			//	require greater than the level
			//	of processing planned?
			//
			//	If so, update the target
			if (t>tt) this->target=target;
			
			retr=true;
		
		}
		
		lock.Release();
		
		return retr;
	
	}
	
	
	ColumnState ColumnContainer::GetState () const noexcept {
	
		return static_cast<ColumnState>(
			static_cast<Word>(
				curr
			)
		);
	
	}
	
	
	void ColumnContainer::Send () {
	
		lock.Acquire();
		
		sent=true;
		
		//	Do not perform a bulk send if
		//	one has already been performed,
		//	or if there are no clients to
		//	send to
		if (sent || (clients.size()==0)) {
		
			lock.Release();
			
			return;
		
		}
		
		try {
		
			//	Prepare a buffer
			Vector<Byte> buffer(ToChunkData());
			
			//	Number of clients sent to
			Word count=0;
			for (auto & c : clients) {
			
				//	Unordered set only allows const
				//	iteration so that the keys of elements
				//	are preserved, but a smart pointer's
				//	hash is dependent only on the memory
				//	address it points to, so it's fine
				//	to make it modifiable and send through
				//	it
				auto & client=const_cast<SmartPointer<Client> &>(c);
			
				//	If this is the last client
				//	we can just send the master
				//	buffer
				if ((++count)==clients.size()) {
				
					client->Send(std::move(buffer));
				
				//	Otherwise we need to create
				//	a temporary buffer
				} else {
				
					Vector<Byte> temp(buffer.Count());
					temp.SetCount(buffer.Count());
					
					memcpy(
						temp.begin(),
						buffer.begin(),
						buffer.Count()
					);
					
					client->Send(std::move(temp));
				
				}
			
			}
		
		} catch (...) {
		
			lock.Release();
			
			throw;
		
		}
		
		lock.Release();
	
	}
	
	
	void ColumnContainer::AddPlayer (SmartPointer<Client> client) {
	
		lock.Execute([&] () {
		
			//	If the client is already added,
			//	abort
			if (!clients.insert(client).second) return;
			
			//	Send if necessary
			if (sent) {
			
				try {
				
					client->Send(ToChunkData());
				
				} catch (...) {
				
					//	Keep it atomic by
					//	undoing the addition
					clients.erase(client);
					
					throw;
				
				}
			
			}
		
		});
	
	}
	
	
	void ColumnContainer::RemovePlayer (SmartPointer<Client> client, bool force) {
	
		lock.Execute([&] () {
		
			//	Send only if:
			//
			//	A.	The client had been added
			//		to this column.
			//	B.	This column had been sent
			//		to clients.
			//	C.	This isn't a forceful
			//		removal.
			if (
				(clients.erase(client)!=0) &&
				!force &&
				sent
			) client->Send(GetUnload());
		
		});
	
	}
	
	
	void ColumnContainer::Interested () noexcept {
	
		++interest;
	
	}
	
	
	void ColumnContainer::EndInterest () noexcept {
	
		--interest;
	
	}
	
	
	bool ColumnContainer::CanUnload () const noexcept {
	
		return (clients.size()==0) && (interest==0);
	
	}
	
	
	Vector<Byte> ColumnContainer::GetUnload () const {
	
		//	Prepare a buffer
		Word size=(
			sizeof(Byte)+
			sizeof(Int32)+
			sizeof(Int32)+
			sizeof(bool)+
			sizeof(UInt16)+
			sizeof(UInt16)+
			sizeof(Int32)
		);
		
		Vector<Byte> buffer(size);
		
		//	Populate the buffer with
		//	the appropriate data
		buffer.Add(0x33);
		
		PacketHelper<Int32>::ToBytes(
			id.X,
			buffer
		);
		PacketHelper<Int32>::ToBytes(
			id.Z,
			buffer
		);
		PacketHelper<bool>::ToBytes(
			true,
			buffer
		);
		PacketHelper<UInt16>::ToBytes(
			0,
			buffer
		);
		PacketHelper<UInt16>::ToBytes(
			0,
			buffer
		);
		PacketHelper<Int32>::ToBytes(
			0,
			buffer
		);
		
		return buffer;
	
	}


	Vector<Byte> ColumnContainer::ToChunkData () const {
	
		//	First we need to convert the sane
		//	in memory format to the insane
		//	Mojang format
		
		Word size=(
			//	One byte per block
			//	for block type
			(16*16*16*16)+
			//	Half a byte per block
			//	for metadata
			((16*16*16*16)/2)+
			//	Half a byte per block
			//	for light
			((16*16*16*16)/2)+
			//	One byte per column for
			//	biome information
			(16*16)
		);
		
		//	Do we have to send "add"?
		bool add=false;
		for (Word i=0;i<(16*16*16*16);++i) {
		
			if (Blocks[i].GetType()>std::numeric_limits<Byte>::max()) {
			
				add=true;
				
				break;
			
			}
		
		}
		
		//	If we have to send add it's
		//	one half byte per block
		if (add) size+=(16*16*16*16)/2;
		
		//	If we have to send skylight it's
		//	one half byte per block
		bool skylight=HasSkylight(id.Dimension);
		if (skylight) size+=(16*16*16*16)/2;
		
		//	Allocate enough space to hold
		//	the column in Mojang format
		Vector<Byte> column(size);
		column.SetCount(size);
		
		Word offset=0;
		
		//	Loop for block types
		for (const auto & b : Blocks) column[offset++]=static_cast<Byte>(b.GetType());
		
		//	Loop for block metadata
		bool even=true;
		for (const auto & b : Blocks) {
		
			if (even) {
			
				column[offset]=b.GetMetadata()<<4;
				even=false;
				
			} else {
			
				column[offset++]|=b.GetMetadata();
				even=true;
			
			}
			
		}
		
		//	Loop for light
		even=true;
		for (const auto & b : Blocks) {
		
			if (even) {
			
				column[offset]=b.GetLight()<<4;
				even=false;
			
			} else {
			
				column[offset++]|=b.GetLight();
				even=true;
			
			}
		
		}
		
		//	Loop for skylight if applicable
		if (skylight) {
		
			even=true;
			
			for (const auto & b : Blocks) {
			
				if (even) {
				
					column[offset]=b.GetSkylight()<<4;
					even=false;
				
				} else {
				
					column[offset++]|=b.GetSkylight();
					even=true;
				
				}
			
			}
			
		}
		
		//	Loop for "add" if applicable
		if (add) {
		
			even=true;
			
			for (const auto & b : Blocks) {
			
				auto type=b.GetType();
				Byte val=(type>std::numeric_limits<Byte>::max()) ? static_cast<Byte>(type>>8) : 0;
			
				if (even) {
				
					column[offset]=val<<4;
					even=false;
				
				} else {
				
					column[offset++]|=val;
					even=true;
				
				}
			
			}
		
		}
		
		//	Copy biomes
		memcpy(
			&column[offset],
			Biomes,
			sizeof(Biomes)
		);
		
		//	Prepare the final buffer
		size=(
			//	Packet type
			sizeof(Byte)+
			//	X-coordinate of this chunk
			sizeof(Int32)+
			//	Z-coordinate of this chunk
			sizeof(Int32)+
			//	Group-up continuous
			sizeof(bool)+
			//	Primary bit mask
			sizeof(UInt16)+
			//	"Add" bit mask
			sizeof(UInt16)+
			//	Size of compressed data
			sizeof(Int32)+
			//	Largest possible size for
			//	compressed column data
			DeflateBound(
				column.Count()
			)
		);
		
		//	Allocate sufficient memory
		Vector<Byte> buffer(size);
		
		buffer.Add(0x33);
		
		PacketHelper<Int32>::ToBytes(
			id.X,
			buffer
		);
		PacketHelper<Int32>::ToBytes(
			id.Z,
			buffer
		);
		PacketHelper<bool>::ToBytes(
			true,
			buffer
		);
		PacketHelper<UInt16>::ToBytes(
			std::numeric_limits<UInt16>::max(),
			buffer
		);
		PacketHelper<UInt16>::ToBytes(
			std::numeric_limits<UInt16>::max(),
			buffer
		);
		
		//	Push end point of buffer past
		//	where the size will go
		//	(as we do not yet know the size)
		Word size_loc=buffer.Count();
		buffer.SetCount(size_loc+sizeof(Int32));
		
		//	Compress
		Deflate(
			column.begin(),
			column.end(),
			&buffer
		);
		
		//	Get compressed size
		SafeWord compressed_size(buffer.Count()-size_loc-sizeof(Int32));
		
		Word final_size=buffer.Count();
		
		//	Insert compressed size
		buffer.SetCount(size_loc);
		PacketHelper<Int32>::ToBytes(
			Int32(compressed_size),
			buffer
		);
		
		buffer.SetCount(final_size);
		
		return buffer;
	
	}
	
	
	void ColumnContainer::SetBlock (BlockID id, Block block) {
	
		//	Get offset within this column
		auto offset=id.GetOffset();
		
		//	Prepare a packet
		typedef PacketTypeMap<0x35> pt;
		Packet packet;
		packet.SetType<pt>();
		packet.Retrieve<pt,0>()=id.X;
		packet.Retrieve<pt,1>()=id.Y;
		packet.Retrieve<pt,2>()=id.Z;
		packet.Retrieve<pt,3>()=block.GetType();
		packet.Retrieve<pt,4>()=block.GetMetadata();
		
		lock.Execute([&] () {
		
			//	Assign block
			Blocks[offset]=block;
			
			//	Now dirty
			dirty=true;
			
			//	If we've sent this column to players,
			//	send a packet
			if (sent) for (auto & client : clients) const_cast<SmartPointer<Client> &>(client)->Send(packet);
		
		});
	
	}
	
	
	Block ColumnContainer::GetBlock (BlockID id) const noexcept {
	
		//	Get offset within this column
		auto offset=id.GetOffset();
		
		//	Retrieve the appropriate block
		return lock.Execute([&] () {	return Blocks[offset];	});
	
	}
	
	
	void ColumnContainer::Acquire () const noexcept {
	
		lock.Acquire();
	
	}
	
	
	void ColumnContainer::Release () const noexcept {
	
		lock.Release();
	
	}
	
	
	bool ColumnContainer::Dirty () const noexcept {
	
		return dirty;
	
	}
	
	
	void ColumnContainer::Clean () noexcept {
	
		dirty=false;
	
	}
	
	
	static const String to_str_temp("X={0}, Z={1}, Dimension={2}");
	
	
	String ColumnContainer::ToString () const {
	
		return String::Format(
			to_str_temp,
			id.X,
			id.Z,
			id.Dimension
		);
	
	}
	
	
	void * ColumnContainer::Get () noexcept {
	
		return this;
	
	}


}

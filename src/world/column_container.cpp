#include <world/world.hpp>
#include <cstring>
#include <limits>


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
		
		lock.Release();
		
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
		
			//	Get a packet
			auto packet=ToChunkData();
			
			for (auto & c : clients) const_cast<SmartPointer<Client> &>(c)->Send(packet);
		
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
	
	
	ColumnContainer::PacketType ColumnContainer::GetUnload () const {
	
		PacketType retr;
		retr.X=id.X;
		retr.Z=id.Z;
		retr.Continuous=true;
		retr.Primary=0;
		retr.Add=0;
		
		return retr;
	
	}
	
	
	static Byte get_add (const Block & b) noexcept {
	
		auto type=b.GetType();
		
		return (type>std::numeric_limits<Byte>::max()) ? 0 : static_cast<Byte>(type>>BitsPerByte());
	
	}


	ColumnContainer::PacketType ColumnContainer::ToChunkData () const {
		
		//	This gives all chunks from
		//	bottom-to-top which are not
		//	all air
		bool chunks [16];
		std::memset(chunks,0,sizeof(chunks));
		//	This gives all chunks from
		//	bottom-to-top which require the
		//	"add" array
		bool add [16];
		std::memset(add,0,sizeof(add));
		
		//	Scan all blocks to determine
		//
		//	A.	Which chunks we need to send.
		//	B.	Which of A need the add array.
		Word chunk=0;
		Word next_chunk=16*16*16;
		for (Word i=0;i<(16*16*16*16);++i) {
		
			//	Maintain record of which
			//	chunk we're in
			if (i==next_chunk) {
			
				next_chunk+=16*16*16;
				++chunk;
			
			}
			
			//	Type of this block
			auto type=Blocks[i].GetType();
			
			//	Are we sending this chunk?
			if (type!=0) {
			
				//	YES
				
				chunks[chunk]=true;
				
				//	Is the block type too large
				//	for one byte?  I.e. do we have
				//	to send the add array for this
				//	chunk?
				if (type>std::numeric_limits<Byte>::max()) add[chunk]=true;
			
			}
		
		}
		
		//	Do we have to send skylight?
		bool skylight=HasSkylight(id.Dimension);
		
		//	Determine the masks that we'll be
		//	sending, the start offset of the
		//	"add" array, as well as the distance
		//	between arrays
		Word add_offset=0;
		UInt16 primary_mask=0;
		UInt16 add_mask=0;
		Word spacing=0;
		for (Word i=0;i<16;++i) if (chunks[i]) {
		
			//	The "spacing" of arrays, i.e.
			//	if the block type array starts
			//	at index 0, at what index does the
			//	metadata array begin?
			spacing+=16*16*16;
		
			UInt16 mask=1<<i;
		
			//	Add bit in appropriate
			//	position to mask
			primary_mask|=mask;
			
			//	Offset the start of the
			//	"add" array appropriately
			add_offset+=16*16*16;
			if (skylight) add_offset+=(16*16*16)/2;
			
			//	If the "add" array will be
			//	sent for this chunk, update
			//	the mask
			if (add[i]) add_mask|=mask;
		
		}
		
		//	Create a buffer on the stack big
		//	enough to hold the largest possible
		//	packet in Mojang format
		Byte column [(16*16*16*16*3)+(16*16)];
		
		//	Loop and convert
		Word offset=0;
		Word nibble_offset=spacing;
		spacing/=2;
		chunk=0;
		next_chunk=16*16*16;
		bool even=true;
		for (Word i=0;i<(16*16*16*16);++i) {
		
			//	Advance to next chunk if
			//	necessary
			if (i==next_chunk) {
			
				next_chunk+=16*16*16;
				++chunk;
			
			}
			
			//	Skip null chunks
			if (!chunks[chunk]) {
			
				i+=(16*16*16)-1;
				
				continue;
			
			}
			
			//	Current block
			const auto & b=Blocks[i];
			
			//	Write non-"add" byte of block
			//	type
			column[offset++]=static_cast<Byte>(b.GetType());
			Word curr=nibble_offset;
			//	Write nibbles
			if (even) {
			
				//	Metadata
				column[curr]=b.GetMetadata()<<4;
				//	Light
				column[curr+=spacing]=b.GetLight()<<4;
				//	Skylight (if applicable)
				if (skylight) column[curr+spacing]=b.GetSkylight()<<4;
				//	"Add" (if applicable)
				if (add[chunk]) column[add_offset]=get_add(b)<<4;
			
			} else {
			
				//	Metadata
				column[curr]|=b.GetMetadata();
				//	Light
				column[curr+=spacing]|=b.GetLight();
				//	Skylight (if applicable)
				if (skylight) column[curr+spacing]|=b.GetSkylight();
				//	"Add" (if applicable)
				if (add[chunk]) column[add_offset++]|=get_add(b);
				
				//	After every odd/even pair,
				//	we move onto the next full
				//	byte
				++nibble_offset;
			
			}
			
			even=!even;
		
		}
		
		//	Copy biomes
		std::memcpy(
			column+add_offset,
			Biomes,
			sizeof(Biomes)
		);
		
		//	Prepare a packet
		PacketType retr;
		retr.X=id.X;
		retr.Z=id.Z;
		retr.Continuous=true;
		retr.Primary=primary_mask;
		retr.Add=add_mask;
		retr.Data=Deflate(
			column,
			column+add_offset+sizeof(Biomes)
		);
		
		return retr;
	
	}
	
	
	void ColumnContainer::SetBlock (BlockID id, Block block) {
	
		//	Get offset within this column
		auto offset=id.GetOffset();
		
		//	Prepare a packet
		Packets::Play::Clientbound::BlockChange packet;
		packet.X=id.X;
		packet.Y=id.Y;
		packet.Z=id.Z;
		packet.Type=block.GetType();
		packet.Metadata=block.GetMetadata();
		
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

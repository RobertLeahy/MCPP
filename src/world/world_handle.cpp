#include <world/world.hpp>
#include <stdexcept>


namespace MCPP {


	inline void WorldHandle::destroy () noexcept {
	
		//	If we don't have a handle to the
		//	world, that means this object
		//	has been moved, so we shouldn't
		//	do anything
		if (world!=nullptr) {
		
			//	If we have a cached column,
			//	end interest in it
			if (cache!=nullptr) cache->EndInterest();
			
			//	If we're holding the lock,
			//	release it
			if (locked) world->wlock.Release();
			
			//	Prevent this from executing
			//	again
			world=nullptr;
		
		}
	
	}
	
	
	inline ColumnContainer * WorldHandle::get_column_impl (ColumnID id) const {
	
		bool create;
		switch (access) {
		
			case BlockAccessStrategy::Load:
			case BlockAccessStrategy::LoadGenerated:
			case BlockAccessStrategy::Generate:
			case BlockAccessStrategy::Populate:
			default:
				create=true;
				break;
			case BlockAccessStrategy::Generated:
			case BlockAccessStrategy::Populated:
				create=false;
				break;
		
		}
		
		return world->get_column(id,create);
	
	}
	
	
	static inline bool state_at_least (ColumnContainer * column, ColumnState state) noexcept {
	
		return static_cast<Word>(column->GetState())>=static_cast<Word>(state);
	
	}
	
	
	inline ColumnContainer * WorldHandle::get_column (ColumnID id, bool read) const {
	
		//	Check the cache
		if (cache==nullptr) {
		
			//	No cache, therefore we must
			//	get column
		
			cache=get_column_impl(id);
		
		} else if (cache->ID()!=id) {
		
			//	Cache miss, therefore we must
			//	purge old cache and get the
			//	correct column
			
			//	Don't leak interest
			cache->EndInterest();
			
			//	Clear cache to prevent double
			//	end interest if an exception
			//	is thrown
			cache=nullptr;
			
			cache=get_column_impl(id);
		
		}
		
		//	If our access strategy is such that we
		//	don't load/generate/populate columns
		//	in certain situations, we may not have
		//	gotten a column, in which case we fail
		if (cache==nullptr) return nullptr;
		
		//	Certain column states require that
		//	the column be in a certain state already,
		//	otherwise they fail
		if (access==BlockAccessStrategy::Generated) {
		
			return state_at_least(cache,ColumnState::Generated) ? cache : nullptr;
		
		} else if (access==BlockAccessStrategy::Populated) {
		
			return state_at_least(
				cache,
				read ? ColumnState::Populated : ColumnState::Generated
			) ? cache : nullptr;
		
		}
		
		ColumnState target;
		switch (access) {
		
			case BlockAccessStrategy::Load:
			case BlockAccessStrategy::LoadGenerated:
				target=ColumnState::Generating;
				break;
			case BlockAccessStrategy::Generate:
				target=ColumnState::Generated;
				break;
			case BlockAccessStrategy::Populate:
			default:
				target=read ? ColumnState::Populated : ColumnState::Generated;
				break;
		
		}
		
		//	Don't deadlock when populating
		if (
			(populate!=0) &&
			(target==ColumnState::Populated)
		) target=ColumnState::Generated;
		
		//	Wait/process as necessary
		if (!cache->WaitUntil(target)) world->process(*cache,this);
		
		//	If our access strategy is one
		//	of the load-related strategies,
		//	check their state
		if (
			(
				(access==BlockAccessStrategy::Load) &&
				!state_at_least(
					cache,
					read ? ColumnState::Populated : ColumnState::Generated
				)
			) ||
			(
				(access==BlockAccessStrategy::LoadGenerated) &&
				!state_at_least(
					cache,
					ColumnState::Generated
				)
			)
		) return nullptr;
		
		return cache;
	
	}
	
	
	inline bool WorldHandle::set_impl (ColumnContainer * column, BlockID id, Block block, bool force) const {
	
		//	Prepare event
		BlockSetEvent event{
			*this,
			id,
			column->GetBlock(id),
			block
		};
		
		//	If we're not forcing, check to
		//	make sure setting this block
		//	here is permitted
		if (!(force || world->can_set(event))) return false;
		
		//	Set block
		column->SetBlock(id,block);
		
		//	Fire event to notify listeners
		//	that block has been set
		world->on_set(event);
		
		return true;
	
	}


	WorldHandle::WorldHandle (World * world, BlockWriteStrategy write, BlockAccessStrategy access)
		:	write(write),
			access(access),
			world(world),
			locked(false),
			cache(nullptr),
			populate(0)
	{
	
		//	If we're beginning a transaction,
		//	begin it by acquiring the lock
		//	at once
		if (write==BlockWriteStrategy::Transactional) {
		
			world->wlock.Acquire();
			
			locked=true;
		
		}
	
	}
	
	
	WorldHandle::WorldHandle (WorldHandle && other) noexcept
	:	write(other.write),
		access(other.access),
		world(other.world),
		locked(other.locked),
		cache(other.cache),
		populate(other.populate)
	{
	
		//	Null out the other object's
		//	world pointer so it doesn't
		//	get cleaned up
		other.world=nullptr;
	
	}
	
	
	WorldHandle & WorldHandle::operator = (WorldHandle && other) noexcept {
	
		//	Guard against self-assignment
		if (&other!=this) {
		
			//	Clean up this handle
			destroy();
			
			//	Move the contents of
			//	the other handle
			write=other.write;
			access=other.access;
			world=other.world;
			locked=other.locked;
			cache=other.cache;
			populate=other.populate;
			
			//	Invalidate the other
			//	handle
			other.world=nullptr;
		
		}
		
		return *this;
	
	}
	
	
	WorldHandle::~WorldHandle () noexcept {
	
		destroy();
	
	}
	
	
	bool WorldHandle::Set (BlockID id, Block block, bool force) const {
	
		//	Get the column that contains the
		//	block to be set
		auto * column=get_column(
			id.GetContaining(),
			false
		);
		
		//	If the column could not be retrieved,
		//	for whatever reason, fail
		if (column==nullptr) return false;
		
		//	Acquire the world lock if
		//	necessary
		bool locked=false;
		if (!this->locked) {
		
			world->wlock.Acquire();
			
			locked=true;
			this->locked=true;
		
		}
		
		bool retr;
		try {
		
			retr=set_impl(
				column,
				id,
				block,
				force
			);
		
		} catch (...) {
		
			//	Don't leak lock
			if (locked) {
			
				world->wlock.Release();
				
				this->locked=false;
				
			}
			
			throw;
		
		}
		
		if (locked) {
		
			world->wlock.Release();
			
			this->locked=false;
			
		}
		
		return retr;
	
	}
	
	
	Nullable<Block> WorldHandle::Get (BlockID id, std::nothrow_t) const {
	
		Nullable<Block> retr;
		
		//	Get the column that contains the
		//	target block
		auto * column=get_column(
			id.GetContaining(),
			true
		);
		
		//	We can't retrieve a block from
		//	a null column
		if (column==nullptr) return retr;
		
		retr.Construct(column->GetBlock(id));
		
		return retr;
	
	}
	
	
	static const char * block_retrieve_error="Block could not be retrieved";
	
	
	Block WorldHandle::Get (BlockID id) const {
	
		Nullable<Block> retr(
			Get(
				id,
				std::nothrow
			)
		);
		
		if (retr.IsNull()) throw std::runtime_error(block_retrieve_error);
		
		return *retr;
	
	}
	
	
	bool WorldHandle::Exclusive () const noexcept {
	
		return locked;
	
	}
	
	
	void WorldHandle::BeginPopulate () const noexcept {
	
		++populate;
	
	}
	
	
	void WorldHandle::EndPopulate () const noexcept {
	
		--populate;
	
	}


}

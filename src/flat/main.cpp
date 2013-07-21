#include <common.hpp>
#include <type_traits>
#include <cstring>
#include <world/world.hpp>


static const String name("Flat Column Provider");
static const Word priority=1;
static const String key("FLAT");
//	The number of layers of ground
//	to create (including the bedrock
//	layer).
//
//	Must be at least two, and cannot
//	be higher than 254.
static const Word ground_layers=2;


class Flat : public Module {


	private:
	
	
		Byte column_template [(16*16*16*3*16)+(16*16)];
		
		
	public:
	
	
		Flat () noexcept {
		
			//	Get the "real" number of ground
			//	layers (must be at least 2,
			//	can't be more than 254).
			Word layers=(ground_layers<2) ? 2 : ((ground_layers>254) ? 254 : ground_layers);
		
			//	Set the top of the column
			//	to air
			memset(
				column_template,
				0,
				(256-layers)*16*16
			);
			
			//	Set the next layer to grassy
			//	dirt
			memset(
				&column_template[(256-layers)*16*16],
				2,
				16*16
			);
			
			//	Set the next layers (except the
			//	bottom most) to stone.
			memset(
				&column_template[(257-layers)*16*16],
				1,
				(layers-2)*16*16
			);
			
			//	Set the bottom most layer to
			//	bedrock
			memset(
				&column_template[16*16*255],
				7,
				16*16
			);
			
			//	Zero out metadata and light array
			memset(
				&column_template[16*16*256],
				0,
				16*16*256
			);
			
			//	TODO: Meaningful calculation for
			//	sky light
			
			//	Zero out sky light array
			memset(
				&column_template[16*16*256*2],
				0,
				16*16*256/2
			);
			
			//	Zero out the add array
			memset(
				&column_template[16*20*256*2],
				0,
				16*16*256/2
			);
			
			//	Set the biome array -- plains all
			//	around
			memset(
				&column_template[16*16*256*3],
				1,
				16*16
			);
		
		}

		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual void Install () override {
		
			//	Install the provider
			World->Add(
				0,
				key,
				[this] (ColumnID id, Column & column) {
			
					//	Copy over the template
					//	column
					memcpy(
						column.Data,
						column_template,
						(16*16*16*3*16)+(16*16)
					);
					
					//	Populated flags
					column.Skylight=true;
					column.Add=true;
					column.Populated=false;
				
				}
			);
		
		}
		

};


static Nullable<Flat> mod;


extern "C" {


	Module * Load () {
	
		if (mod.IsNull()) mod.Construct();
		
		return &(*mod);
	
	}
	
	
	void Unload () {
	
		mod.Destroy();
	
	}


}
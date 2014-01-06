#include <rleahylib/rleahylib.hpp>
#include <world/world.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <cstring>
#include <limits>
#include <utility>


using namespace MCPP;


static const String name("Super Flat World Generator");
static const Word priority=2;
static const SByte dimensions []={0};
//	Key used in the backing store for the
//	super flat code
static const String key("superflat_preset_code");
//	The world type string which identifies
//	this generator
static const String world_type("FLAT");
//	Default preset code used in the absence
//	of a code from the database
static const String default_preset("2;7,2x3,2");
static const Regex preset_parse("(?<=^|;).*?(?=$|;)");
static const Regex block_parse(
	"(?<=^|,)\\s*(?:(\\d+)\\s*x\\s*)?(\\d+)(?:\\s*\\:\\s*(\\d+))?\\s*(?=$|,)",
	RegexOptions().SetIgnoreCase()
);
//	Presets ripped directly from vanilla
//	Minecraft
static const Vector<Tuple<String,String>> presets={
	{
		"Classic Flat",
		default_preset
	},
	{
		"Tunneler's Dream",
		"2;7,230x1,5x3,2;3"
	},
	{
		"Water World",
		"2;7,5x1,5x3,5x12,90x9;1"
	},
	{
		"Overworld",
		"2;7,59x1,3x3,2;1"
	},
	{
		"Snowy Kingdom",
		"2;7,59x1,3x3,2,78;12"
	},
	{
		"Bottomless Pit",
		"2;2x4,3x3,2;1"
	},
	{
		"Desert",
		"2;7,3x1,52x24,8x12;2"
	},
	{
		"Redstone Ready",
		"2;7,3x1,52x24;2"
	}
};


//	Parses the preset code into a tuple specifying
//	blocks on a per layer basis (from zero up) and
//	biome
static Nullable<Tuple<Vector<Block>,Biome>> parse (const String & code) {

	Nullable<
		Tuple<
			Vector<Block>,
			Biome
		>
	> retr;

	//	Attempt to do a high level parse, extracting
	//	each of the semicolon-delimited substrings
	auto matches=preset_parse.Matches(code);
	
	Word version;	//	Version number
	if (!(
		//	We need at least two matches -- the
		//	version number and the layers
		//	specification
		(matches.Count()>=2) &&
		//	We need to attempt to extract the
		//	version number from the first match.
		//	If this isn't an integer, we cannot
		//	proceed
		matches[0].Value().Trim().ToInteger(&version) &&
		//	Only supported version at the moment
		//	is version 2
		(version==2)
	)) return retr;
	
	//	Default biome is plains
	Biome biome=Biome::Plains;
	//	If there are three or more semicolon-delimited
	//	substrings, the third one specifies a biome
	if (matches.Count()>=3) {
	
		Byte biome_byte;
		
		//	Check to make sure the third match is
		//
		//	A.	A valid byte.
		//	B.	A valid biome.
		//
		//	If it is not, fail out.
		if (!(
			matches[2].Value().Trim().ToInteger(&biome_byte) &&
			IsValidBiome(biome_byte)
		)) return retr;
		
		//	Biome byte has been validated, assign
		biome=static_cast<Biome>(biome_byte);
	
	}
	
	//	Attempt to match layer specifications
	Vector<Block> layers;
	for (auto & match : block_parse.Matches(matches[1].Value())) {
	
		//	If we've added enough layers already,
		//	abort
		if (layers.Count()>256) return retr;
	
		//	Number of times this layer will
		//	be repeated -- defaults to 1
		Word multiplier=1;
		//	ID of the block to fill this layer
		//	with
		UInt16 id;
		//	Metadata to use for the blocks
		//	making up this layer -- defaults
		//	to zero
		Byte metadata=0;
		
		//	Attempt to extract data from
		//	regular expression match
		if (!(
			//	Extract multiplier if it's
			//	specified
			(
				(match[1].Count()==0) ||
				(
					match[1].Value().ToInteger(&multiplier) &&
					//	Sanity check on the multiplier
					((layers.Count()+multiplier)<=256)
				)
			) &&
			//	Extract block ID
			match[2].Value().ToInteger(&id) &&
			//	Sanity check block ID -- valid
			//	block IDs are 0-4095 (inclusive)
			(id<4096) &&
			//	Extract metadata if it's
			//	specified
			(
				(match[3].Count()==0) ||
				(
					match[3].Value().ToInteger(&metadata) &&
					//	Valid metadata values are
					//	0-15 (inclusive)
					(metadata<16)
				)
			)
		)) return retr;
		
		//	VALID -- add layers
		
		Block block(id);
		block.SetMetadata(metadata);
		
		for (Word i=0;i<multiplier;++i) layers.Add(block);
	
	}
	
	//	If no layers were specified, abort
	if (layers.Count()==0) return retr;
	
	retr.Construct(
		std::move(layers),
		biome
	);
	
	return retr;

}


class SuperFlatGenerator : public Module, public WorldGenerator {


	private:
	
	
		//	A template of the column that
		//	will be repeatedly "generated"
		//	via memcpy
		Block blocks [16*16*16*16];
		//	The biome that will be set on
		//	all columns
		Biome biome;
		
		
	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			//	Attempt to get a preset code from
			//	the backing store
			auto code=Server::Get().Data().GetSetting(key);
			
			//	The spec to which we'll build the
			//	template column
			decltype(parse(*code)) spec;
			
			//	Attempt to get a spec from the code
			//	loaded from the backing store
			if (!code.IsNull()) {
			
				//	Check to see if the code is one
				//	of the presets
				bool found=false;
				for (auto & t : presets) if (*code==t.Item<0>()) {
				
					spec=parse(t.Item<1>());
					
					found=true;
					
					break;
				
				}
				
				//	If it wasn't a preset, attempt
				//	to parse
				if (!found) spec=parse(*code);
			
			}
			
			//	If that fails, attempt to get a spec
			//	from the default preset
			if (spec.IsNull()) spec=parse(default_preset);
			
			//	If that fails, PANIC
			if (spec.IsNull()) {
			
				Server::Get().Panic();
				
				//	DO NOT EXECUTE THE REMAINDER
				//	OF THIS FUNCTION
				return;
				
			}
			
			const auto & layers=spec->Item<0>();
			biome=spec->Item<1>();
			
			//	Loop over the template column in memory
			//	order (for locality's sake)
			Word offset=0;	//	Offset within the template column
			for (Byte y=0;;++y) {
			
				for (Word z=0;z<16;++z) for (Word x=0;x<16;++x) {
				
					//	If we're within range of the specified
					//	blocks, set the specified block, otherwise
					//	use air
					blocks[offset++]=(y<layers.Count()) ? layers[y] : Block();
				
				}
			
				//	Check for end of loop
				if (y==std::numeric_limits<Byte>::max()) break;
			
			}
		
			//	Install ourselves into the
			//	world container
			auto & world=World::Get();
			for (auto d : dimensions) world.Add(
				this,
				world_type,
				d
			);
		
		}
		
		
		virtual void operator () (ColumnContainer & column) const override {
		
			//	Ensure that the copy operation
			//	is safe
			static_assert(
				(sizeof(column.Blocks)==sizeof(blocks)),
				"Layout incompatible"
			);
		
			//	Since all superflat generated
			//	columns are identical, we just
			//	have to memcpy
			std::memcpy(
				column.Blocks,
				blocks,
				sizeof(column.Blocks)
			);
			
			//	Loop and set all biomes
			for (auto & b : column.Biomes) b=biome;
		
		}


};


INSTALL_MODULE(SuperFlatGenerator)

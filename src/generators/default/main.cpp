#include <rleahylib/rleahylib.hpp>
#include <world/world.hpp>
#include <fma.hpp>
#include <mod.hpp>
#include <noise.hpp>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>


#pragma GCC optimize ("fast-math")


//	Horrible, evil token pasting macros
//	to make life easier
#define PASTE(x,y) x ## y
#define SETTING_PREFIX "default_generator_"
#define GET(x,y) x(PASTE(get_,y)(PASTE(x,_key),PASTE(x,_default)))
#define GET_INT(x) GET(x,int)
#define GET_DBL(x) GET(x,dbl)
#define GET_THRSHLD(x) GET(x,thrshld)
#define DEFAULT_KEY(x) static const String PASTE(x,_key)(SETTING_PREFIX #x)
#define DEFAULT(x,y,z) DEFAULT_KEY(x); \
	static const z PASTE(x,_default)=y
#define DEFAULT_DBL(x,y) DEFAULT(x,y,Double)
#define DEFAULT_INT(x,y) DEFAULT(x,y,Word)


using namespace MCPP;


//	Blocks
static const Block bedrock(7);
static const Block air;
static const Block water(9);
static const Block stone(1);
static const Block sand(12);
static const Block dirt(3);
static const Block grass(2);


//	Perturbation-related defaults
DEFAULT_INT(perturbate_octaves,4);
DEFAULT_DBL(perturbate_persistence,0.9);
DEFAULT_DBL(perturbate_frequency,0.0075);
DEFAULT_DBL(perturbate_max,20);
DEFAULT_DBL(perturbate_min,-20);


//	Ocean-related defaults
DEFAULT_INT(ocean_octaves,4);
DEFAULT_DBL(ocean_persistence,0.7);
DEFAULT_DBL(ocean_frequency,0.00005);
DEFAULT_DBL(continental_shelf_threshold,-0.0075);
DEFAULT_DBL(ocean_threshold,-0.025);
DEFAULT_DBL(beach_threshold,0);
DEFAULT_DBL(land_threshold,0.0125);


//	Min-/max-/height-related defaults
DEFAULT_INT(min_octaves,4);
DEFAULT_DBL(min_persistence,0.5);
DEFAULT_DBL(min_frequency,0.00005);
DEFAULT_INT(max_octaves,4);
DEFAULT_DBL(max_persistence,0.9);
DEFAULT_DBL(max_frequency,0.0005);
DEFAULT_INT(heightmap_octaves,8);
DEFAULT_DBL(heightmap_persistence,0.5);
DEFAULT_DBL(heightmap_frequency,0.001);


//	River-related defaults
DEFAULT_INT(river_octaves,4);
DEFAULT_DBL(river_persistence,0.8);
DEFAULT_DBL(river_frequency,0.0001);
DEFAULT_DBL(river_threshold,0.95);
DEFAULT_DBL(river_transition_threshold,0.025);
DEFAULT_DBL(river_beach_threshold,0.01);
DEFAULT_DBL(river_thicken_factor,0.03);
DEFAULT_DBL(river_thin_factor,0.03);
DEFAULT_DBL(river_delta_factor,0.25);
DEFAULT_DBL(river_delta_threshold,0.04);
DEFAULT_DBL(river_depth,6);
DEFAULT_DBL(river_depth_max,12);
DEFAULT_DBL(river_depth_min,0);	


//	Misc defaults
DEFAULT_INT(sea_level,64);
DEFAULT_INT(ocean_ceiling,68);
DEFAULT_INT(ocean_floor,48);
DEFAULT_INT(ocean_ceiling_max,ocean_ceiling_default+16);
DEFAULT_INT(ocean_floor_min,ocean_floor_default-16);
DEFAULT_KEY(max_thresholds);
static const Vector<Tuple<Double,Double>> max_thresholds_default={
	{0.4,72},
	{0.8,256}
};
DEFAULT_KEY(min_thresholds);
static const Vector<Tuple<Double,Double>> min_thresholds_default={
	{0,52},
	{0.2,64},
	{0.6,64},
	{0.9,128}
};
DEFAULT_INT(surface_buffer,3);
DEFAULT_INT(grass_height_limit,128);
DEFAULT_DBL(max_offset,10000);
DEFAULT_DBL(min_offset,-10000);
DEFAULT_DBL(offset_x,0);
DEFAULT_DBL(offset_z,0);


class DefaultGenerator : public WorldGenerator {


	enum class Type {
	
		River,
		Ocean,
		ContinentalShelf,
		Beach,
		Transition,
		Land
	
	};


	private:
	
	
		//	Gets a pseudo-random double on a certain range
		template <typename T>
		static inline Double get_random (T & gen, Double min, Double max) {

			typedef decltype(gen()) result;
		
			//	Create a structure to hold
			//	a 32-bit unsigned integer and
			//	enough generator outputs to
			//	compose a 32-bit unsigned integer
			union {
				UInt32 out;
				result in [
					(sizeof(result)<sizeof(out))
						?	(
								//	Divide the size of
								//	the 32-bit unsigned
								//	integer by the size
								//	of the result to determine
								//	how many results it's
								//	going to take to compose
								//	the 32-bit unsigned integer
								(sizeof(out)/sizeof(result))+
								//	Add 1 if the division isn't
								//	even so that the entire
								//	output is filled with random
								//	bits
								(
									((sizeof(out)%sizeof(result))==0)
										?	0
										:	1
								)
							)
						:	1
				];
			};
			
			//	Populate the above structure
			//	with random bits
			for (auto & n : in) n=gen();
			
			return fma(
				//	The distance between the
				//	top and bottom of the range
				max-min,
				//	Multiplied by a number on the range
				//	0..1 to get the distance from min that
				//	the result should be
				static_cast<Double>(out)/std::numeric_limits<decltype(out)>::max(),
				//	Added to min to place this number back
				//	in the range
				min
			);
		
		}
		
		
		//	Places a value according to a sequence of
		//	thresholds.
		static inline Double threshold_value (
			const Vector<Tuple<Double,Double>> & thresholds,
			Double val
		) noexcept {
		
			if (val<thresholds[0].Item<0>()) return thresholds[0].Item<1>();
			
			for (Word i=1;i<thresholds.Count();++i) {
			
				if (val<thresholds[i].Item<0>()) {
				
					Word index=i-1;
					
					return fma(
						Normalize(
							thresholds[index].Item<0>(),
							thresholds[i].Item<0>(),
							val
						),
						thresholds[i].Item<1>()-thresholds[index].Item<1>(),
						thresholds[index].Item<1>()
					);
				
				}
			
			}
			
			return thresholds[thresholds.Count()-1].Item<1>();
		
		}
		
		
		static inline Nullable<String> get_setting (const String & name) {
		
			return Server::Get().Data().GetSetting(name);
		
		}
		
		
		//	Gets a setting from the backing
		//	store
		static inline Word get_int (const String & name, Word default_val) {
		
			auto setting=get_setting(name);
			Word retr;
			return (setting.IsNull() || !setting->ToInteger(&retr)) ? default_val : retr;
		
		}
		
		
		//	Gets a setting from the backing
		//	store
		static inline Double get_dbl (const String & name, Double default_val) {
		
			auto setting=get_setting(name);
			
			if (setting.IsNull()) return default_val;
			
			auto c_str=setting->ToCString();
			char * end;
			Double retr=strtod(
				static_cast<char *>(c_str),
				&end
			);
			
			return (end==static_cast<char *>(c_str)) ? default_val : retr;
		
		}
		
		
		static inline Vector<Tuple<Double,Double>> get_thrshld (const String & name, Vector<Tuple<Double,Double>> default_val) {
		
			//	TODO: Implement
			return default_val;
		
		}
		
		
		//
		//	DOMAIN PERTURBATION
		//
		
		
		Simplex perturbate_x;
		Simplex perturbate_z;
		Word perturbate_octaves;
		Double perturbate_persistence;
		Double perturbate_frequency;
		Double perturbate_max;
		Double perturbate_min;
		
		
		Tuple<Double,Double> perturbate (Double x, Byte y, Double z) const noexcept {
		
			return Tuple<Double,Double>(x,z);
		
			return Tuple<Double,Double>(
				x+Scale(
					perturbate_min,
					perturbate_max,
					-1,
					1,
					Octave(
						perturbate_octaves,
						perturbate_persistence,
						perturbate_frequency,
						perturbate_x,
						x,
						y,
						z
					)
				),
				z+Scale(
					perturbate_min,
					perturbate_max,
					-1,
					1,
					Octave(
						perturbate_octaves,
						perturbate_persistence,
						perturbate_frequency,
						perturbate_z,
						x,
						y,
						z
					)
				)
			);
		
		}
		
		
		//
		//	OCEANS
		//
		
		
		Simplex ocean;
		Word ocean_octaves;
		Double ocean_persistence;
		Double ocean_frequency;
		Double continental_shelf_threshold;
		Double ocean_threshold;
		Double beach_threshold;
		Double land_threshold;
		
		
		Tuple<Type,Double,Double> get_ocean (Double x, Double z) const noexcept {
		
			//	Get noise value
			auto val=Octave(
				ocean_octaves,
				ocean_persistence,
				ocean_frequency,
				ocean,
				x,
				z
			);
			
			//	Determine what it corresponds
			//	to
			Type retr;
			Double dist;
			if (val<ocean_threshold) {
			
				//	OCEAN
			
				retr=Type::Ocean;
				dist=Normalize(
					-1,
					ocean_threshold,
					val
				);
			
			} else if (val<continental_shelf_threshold) {
			
				//	CONTINENTAL SHELF
			
				retr=Type::ContinentalShelf;
				dist=Normalize(
					ocean_threshold,
					continental_shelf_threshold,
					val
				);
			
			} else if (val<beach_threshold) {
			
				//	BEACH
			
				retr=Type::Beach;
				dist=Normalize(
					continental_shelf_threshold,
					beach_threshold,
					val
				);
			
			} else if (val<land_threshold) {
			
				//	TRANSITION
			
				retr=Type::Transition;
				dist=Normalize(
					beach_threshold,
					land_threshold,
					val
				);
			
			} else {
			
				//	LAND
			
				retr=Type::Land;
				dist=Normalize(
					land_threshold,
					1,
					val
				);
			
			}
			
			return Tuple<Type,Double,Double>(retr,dist,val);
		
		}
		
		
		//
		//	MIN/MAX/HEIGHT
		//
		
		
		Simplex min;
		Simplex max;
		Simplex heightmap;
		Word min_octaves;
		Double min_persistence;
		Double min_frequency;
		Word max_octaves;
		Double max_persistence;
		Double max_frequency;
		Word heightmap_octaves;
		Double heightmap_persistence;
		Double heightmap_frequency;
		
		
		Tuple<Double,Double,Double> get_height (Double x, Double z) const noexcept {
		
			return Tuple<Double,Double,Double>(
				//	Minimum
				Scale(
					0,
					1,
					-1,
					1,
					Octave(
						min_octaves,
						min_persistence,
						min_frequency,
						min,
						x,
						z
					)
				),
				//	Maximum
				Scale(
					0,
					1,
					-1,
					1,
					Octave(
						max_octaves,
						max_persistence,
						max_frequency,
						max,
						x,
						z
					)
				),
				//	Heightmap
				Scale(
					0,
					1,
					-1,
					1,
					Octave(
						heightmap_octaves,
						heightmap_persistence,
						heightmap_frequency,
						heightmap,
						x,
						z
					)
				)
			);
		
		}
		
		
		Word sea_level;
		Word ocean_ceiling;
		Word ocean_floor;
		Word ocean_ceiling_max;
		Word ocean_floor_min;
		Vector<Tuple<Double,Double>> max_thresholds;
		Vector<Tuple<Double,Double>> min_thresholds;
		
		
		Word get_height (Type type, Double dist, Double min, Double max, Double height_val) const noexcept {
		
			//	Beaches are straightforward --
			//	they're always at sea level
			if (type==Type::Beach) return sea_level;
			
			//	We'll have to calculate
			//	a ceiling and a floor,
			//	and then use a noise value
			//	to select between them
			Double ceiling;
			Double floor;
			if (
				(type==Type::Ocean) ||
				(type==Type::ContinentalShelf)
			) {
			
				ceiling=Select(
					ocean_ceiling,
					ocean_ceiling_max,
					min
				);
				floor=Select(
					ocean_floor_min,
					ocean_floor,
					max
				);
			
			} else {
			
				ceiling=threshold_value(
					max_thresholds,
					max
				);
				floor=threshold_value(
					min_thresholds,
					min
				);
			
			}
			
			if (floor>ceiling) std::swap(floor,ceiling);
			
			//	If we're in a transition zone, we
			//	need to "dampen" the floor and
			//	ceiling by pulling them towards
			//	sea level
			if (
				(type==Type::ContinentalShelf) ||
				(type==Type::Transition)
			) {
			
				//	Get the scaling factor
				//
				//	The scaling factor is 1
				//	at beaches, and 0 at the
				//	ocean/land
				Double factor=(type==Type::Transition) ? (1-dist) : dist;
				
				//	Smoothly reduce the ceiling and
				//	floor so that at beaches they
				//	reach beach level
				ceiling-=(ceiling-sea_level)*factor;
				floor-=(floor-sea_level)*factor;
			
			}
			
			//	Use the height value to determine
			//	where between the floor and ceiling
			//	the column height shall be
			return floor+((ceiling-floor)*height_val);
		
		}
		
		
		//
		//	RIVERS
		//
		
		
		Simplex river;
		Word river_octaves;
		Double river_persistence;
		Double river_frequency;
		
		
		Double get_river (Double x, Double z) const noexcept {
		
			return Ridged(
				Octave(
					river_octaves,
					river_persistence,
					river_frequency,
					river,
					x,
					z
				)
			);
		
		}
		
		
		Double river_threshold;
		Double river_transition_threshold;
		Double river_beach_threshold;
		Double river_thicken_factor;
		Double river_thin_factor;
		Double river_delta_factor;
		Double river_delta_threshold;
		Double river_depth;
		Double river_depth_max;
		Double river_depth_min;
		
		
		void get_river (Type & type, Double ocean_val, Word & height, Double x, Double z, Double min, Double max, Double height_val) const noexcept {
		
			//	Rivers do not occur in the
			//	ocean
			if (type==Type::Ocean) return;
			
			//	Get river noise value
			Double val=get_river(x,z);
			
			//	Determine the "height" of the 
			//	world.
			//
			//	This is the average of the three
			//	noise values which contribute to
			//	the world's height, and allows
			//	us to scale rivers up or down
			//	depending on whether this area
			//	of the world is considered to be
			//	"high" or "low".
			Double avg=(height_val+max+min)/3;
			
			//	Create a mutable, local copy
			//	of the river threshold so we
			//	can scale it up or down
			Double rt=river_threshold;
			
			//	Height is "high", we decrease
			//	the intensity of rivers etc.
			if (avg>0.5) rt+=(avg-0.5)*2*river_thin_factor;
			//	Height is "low", we increase the
			//	intensity of rivers
			else rt-=(1-(avg*2))*river_thicken_factor;
			
			//	Near the ocean we thicken rivers
			//	out substantiantally to form deltas
			if (ocean_val<river_delta_threshold) rt-=Normalize(
				river_delta_threshold,
				ocean_threshold,
				ocean_val
			)*river_delta_factor;
			
			//	We get rivers only over the
			//	threshold
			if (val<rt) return;
			
			//	Determine what the relationship between
			//	the original river threshold and the
			//	current river threshold is.
			//
			//	This will be used to scale other thresholds,
			//	to maintain proportionality and smoothness.
			Double scalar=river_threshold/rt;
			
			//	We'll need a transitional
			//	area where the height of
			//	the world gradually is shaped
			//	to meet the river
			Double rtt=rt+(river_transition_threshold*scalar);
			//	And after that there will need
			//	to be a beach
			Double rbt=rtt+(river_beach_threshold*scalar);
			
			if (val<=rbt) {
			
				//	Transition
				
				//	Transitions don't occur on beaches
				//	or in the continental shelf
				if (
					(type==Type::Beach) ||
					(type==Type::ContinentalShelf)
				) return;
				
				//	Are we in a beach or a transition?
				if (val<=rtt) {
				
					//	Transition
					
					height=static_cast<Word>(
						Select(
							height,
							sea_level,
							Normalize(
								rt,
								rtt,
								val
							)
						)
					);
					
					return;
				
				}
				
				//	Beach
				
				height=sea_level;
				type=Type::Beach;
				
				return;
			
			}
			
			//	River
			
			//	Decide on the depth based on
			//	the value of the noise
			height=static_cast<Word>(
				Select(
					sea_level,
					sea_level-river_depth,
					Normalize(
						rbt,
						1,
						val
					)
				)
			);
			
			type=Type::River;
		
		}
		
		
		//
		//	MISC
		//
		
		
		Double offset_x;
		Double offset_z;
		

		Word surface_buffer;
		Word grass_height_limit;
		
		
	public:
	
	
		DefaultGenerator () = delete;
		DefaultGenerator (const DefaultGenerator &) = delete;
		DefaultGenerator (DefaultGenerator &&) = delete;
		DefaultGenerator & operator = (const DefaultGenerator &) = delete;
		DefaultGenerator & operator = (DefaultGenerator &&) = delete;
		
		
		template <typename T>
		DefaultGenerator (
			T & gen
		)	:	perturbate_x(gen),
				perturbate_z(gen),
				GET_INT(perturbate_octaves),
				GET_DBL(perturbate_persistence),
				GET_DBL(perturbate_frequency),
				GET_DBL(perturbate_max),
				GET_DBL(perturbate_min),
				ocean(gen),
				GET_INT(ocean_octaves),
				GET_DBL(ocean_persistence),
				GET_DBL(ocean_frequency),
				GET_DBL(continental_shelf_threshold),
				GET_DBL(ocean_threshold),
				GET_DBL(beach_threshold),
				GET_DBL(land_threshold),
				min(gen),
				max(gen),
				heightmap(gen),
				GET_INT(min_octaves),
				GET_DBL(min_persistence),
				GET_DBL(min_frequency),
				GET_INT(max_octaves),
				GET_DBL(max_persistence),
				GET_DBL(max_frequency),
				GET_INT(heightmap_octaves),
				GET_DBL(heightmap_persistence),
				GET_DBL(heightmap_frequency),
				GET_INT(sea_level),
				GET_INT(ocean_ceiling),
				GET_INT(ocean_floor),
				GET_INT(ocean_ceiling_max),
				GET_INT(ocean_floor_min),
				GET_THRSHLD(max_thresholds),
				GET_THRSHLD(min_thresholds),
				river(gen),
				GET_INT(river_octaves),
				GET_DBL(river_persistence),
				GET_DBL(river_frequency),
				GET_DBL(river_threshold),
				GET_DBL(river_transition_threshold),
				GET_DBL(river_beach_threshold),
				GET_DBL(river_thicken_factor),
				GET_DBL(river_thin_factor),
				GET_DBL(river_delta_factor),
				GET_DBL(river_delta_threshold),
				GET_DBL(river_depth),
				GET_DBL(river_depth_max),
				GET_DBL(river_depth_min),
				GET_INT(surface_buffer),
				GET_INT(grass_height_limit)
		{
		
			Double GET_DBL(max_offset);
			Double GET_DBL(min_offset);
			
			this->offset_x=get_random(
				gen,
				min_offset,
				max_offset
			);
			this->offset_z=get_random(
				gen,
				min_offset,
				max_offset
			);
			
			Double GET_DBL(offset_x);
			this->offset_x+=offset_x;
			
			Double GET_DBL(offset_z);
			this->offset_z+=offset_z;
		
		}
		
		
		virtual void operator () (ColumnContainer & column) const override {
		
			auto id=column.ID();
			
			Int32 start_x=id.GetStartX();
			Int32 end_x=id.GetEndX();
			Int32 start_z=id.GetStartZ();
			Int32 end_z=id.GetEndZ();
			
			Word offset=0;
			Word biome=0;
			
			for (Byte y=0;;++y) {
			
				for (Int32 z=start_z;z<=end_z;++z)
				for (Int32 x=start_x;x<=end_x;++x) {
				
					//	Bottom layer is unconditionally
					//	bedrock
					if (y==0) {
					
						column.Blocks[offset++]=bedrock;
						
						continue;
						
					}
					
					//	Adjust X and Z co-ordinates with
					//	perturbation
					auto perturb=perturbate(
						x+offset_x,
						y,
						z+offset_z
					);
					Double dbl_x=perturb.Item<0>();
					Double dbl_z=perturb.Item<1>();
					
					//	Get the ocean values
					auto ocean=get_ocean(dbl_x,dbl_z);
					Type type=ocean.Item<0>();
					
					//	Get min and max noise
					//	values
					auto height_t=get_height(dbl_x,dbl_z);
					Double min=height_t.Item<1>();
					Double max=height_t.Item<2>();
					Double height_val=height_t.Item<2>();
					
					//	Determine the height of this
					//	column
					Word height=get_height(
						type,
						ocean.Item<1>(),
						min,
						max,
						height_val
					);
					
					//	Generate rivers
					get_river(
						type,
						ocean.Item<2>(),
						height,
						dbl_x,
						dbl_z,
						min,
						max,
						height_val
					);
					
					//	Determine what type of block
					//	to return
					Block block=(
						(
							(type==Type::River) ||
							(type==Type::Beach) ||
							(type==Type::ContinentalShelf) ||
							(type==Type::Ocean)
						)
							//	Ocean logic
							?	(
									(y>height)
										//	We're above this column's height --
										//	we're returning either water (if
										//	below sea level) or air (otherwise).
										?	(
												(y>sea_level)
													?	air
													:	water
											)
										//	We're at or below the column's
										//	height -- we're returning either
										//	sand or dirt (if within a certain
										//	distance of the height) or stone
										//	(otherwise).
										:	(
												(y<(height-surface_buffer))
													?	stone
													//	If the height is above
													//	sea level -- as in an
													//	island -- switch to using
													//	dirt
													:	(
															(height>sea_level)
																?	(
																		(y==height)
																			?	grass
																			:	dirt
																	)
																:	sand
														)
											)
								)
							//	Land logic
							:	(
									(y>height)
										//	Above the height, return air
										//	unconditionally
										?	air
										//	If the height of this column
										//	is above a certain threshold,
										//	we return stone -- grass
										//	grass does not form above a
										//	certain elevation
										:	(height>grass_height_limit)
												?	stone
												//	We return grass at the
												//	surface, dirt within a
												//	buffer distance to the
												//	surface, and stone under
												//	that
												:	(
														(y==height)
															?	grass
															:	(
																	(y<(height-surface_buffer))
																		?	stone
																		:	dirt
																)
													)
								)			
					);
					
					block.SetSkylight(15);
					block.SetLight(15);
					
					if ((y<sea_level) && (block.GetType()==0)) {
					
						Server::Get().WriteLog("AAAA",Service::LogType::Debug);
					
					}
					
					column.Blocks[offset++]=block;
					
					//	Set biome if this is the
					//	last block in this column
					if (y==std::numeric_limits<Byte>::max()) {
					
						Biome b;
						switch (type) {
						
							case Type::Ocean:
							case Type::ContinentalShelf:
								b=Biome::Ocean;
								break;
								
							case Type::River:
								b=Biome::River;
								break;
								
							default:
								b=Biome::Plains;
								break;
						
						}
						
						column.Biomes[biome++]=b;
					
					}
				
				}
				
				//	Check loop break condition
				if (y==std::numeric_limits<Byte>::max()) break;
				
			}
		
		}


};


static const String name("Default World Generator");
static const Word priority=2;
static const SByte dimensions []={0};


class DefaultGeneratorInstaller : public Module {

	
	private:
	
	
		Nullable<DefaultGenerator> generator;
		
		
	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			auto & world=World::Get();
		
			//	Create and seed a Mersenne
			//	Twister random number generator
			//	which shall be used to seed
			//	the Simplex noise generators
			
			//	Mersenne Twister pseudo-random
			//	number generators have a very
			//	large internal state, and require
			//	a "warm up" to fill their large
			//	internal state with non-zero bits
			
			//	We only have a 64-bit seed, we'll
			//	use a seed sequence (deterministic
			//	as per 26.5.7.1 ISO C++11)
			
			std::seed_seq seq({world.Seed()});
			
			std::mt19937 gen(seq);
			
			//	Create the world generator
			generator.Construct(gen);
			
			//	Install the world generator
			for (auto d : dimensions) world.Add(
				&(*generator),
				d
			);
		
		}


};


static Nullable<DefaultGeneratorInstaller> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

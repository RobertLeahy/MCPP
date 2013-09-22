#include <world/world.hpp>
#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <noise.hpp>
#include <fma.hpp>
#include <type_traits>
#include <limits>
#include <utility>
#include <random>
#include <cstdlib>


#pragma GCC optimize ("fast-math")


//	Horrible, evil token pasting macros
//	to make life easier
#define GET(x,y) x( get_ ## y ( x ## _key , x ## _default ))
#define GET_INT(x) GET(x,int)
#define GET_DBL(x) GET(x,dbl)
#define GET_THRSHLD(x) GET(x,thrshld)


namespace MCPP {


	static const String setting_prefix("default_generator_");


	//	Blocks
	static const Block bedrock(7);
	static const Block air;
	static const Block water(9);
	static const Block stone(1);
	static const Block sand(12);
	static const Block dirt(3);
	static const Block grass(2);


	//	Perturbation-related keys
	static const String perturbate_octaves_key("perturbate_octaves");
	static const String perturbate_persistence_key("perturbate_persistence");
	static const String perturbate_frequency_key("perturbate_frequency");
	static const String perturbate_max_key("perturbate_max");
	static const String perturbate_min_key("perturbate_min");
	static const String perturbate_gain_key("perturbate_gain");
	//	Perturbation-related defaults
	static const Word perturbate_octaves_default=3;
	static const Double perturbate_persistence_default=0.4;
	static const Double perturbate_frequency_default=0.01;
	static const Double perturbate_max_default=20;
	static const Double perturbate_min_default=-20;
	static const Double perturbate_gain_default=0.75;


	//	Ocean-related keys
	static const String ocean_octaves_key("ocean_octaves");
	static const String ocean_persistence_key("ocean_persistence");
	static const String ocean_frequency_key("ocean_frequency");
	static const String continental_shelf_threshold_key("continental_shelf_threshold");
	static const String ocean_threshold_key("ocean_threshold");
	static const String beach_threshold_key("beach_threshold");
	static const String land_threshold_key("land_threshold");
	//	Ocean-related defaults
	static const Word ocean_octaves_default=4;
	static const Double ocean_persistence_default=0.7;
	static const Double ocean_frequency_default=0.00005;
	static const Double continental_shelf_threshold_default=-0.0125;
	static const Double ocean_threshold_default=-0.025;
	static const Double beach_threshold_default=0;
	static const Double land_threshold_default=0.0125;


	//	Min-/max-/height-related keys
	static const String min_octaves_key("min_octaves");
	static const String min_persistence_key("min_persistence");
	static const String min_frequency_key("min_frequency");
	static const String max_octaves_key("max_octaves");
	static const String max_persistence_key("max_persistence");
	static const String max_frequency_key("max_frequency");
	static const String heightmap_octaves_key("heightmap_octaves");
	static const String heightmap_persistence_key("heightmap_persistence");
	static const String heightmap_frequency_key("heightmap_frequency");
	//	Min-/max-/height-related defaults
	static const Word min_octaves_default=4;
	static const Double min_persistence_default=0.5;
	static const Double min_frequency_default=0.00005;
	static const Word max_octaves_default=4;
	static const Double max_persistence_default=0.9;
	static const Double max_frequency_default=0.0005;
	static const Word heightmap_octaves_default=8;
	static const Double heightmap_persistence_default=0.5;
	static const Double heightmap_frequency_default=0.001;


	//	Misc keys
	static const String sea_level_key("sea_level");
	static const String ocean_ceiling_key("ocean_ceiling");
	static const String ocean_floor_key("ocean_floor");
	static const String ocean_ceiling_max_key("ocean_ceiling_max");
	static const String ocean_floor_min_key("ocean_floor_min");
	static const String max_thresholds_key("max_thresholds");
	static const String min_thresholds_key("min_thresholds");

	static const String surface_buffer_key("surface_buffer");
	static const String grass_height_limit_key("grass_height_limit");
	static const String max_offset_key("max_offset");
	static const String min_offset_key("min_offset");
	//	Misc defaults
	static const Word sea_level_default=64;
	static const Word ocean_ceiling_default=68;
	static const Word ocean_floor_default=48;
	static const Word ocean_ceiling_max_default=ocean_ceiling_default+16;
	static const Word ocean_floor_min_default=ocean_floor_default-16;
	static const Vector<Tuple<Double,Double>> max_thresholds_default={
		{0.4,72},
		{0.8,256}
	};
	static const Vector<Tuple<Double,Double>> min_thresholds_default={
		{0,52},
		{0.2,64},
		{0.6,64},
		{0.9,128}
	};

	static const Word surface_buffer_default=3;
	static const Word grass_height_limit_default=128;
	static const Double max_offset_default=10000;
	static const Double min_offset_default=-10000;


	class DefaultGenerator {
	
	
		enum class Type {
		
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
		
		
			//	Selects a value between lo and hi based on
			//	the value of val.  If val is 1, selects hi
			//	itself.  If val is 0, selects lo itself.
			//	For values in between selects a value placed
			//	proportionally between lo and hi.
			static inline Double select (Double lo, Double hi, Double val) noexcept {
			
				return fma(
					hi-lo,
					val,
					lo
				);
			
			}
		
		
			//	Places a value between 0 and 1 (inclusive)
			//	based on its position relative to lo and hi.
			//
			//	lo and hi define a range between which val will
			//	be assigned a value proportionally-placed between
			//	0 and 1, with values closer to lo yielding results
			//	closer to 0, and values closer to hi yielding
			//	results closer to 1.
			//
			//	Values outside the range defined by lo and hi
			//	shall unconditionally yield the value that would
			//	be yielded were they equal to either lo or hi
			//	(whichever is closer).
			static inline Double normalize (Double lo, Double hi, Double val) noexcept {
			
				if (lo==hi) {

					if (val==lo) return 0.5;
					if (val>lo) return 1;
					return 0;

				}

				bool complement;
				if (lo>hi) {

					complement=true;
					std::swap(lo,hi);

				} else {

					complement=false;

				}

				Double difference=val-lo;
				Double range=hi-lo;

				if (difference<0) return complement ? 1 : 0;
				if (difference>range) return complement ? 0 : 1;

				Double result=difference/range;

				return complement ? 1-result : result;
			
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
							normalize(
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
			
				return Server::Get().Data().GetSetting(setting_prefix+name);
			
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
			Double perturbate_gain;
			
			
			Tuple<Double,Double> perturbate (Double x, Byte y, Double z) const noexcept {
			
				return Tuple<Double,Double>(
					x+MakeScale(
						MakeGain(
							MakeOctave(
								perturbate_x,
								perturbate_octaves,
								perturbate_persistence,
								perturbate_frequency
							),
							perturbate_gain
						),
						perturbate_min,
						perturbate_max
					)(x,y,z),
					z+MakeScale(
						MakeGain(
							MakeOctave(
								perturbate_z,
								perturbate_octaves,
								perturbate_persistence,
								perturbate_frequency
							),
							perturbate_gain
						),
						perturbate_min,
						perturbate_max
					)(x,y,z)
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
			
			
			Tuple<Type,Double> get_ocean (Double x, Double z) const noexcept {
			
				//	Get noise value
				auto val=MakeOctave(
					ocean,
					ocean_octaves,
					ocean_persistence,
					ocean_frequency
				)(x,z);
				
				//	Determine what it corresponds
				//	to
				Type retr;
				Double dist;
				if (val<ocean_threshold) {
				
					//	OCEAN
				
					retr=Type::Ocean;
					dist=normalize(
						-1,
						ocean_threshold,
						val
					);
				
				} else if (val<continental_shelf_threshold) {
				
					//	CONTINENTAL SHELF
				
					retr=Type::ContinentalShelf;
					dist=normalize(
						ocean_threshold,
						continental_shelf_threshold,
						val
					);
				
				} else if (val<beach_threshold) {
				
					//	BEACH
				
					retr=Type::Beach;
					dist=normalize(
						continental_shelf_threshold,
						beach_threshold,
						val
					);
				
				} else if (val<land_threshold) {
				
					//	TRANSITION
				
					retr=Type::Transition;
					dist=normalize(
						beach_threshold,
						land_threshold,
						val
					);
				
				} else {
				
					//	LAND
				
					retr=Type::Land;
					dist=normalize(
						land_threshold,
						1,
						val
					);
				
				}
				
				return Tuple<Type,Double>(retr,dist);
			
			}
			
			
			//
			//	MIN/MAX/HEIGHT VAL
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
					MakeScale(
						MakeOctave(
							min,
							min_octaves,
							min_persistence,
							min_frequency
						),
						0,
						1
					)(x,z),
					//	Maximum
					MakeScale(
						MakeOctave(
							max,
							max_octaves,
							max_persistence,
							max_frequency
						),
						0,
						1
					)(x,z),
					//	Heightmap
					MakeScale(
						MakeOctave(
							heightmap,
							heightmap_octaves,
							heightmap_persistence,
							heightmap_frequency
						),
						0,
						1
					)(x,z)
				);
			
			}
			
			
			//
			//	MISC
			//
			
			
			Double offset_x;
			Double offset_z;
			
			
			Word sea_level;
			Word ocean_ceiling;
			Word ocean_floor;
			Word ocean_ceiling_max;
			Word ocean_floor_min;
			Vector<Tuple<Double,Double>> max_thresholds;
			Vector<Tuple<Double,Double>> min_thresholds;
			Word surface_buffer;
			Word grass_height_limit;
			Word snow_height_threshold;
			
			
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
					GET_DBL(perturbate_gain),
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
					GET_INT(surface_buffer),
					GET_INT(grass_height_limit)
			{
			
				Double GET_DBL(max_offset);
				Double GET_DBL(min_offset);
				
				offset_x=get_random(
					gen,
					min_offset,
					max_offset
				);
				offset_z=get_random(
					gen,
					min_offset,
					max_offset
				);
			
			}
			
			
			Block operator () (const BlockID & id) const noexcept {
			
				//	If we're filling in the bottom
				//	layer just return bedrock unconditionally
				if (id.Y==0) return bedrock;
			
				//	Adjust X and Z co-ordinates with
				//	perturbation
				auto perturb=perturbate(
					id.X+offset_x,
					id.Y,
					id.Z+offset_z
				);
				Double x=perturb.Item<0>();
				Double z=perturb.Item<1>();
				
				//	Get the ocean values
				auto ocean=get_ocean(x,z);
				Type type=ocean.Item<0>();
				Double type_dist=ocean.Item<1>();
				
				//	Determine the height of this
				//	column
				Word height;
				if (type==Type::Beach) {
				
					//	Beaches are straightforward --
					//	they're always at sea level
					height=sea_level;
				
				} else {
				
					//	We'll need min and max
					//	noise values
					auto height_t=get_height(x,z);
					Double min=height_t.Item<1>();
					Double max=height_t.Item<2>();
					Double height_val=height_t.Item<2>();
				
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
					
						ceiling=select(
							ocean_ceiling,
							ocean_ceiling_max,
							min
						);
						floor=select(
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
						Double factor=(type==Type::Transition) ? (1-type_dist) : type_dist;
						
						//	Smoothly reduce the ceiling and
						//	floor so that at beaches they
						//	reach beach level
						ceiling-=(ceiling-sea_level)*factor;
						floor-=(floor-sea_level)*factor;
					
					}
					
					//	Use the height value to determine
					//	where between the floor and ceiling
					//	the column height shall be
					height=floor+((ceiling-floor)*height_val);
				
				}
				
				//	Determine what type of block
				//	to return
				Block retr=(
					(
						(type==Type::Beach) ||
						(type==Type::ContinentalShelf) ||
						(type==Type::Ocean)
					)
						//	Ocean logic
						?	(
								(id.Y>height)
									//	We're above this column's height --
									//	we're returning either water (if
									//	below sea level) or air (otherwise).
									?	(
											(id.Y>sea_level)
												?	air
												:	water
										)
									//	We're at or below the column's
									//	height -- we're returning either
									//	sand or dirt (if within a certain
									//	distance of the height) or stone
									//	(otherwise).
									:	(
											(id.Y<(height-surface_buffer))
												?	stone
												//	If the height is above
												//	sea level -- as in an
												//	island -- switch to using
												//	dirt
												:	(
														(height>sea_level)
															?	(
																	(id.Y==height)
																		?	grass
																		:	dirt
																)
															:	sand
													)
										)
							)
						//	Land logic
						:	(
								(id.Y>height)
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
													(id.Y==height)
														?	grass
														:	(
																(id.Y<(height-surface_buffer))
																	?	stone
																	:	dirt
															)
												)
							)			
				);
				
				retr.SetSkylight(15);
				retr.SetLight(15);
				
				return retr;
			
			}
			
			
			Biome operator () (Int32 x, Int32 z, SByte dimension) const noexcept {
			
				return Biome::Ocean;
			
			}
	
	
	};


}


static const String name("Default World Generator");
static const Word priority=2;
static const SByte dimensions []={0};


class DefaultGeneratorInstaller : public Module {

	
	private:
	
	
		Nullable<
			WorldGeneratorHelper<
				DefaultGenerator
			>
		> generator;
		
		
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

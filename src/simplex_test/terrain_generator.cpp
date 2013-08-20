/**
 *	\file
 */
 
 
#pragma once


#include <noise.hpp>
#include <random>
#include <type_traits>
#include <utility>


namespace MCPP {


	class TerrainGenerator {
	
	
		private:
		
		
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
			
			
			//	Determines a result based on a value's position
			//	relative to a series of thresholds.
			template <typename T, Word num>
			static inline Double threshold_value (
				const Double (& thresholds) [num],
				const T (& values) [num],
				Double val
			) noexcept {
			
				if (val<thresholds[0]) return values[0];
				
				for (Word i=1;i<num;++i) {
				
					if (val<thresholds[i]) {
					
						Word index=i-1;
						
						return fma(
							normalize(
								thresholds[index],
								thresholds[i],
								val
							),
							values[i]-values[index],
							values[index]
						);
					
					}
				
				}
				
				return values[num];
			
			}
		
		
			//	Constants
			
			//	Block types
			static constexpr Byte air=0;
			static constexpr Byte water=1;
			static constexpr Byte solid=2;
			
			//	Ocean noise filter constants
			static constexpr Word ocean_num_octaves=4;
			static constexpr Double ocean_persistence=0.7;
			static constexpr Double ocean_frequency=0.00005;
			
			//	Ceiling noise filter constants
			static constexpr Word max_num_octaves=4;
			static constexpr Double max_persistence=0.9;
			static constexpr Double max_frequency=0.0005;
			
			//	Floor noise filter constants
			static constexpr Word min_num_octaves=4;
			static constexpr Double min_persistence=0.5;
			static constexpr Double min_frequency=0.00005;
			static constexpr Double min_bias=0.25;
			
			//	Heightmap noise filter constants
			static constexpr Word heightmap_num_octaves=4;
			static constexpr Double heightmap_persistence=0.5;
			static constexpr Double heightmap_frequency=0.001;
			
			//	Rivers noise filter constants
			static constexpr Word rivers_num_octaves=4;
			static constexpr Double rivers_persistence=0.8;
			static constexpr Double rivers_frequency=0.0001;
			
			//	X perturbation noise filter constants
			static constexpr Word perturb_x_num_octaves=3;
			static constexpr Double perturb_x_persistence=0.4;
			static constexpr Double perturb_x_frequency=0.01;
			static constexpr Double perturb_x_max=20;
			static constexpr Double perturb_x_min=-20;
			static constexpr Double perturb_x_gain=0.25;
			
			//	Z perturbation noise filter constants
			static constexpr Word perturb_z_num_octaves=3;
			static constexpr Double perturb_z_persistence=0.4;
			static constexpr Double perturb_z_frequency=0.01;
			static constexpr Double perturb_z_max=20;
			static constexpr Double perturb_z_min=-20;
			static constexpr Double perturb_z_gain=0.25;
			
			//	Caves noise filter constants
			static constexpr Word caves_num_octaves=4;
			static constexpr Double caves_persistence=0.6;
			static constexpr Double caves_frequency=0.01;
			
			//	Cave Y input perturbation noise filter constants
			static constexpr Word cave_surface_num_octaves=4;
			static constexpr Double cave_surface_persistence=0.5;
			static constexpr Double cave_surface_frequency=0.05;
			static constexpr Double cave_surface_min=0;
			static constexpr Double cave_surface_max=18;
			
			//	World generation constants
			
			//	The height of sea level
			static constexpr Word sea_level=64;
			
			//	Oceans
			
			//	The threshold (between -1 and 1) below
			//	which continental shelves shall occur
			static constexpr Double continental_shelf_threshold=-0.0125;
			//	The threshold (between -1 and 1) below
			//	which the ocean proper shall occur
			static constexpr Double ocean_threshold=-0.025;
			//	The baseline cap for ocean height
			static constexpr Word ocean_ceiling=68;
			//	The baseline floor for ocean height
			static constexpr Word ocean_floor=48;
			//	The amount that the value from the
			//	ceiling noise generator should raise
			//	the ceiling when its output is at
			//	maximum (i.e. 1)
			static constexpr Word ocean_max_factor=16;
			//	The amount that the value from the
			//	floor noise generator should lower
			//	the floor when its output is at
			//	maximum (i.e. 1)
			static constexpr Word ocean_min_factor=16;
			
			//	Land
			
			//	Thresholds and height values which map
			//	max noise values to actual ceiling values
			static constexpr Double max_thresholds []={
				0.4,
				0.8
			};
			static constexpr Word max_values []={
				72,
				256
			};
			//	Thresholds and height values which map
			//	min noise values to actual floor values
			static constexpr Double min_thresholds []={
				0,
				0.2,
				0.6,
				0.9
			};
			static constexpr Word min_values []={
				52,
				64,
				64,
				128
			};
			//	The threshold below which beaches shall be
			//	formed
			static constexpr Double beach_threshold=0;
			//	The threshould above and equal to which
			//	land proper shall be formed
			static constexpr Double land_threshold=0.0125;
			
			//	Rivers
			
			//	The default threshould above and equal to which
			//	rivers and other related features may form.
			//
			//	This value is mutated based on the world's current
			//	overall height and proximity to oceans.  Closer to
			//	oceans rivers thicken.  As the world's overall height
			//	rises rivers thin.  As the world's overall height falls
			//	rivers thicken.
			static constexpr Double river_threshold=0.965;
			//	Within this proximity to river_threshould transitional
			//	features -- beaches and valley/canyon walls -- form
			static constexpr Double river_transition_threshold=0.07;
			//	Within this proximity to river_threshold+river_transition_threshold
			//	beaches form, with the remainder of the space between
			//	river_threshold and river_transitions threshold forming
			//	valley/canyon walls
			static constexpr Double river_beach_threshold=0.0025;
			//	As the overall height of the world decreases,
			//	rivers thicken.
			//
			//	At the lowest point possible, river_threshold is
			//	decreased by this amount
			static constexpr Double river_thicken_factor=0.06;
			//	As the overall height of the world increases,
			//	rivers thin.
			//
			//	At the highest point possible, river_threshold is
			//	increased by this amount.
			static constexpr Double river_thin_factor=0.035;
			//	As rivers run close to the ocean they thicken.
			//
			//	At the ocean-facing edge of the continental shelf
			//	river_threshold will be decreased by this amount.
			static constexpr Double river_delta_factor=0.25;
			//	As rivers run close to the ocean they thicken.
			//
			//	All ocean noise values below this value are
			//	considered "close to the ocean".
			static constexpr Double river_delta_threshold=0.04;
			//	The starting depth for rivers.
			static constexpr Double river_depth_start=8;
			//	The maximum depth a river may reach.
			static constexpr Double river_depth_max=24;
			//	The minimum depth a river may reach.
			static constexpr Double river_depth_min=0;
			
			//	Caves
			
			//	Between the natural roof of caves and the
			//	surface this many blocks of solid buffer
			//	are left.
			//
			//	The cave surface noise value allows caves
			//	to extend through this buffer, possibly even
			//	cutting holes in the surface
			static constexpr Word surface_buffer=10;
			//	At this proximity to bedrock caves will start
			//	thinning out, leaving at least 1 block between
			//	them and bedrock.
			static constexpr Word bedrock_buffer=3;
			//	As caves go deeper they thicken.
			//
			//	At the deepest possible point, cave_threshold
			//	will be decreased by this amount.
			static constexpr Double cave_depth_scalar=0.06;
			//	Determines when caves form.  For cave noise
			//	values above or equal to this value, caves
			//	are present, otherwise they are not.
			static constexpr Double cave_threshold=0.86;
			//	Caves will avoid breaking through the bottom
			//	of the ocean.  This is the number of blocks
			//	that will be left between the ocean bottom
			//	and the highest caves.
			static constexpr Word ocean_buffer=3;
			
		
		
			//	Simplex noise generators
			
			//	Generates noise which determines
			//	where oceans/continental shelves/
			//	beaches/land-to-ocean transitions
			//	occur
			Simplex ocean;
			//	Generates noise which determines
			//	the ceiling on the world's height
			//	(i.e. the highest that terrain may
			//	go)
			Simplex max;
			//	Generates noise which determines
			//	the floor on the world's height
			//	(i.e. the lowest that terrain may
			//	go)
			Simplex min;
			//	Generates noise which determines where
			//	between the floor and ceiling the terrain
			//	height shall actually be
			Simplex heightmap;
			//	Generates noise which determines where
			//	rivers shall go
			Simplex rivers;
			//	Generates noise which determines where
			//	caves form
			Simplex caves;
			//	Generates noise which perturbates the
			//	y input of the cave noise generator to:
			//
			//	A.	Provide a smooth fade out of caves
			//		near the surface.
			//	B.	Allow caves to breach the surface
			//		from time-to-time.
			Simplex cave_surface;
			//	Perturbates the x input of the noise
			//	generators
			Simplex perturbate_x;
			//	Perturbates the z input of the noise
			//	generators
			Simplex perturbate_z;
			
			
		public:
		
		
			template <typename T>
			TerrainGenerator (T & gen) noexcept(
				std::is_nothrow_constructible<
					Simplex,
					T &
				>::value
			)	:	ocean(gen),
					max(gen),
					min(gen),
					heightmap(gen),
					rivers(gen),
					caves(gen),
					cave_surface(gen),
					perturbate_x(gen),
					perturbate_z(gen)
			{	}
		
		
			UInt16 operator () (Int32 x, Byte y, Int32 z) const noexcept {
			
				//	Bottom layer is always solid
				if (y==0) return solid;
			
				//	Perturbate x and z values
				Double real_x=x+MakeScale(
					MakeGain(
						MakeOctave(
							perturbate_x,
							perturb_x_num_octaves,
							perturb_x_persistence,
							perturb_x_frequency
						),
						perturb_x_gain
					),
					perturb_x_min,
					perturb_x_max
				)(x,y,z);
				Double real_z=z+MakeScale(
					MakeGain(
						MakeOctave(
							perturbate_z,
							perturb_z_num_octaves,
							perturb_z_persistence,
							perturb_z_frequency
						),
						perturb_z_gain
					),
					perturb_z_min,
					perturb_z_max
				)(x,y,z);
				
				//	Obtain primary noise values
				Double ocean_val=MakeOctave(
					ocean,
					ocean_num_octaves,
					ocean_persistence,
					ocean_frequency
				)(real_x,real_z);
				Double max_val=MakeScale(
					MakeOctave(
						max,
						max_num_octaves,
						max_persistence,
						max_frequency
					),
					0,
					1
				)(real_x,real_z);
				Double min_val=MakeScale(
					MakeOctave(
						min,
						min_num_octaves,
						min_persistence,
						min_frequency
					),
					0,
					1
				)(real_x,real_z);
				Double height_val=MakeScale(
					MakeOctave(
						heightmap,
						heightmap_num_octaves,
						heightmap_persistence,
						heightmap_frequency
					),
					0,
					1
				)(real_x,real_z);
				
				//	The type of block shall be
				//	stored here -- solid/water/air
				Byte result;
				//	Cap of the world's height
				Double ceiling;
				//	Floor on the world's height
				Double floor;
				//	Actual cap used in terrain
				//	generation (may differ from
				//	ceiling because of transitions)
				Double real_ceiling;
				//	Actual floor used in terrain
				//	generation (may differ from
				//	floor because of transitions)
				Double real_floor;
				//	The height of the solid world
				Double height;
				//	Whether or not this is inside
				//	a river
				bool is_river=false;
				
				//	OCEAN OR LAND?
				if (ocean_val<continental_shelf_threshold) {
				
					//	OCEAN
					
					//	Determine ceiling and floor
					ceiling=fma(
						min_val,
						ocean_max_factor,
						ocean_ceiling
					);
					floor=fma(
						max_val,
						ocean_min_factor,
						ocean_floor
					);
					if (floor>ceiling) std::swap(floor,ceiling);
					
					//	To enable a smooth transition between
					//	land and ocean we have a "continental
					//	shelf" where the land/beach drop away
					//	into the ocean
					//
					//	Apply this if necessary
					if (ocean_val<ocean_threshold) {
					
						//	Ocean proper -- no transition
						
						real_floor=floor;
						real_ceiling=ceiling;
					
					} else {
					
						//	Continental shelf -- transition
						
						Double dampen=normalize(
							ocean_threshold,
							continental_shelf_threshold,
							ocean_val
						);
						real_floor=(dampen*(sea_level-floor))+floor;
						real_ceiling=(dampen*(sea_level-ceiling))+ceiling;
					
					}
					
					//	Determine current height
					height=fma(
						height_val,
						real_ceiling-real_floor,
						real_floor
					);
					
					//	Set result
					result=(y>height) ? ((y>sea_level) ? air : water) : solid;
				
				} else {
				
					//	LAND
					
					ceiling=threshold_value(
						max_thresholds,
						max_values,
						max_val
					);
					floor=threshold_value(
						min_thresholds,
						min_values,
						min_val
					);
					if (floor>ceiling) std::swap(floor,ceiling);
					
					//	The land must fall off smoothly
					//	towards beaches, so apply dampening
					//	if necessary
					if (
						(ocean_val>=beach_threshold) &&
						(ocean_val<land_threshold)
					) {
					
						Double dampen=normalize(
							land_threshold,
							beach_threshold,
							ocean_val
						);
						real_floor=(dampen*(sea_level-floor))+floor;
						real_ceiling=(dampen*(sea_level-ceiling))+ceiling;
					
					} else {
					
						real_floor=floor;
						real_ceiling=ceiling;
					
					}
					
					//	Determine current height
					//
					//	Beaches are always at sea
					//	level
					height=(
						(ocean_val<beach_threshold)
							?	sea_level
							:	fma(
									height_val,
									real_ceiling-real_floor,
									real_floor
								)
					);
					
					result=(y>height) ? air : solid;
				
				}
				
				//	RIVERS
				
				//	Rivers do not occur in the
				//	ocean
				if (ocean_val>=ocean_threshold) {
				
					//	Create a mutable, local copy
					//	of the threshold, so that we
					//	can alter it with certain factors
					Double rt=river_threshold;
					
					//	Use the average of the minimum,
					//	maximum, and height map to adjust
					//	the threshould
					Double avg=(max_val+min_val+height_val)/3;
					if (avg<0.5) rt-=avg*2*river_thicken_factor;
					else rt+=(avg-0.5)*2*river_thin_factor;
					
					//	Near oceans rivers widen (i.e. deltas)
					if (ocean_val<river_delta_threshold) rt-=normalize(
						river_delta_threshold,
						ocean_threshold,
						ocean_val
					)*river_delta_factor;
					
					//	Obtain river noise value
					Double rivers_val=MakeRidged(
						MakeOctave(
							rivers,
							rivers_num_octaves,
							rivers_persistence,
							rivers_frequency
						)
					)(real_x,real_z);
					
					//	Test for a river
					if (rivers_val>rt) {
					
						Double rtt=rt+river_transition_threshold;
						
						//	There needs to be a transition between
						//	regular terrain and a river, check which
						//	we're in
						if (rivers_val<rtt) {
						
							//	We're transitioning towards a river
							
							//	River transitions don't need to occur
							//	on beaches (which are already at
							//	sea level anyway) or in the ocean (which
							//	are already below sea level anyway)
							if ((ocean_val>=beach_threshold) && (height<sea_level)) {
							
								Double rbt=rtt-river_beach_threshold;
								
								//	River transition or river beach?
								if (rivers_val<rbt) {
								
									//	River transition
									
									//	"Pull" the world height towards
									//	sea level to meet up with the river
									height=((height-sea_level)*normalize(
										rbt,
										rt,
										rivers_val
									))+sea_level;
								
								} else {
								
									//	River beach
									
									height=sea_level;
									is_river=true;
								
								}
								
								//	Adjust result
								result=(y>height) ? air : solid;
							
							}
						
						} else {
						
							//	We're in a river
							
							//	Determine the river's depth
							
							Double river_depth=river_depth_start;
							
							//	We use the threshold to adjust the
							//	depth
							if (rt<river_threshold) {
							
								river_depth+=normalize(
									river_threshold,
									river_threshold-river_thicken_factor-river_delta_factor,
									rt
								)*(river_depth_max-river_depth_start);
							
							} else {
							
								river_depth-=normalize(
									river_threshold,
									1,
									rt
								)*(river_depth_start-river_depth_min);
							
							}
							
							//	Determine the "strength" of the noise
							//	based on how much it exceeds the threshold
							Double river_noise_strength=normalize(
								rtt,
								1,
								rivers_val
							);
							//	We use the "strength" value to obtain
							//	a nice curved river bottom
							//
							//	See graph of y=x^2 for 0<=x<=1
							river_depth*=river_noise_strength*river_noise_strength;
							Double river_height=sea_level-river_depth;
							
							height=(
								//	If we're in a continental shelf, we
								//	need to make the river "fade" into the
								//	ocean bottom
								(ocean_val<continental_shelf_threshold)
									?	river_height+(
											(height-river_height)*normalize(
												continental_shelf_threshold,
												ocean_threshold,
												ocean_val
											)
										)
									:	river_height
							);
							
							result=(y>sea_level) ? air : ((y>height) ? water : solid);
							is_river=true;
						
						}
					
					}
				
				}
				
				//	CAVES
				
				//	Caves do not occur above ground,
				//	so do not even do cave processing
				//	in those areas
				if (y<=height) {
				
					//	We obtain a new height value based
					//	on perturbation
					Double h=height+MakeScale(
						MakeOctave(
							cave_surface,
							cave_surface_num_octaves,
							cave_surface_persistence,
							cave_surface_frequency
						),
						cave_surface_min,
						cave_surface_max
					)(real_x,real_z)-surface_buffer;
					
					//	Avoid the bottom of bodies of
					//	water
					Word ocean_max_height;
					if (
						(
							(ocean_val<beach_threshold) ||
							is_river
						) &&
						(static_cast<Word>(h)>=(ocean_max_height=static_cast<Word>(height)-ocean_buffer))
					) h=ocean_max_height;
					
					//	Only proceed if its low enough
					//	for caves to form
					if (y<h) {
				
						//	Obtain cave noise value
						auto cave_val=MakeRidged(
							MakeOctave(
								caves,
								caves_num_octaves,
								caves_persistence,
								caves_frequency
							)
						)(real_x,y,real_z);
					
						//	Create a local, mutable copy
						//	of the cave selection threshold
						Double ct=cave_threshold;
						
						//	Determine the y height at which
						//	caves entirely vanish due to
						//	proximity to bedrock
						Word bbh=bedrock_buffer+1;
						
						//	Caves thin as they near bedrock
						if (y>bbh) ct-=normalize(
							h,
							bbh,
							y
						)*cave_depth_scalar;
						//	Caves thicken as they go deeper
						else ct+=normalize(
							bbh,
							2,
							y
						)*(1-cave_threshold);
						
						if (cave_val>=ct) result=air;
					
					}
				
				}
				
				return result;
			
			}
	
	
	};
	
	
	constexpr Double TerrainGenerator::max_thresholds [];
	constexpr Double TerrainGenerator::min_thresholds [];
	constexpr Word TerrainGenerator::min_values [];
	constexpr Word TerrainGenerator::max_values [];


}

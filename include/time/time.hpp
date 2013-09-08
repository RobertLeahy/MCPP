#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <server.hpp>
#include <atomic>
#include <functional>


namespace MCPP {


	/**
	 *	Contains all possible phases of
	 *	the moon.
	 */
	enum class LunarPhase {
	
		Full=0,
		WaningGibbous=1,
		LastQuarter=2,
		WaningCrescent=3,
		New=4,
		WaxingCrescent=5,
		FirstQuarter=6,
		WaxingGibbous=7
	
	};
	
	
	/**
	 *	Specifies the different times of
	 *	day.
	 */
	enum class TimeOfDay {
	
		Day,
		Dusk,
		Night,
		Dawn
	
	};


	/**
	 *	Keeps track of the time of day.
	 */
	class TimeModule : public Module {
	
	
		private:
		
		
			mutable Mutex lock;
			
			
			//	Age of the world
			UInt64 age;
			//	Time of day
			UInt64 time;
			//	Time since last save
			UInt64 since;
			//	If true time stops when
			//	no one is logged in
			bool offline_freeze;
			//	Ticks between saves
			UInt64 between_saves;
			//	Number if milliseconds per
			//	tick
			Word tick_length;
			
			
			void tick ();
			void save (UInt64, UInt64) const;
			LunarPhase get_lunar_phase () const noexcept;
			UInt64 get_time (bool) const noexcept;
			TimeOfDay get_time_of_day () const noexcept;
			
			
		public:
		
		
			/**
			 *	Retrieves a string which describes
			 *	a phase of the moon.
			 *
			 *	\param [in] phase
			 *		A phase of the moon.
			 *
			 *	\return
			 *		A string which describes \em phase.
			 */
			static const String & GetLunarPhase (LunarPhase phase) noexcept;
			/**
			 *	Retrieves a string which describes
			 *	a time of day.
			 *
			 *	\param [in] time_of_day
			 *		A time of day.
			 *
			 *	\return
			 *		A string which describes \em time_of_day.
			 */
			static const String & GetTimeOfDay (TimeOfDay time_of_day) noexcept;
		
		
			TimeModule () noexcept;
			
			
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	Determines the phase the moon is
			 *	currently in.
			 *
			 *	\return
			 *		The current phase of the moon.
			 */
			LunarPhase GetLunarPhase () const noexcept;
			/**
			 *	Retrieves the current time.
			 *
			 *	\param [in] total
			 *		If \em true the number of ticks
			 *		by which in game time has been
			 *		displaced from the start of this
			 *		world.  If \em false the number
			 *		of ticks since midnight.  Defaults
			 *		to \em false.
			 *
			 *	\return
			 *		The current time.
			 */
			UInt64 GetTime (bool total=false) const noexcept;
			/**
			 *	Retrieves the current time of day.
			 *
			 *	\return
			 *		The current time of day.
			 */
			TimeOfDay GetTimeOfDay () const noexcept;
			/**
			 *	Retrieves the age of the world.
			 *
			 *	\return
			 *		The number of ticks that have
			 *		passed since this world began.
			 */
			UInt64 GetAge () const noexcept;
			/**
			 *	Gets all time-related information
			 *	atomically.
			 *
			 *	\return
			 *		A tuple containing the current time,
			 *		the number of ticks since it was
			 *		last midnight, the current lunar phase,
			 *		the current time of day, and the current
			 *		world's age, respectively.
			 */
			Tuple<UInt64,UInt64,LunarPhase,TimeOfDay,UInt64> Get () const noexcept;
			
			
			/**
			 *	Adds a certain number of ticks to the
			 *	current time.
			 *
			 *	The age of the world is unaffected.
			 *
			 *	\param [in] ticks
			 *		The number of ticks to add to
			 *		the current time.
			 */
			void Add (UInt64 ticks) noexcept;
			/**
			 *	Subtracts a certain number of ticks
			 *	from the current time.
			 *
			 *	If the current time would underflow
			 *	as a result of this subtraction, the
			 *	time is set to the smallest value which
			 *	causes the time to have the same displacement
			 *	from midnight as the subtraction would
			 *	have caused.
			 *
			 *	The age of the world is unaffected.
			 *
			 *	\param [in] ticks
			 *		The number of ticks to subtract from
			 *		the current time.
			 */
			void Subtract (UInt64 ticks) noexcept;
			/**
			 *	Sets the time to a certain value.
			 *
			 *	\param [in] time
			 *		The time to set.
			 *	\param [in] total
			 *		If \em true the time will be set
			 *		to exactly \em time, otherwise
			 *		the time shall be adjusted so that
			 *		the difference between midinght
			 *		and the current time is the same
			 *		as the difference between midnight
			 *		and \em time.  Defaults to \em false.
			 */
			void Set (UInt64 time, bool total=false) noexcept;
			
			
			/**
			 *	Causes the age of the world and
			 *	time of day to immediately be
			 *	saved to the backing store.
			 */
			void Save () const;
	
	
	};
	
	
	/**
	 *	The single valid instance of TimeModule.
	 */
	extern Nullable<TimeModule> Time;


}

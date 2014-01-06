/**
 *	\file
 */


#pragma once


#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <multi_scope_guard.hpp>
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
	 *	Information about the time module.
	 */
	class TimeInfo {
	
	
		public:
		
		
			/**
			 *	The current number of ticks that
			 *	the game time is displaced from
			 *	the start of the world.
			 */
			UInt64 Current;
			/**
			 *	The current number of ticks that
			 *	the game time is displaced from
			 *	midnight.
			 */
			UInt64 SinceMidnight;
			/**
			 *	The current age of the world, in
			 *	ticks.
			 *
			 *	This is identical to \em Current,
			 *	except that this value is not
			 *	modified when the time is set.
			 */
			UInt64 Age;
			/**
			 *	The current phase of the moon.
			 */
			LunarPhase Phase;
			/**
			 *	The current time of day.
			 */
			TimeOfDay DayPart;
			/**
			 *	The number of ticks that the time
			 *	module has simulated since the
			 *	server was last restarted.
			 */
			Word Elapsed;
			/**
			 *	The total number of nanoseconds the
			 *	server has spent in a tick.
			 */
			UInt64 Total;
			/**
			 *	The total number of nanoseconds the
			 *	server has spent simulating ticks.
			 */
			UInt64 Executing;
			/**
			 *	The current tick length in milliseconds.
			 */
			Word Length;
	
	
	};


	/**
	 *	Keeps track of the time of day.
	 */
	class Time : public Module {
	
	
		private:
		
		
			class Callback {
			
			
				private:
				
				
					std::function<void (MultiScopeGuard)> callback;
					bool wait;
			
				
				public:
				
				
					Callback () = delete;
					Callback (
						std::function<void (MultiScopeGuard)>,
						bool
					) noexcept;
					
					
					void operator () (const MultiScopeGuard &) const;
				
			
			};
		
		
			//	Lock for age/time
			mutable Mutex lock;
			//	Lock for callbacks
			mutable Mutex callbacks_lock;
			
			
			//	Time since last tick
			Timer timer;
			//	Number of ticks
			std::atomic<Word> ticks;
			//	Time spent ticking
			std::atomic<UInt64> tick_time;
			//	Time spent executing ticks
			std::atomic<UInt64> executing;
			
			
			//	Age of the world
			UInt64 age;
			//	Time of day
			UInt64 time;
			//	If true time stops when
			//	no one is logged in
			bool offline_freeze;
			//	Number of milliseconds per
			//	tick
			Word tick_length;
			//	Threshold above which a tick
			//	is considered "too long"
			Word threshold;
			
			
			//	Tasks to be executed every
			//	tick
			Vector<Callback> callbacks;
			//	Tasks to be executed at a certain
			//	point in time
			Vector<Tuple<Word,Callback>> scheduled_callbacks;
			
			
			void too_long (UInt64);
			bool do_tick () const noexcept;
			void tick ();
			LunarPhase get_lunar_phase () const noexcept;
			UInt64 get_time (bool) const noexcept;
			TimeOfDay get_time_of_day () const noexcept;
			
			
		public:
		
		
			/**
			 *	Retrieves a reference to a valid instance
			 *	of this class.
			 *
			 *	\return
			 *		A reference to a valid instance of
			 *		this class.
			 */
			static Time & Get () noexcept;
		
		
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
			
			
			/**
			 *	\cond
			 */
		
		
			Time () noexcept;
			
			
			virtual const String & Name () const noexcept override;
			virtual Word Priority () const noexcept override;
			virtual void Install () override;
			
			
			/**
			 *	\endcond
			 */
			
			
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
			TimeInfo GetInfo () const noexcept;
			
			
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
			 *	Enqueues a task to be asynchronously
			 *	invoked each tick.
			 *
			 *	\tparam T
			 *		The type of callback which shall be
			 *		invoked each tick.
			 *	\tparam Args
			 *		The types of the arguments which shall
			 *		be passed through to the callback of
			 *		type \em T.
			 *
			 *	\param [in] wait
			 *		If \em true each tick shall be
			 *		delayed until this task completes.
			 *	\param [in] callback
			 *		The callback which shall be invoked
			 *		each tick.
			 *	\param [in] args
			 *		The arguments which shall be forwarded
			 *		through to \em callback.
			 */
			template <typename T, typename... Args>
			void Enqueue (bool wait, T && callback, Args &&... args) {
			
				//	Manually locking etc. because GCC
				//	has problems capturing parameter packs
				callbacks_lock.Acquire();
				
				try {
				
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wpedantic"
					callbacks.EmplaceBack(
						[wrapped=std::bind(
							std::forward<T>(callback),
							std::forward<Args>(args)...
						)] (MultiScopeGuard) {
						
							wrapped();
						
						},
						wait
					);
					#pragma GCC diagnostic pop
				
				} catch (...) {
				
					callbacks_lock.Release();
					
					throw;
				
				}
				
				callbacks_lock.Release();
			
			}
			
			
			/**
			 *	Enqueues a task to be asynchronously invoked
			 *	after a certain number of ticks have passed.
			 *
			 *	\tparam T
			 *		The type of callback which shall be
			 *		invoked.
			 *	\tparam Args
			 *		The types of the arguments which shall be
			 *		passed through to the callback of type
			 *		\em T.
			 *
			 *	\param [in] ticks
			 *		The number of ticks after which \em callback
			 *		shall be invoked.
			 *	\param [in] wait
			 *		If \em true the tick after this task is
			 *		executed shall wait until the task completes.
			 *	\param [in] callback
			 *		The callback that shall be invoked after \em ticks
			 *		ticks.
			 *	\param [in] args
			 *		The arguments that shall be forwarded through to
			 *		\em callback.
			 */
			template <typename T, typename... Args>
			void Enqueue (Word ticks, bool wait, T && callback, Args &&... args) {
			
				//	Deduce execution time
				Word when=lock.Execute([&] () {	return age;	})+ticks;
				
				//	Manual locking because GCC has problems
				//	capturing parameter packs
				callbacks_lock.Acquire();
				
				try {
				
					//	Find insertion point
					Word loc=0;
					for (
						;
						(loc<scheduled_callbacks.Count()) &&
						(scheduled_callbacks[loc].Item<0>()<when);
						++loc
					);
					
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wpedantic"
					scheduled_callbacks.Emplace(
						loc,
						when,
						Callback(
							[wrapped=std::bind(
								std::forward<T>(callback),
								std::forward<Args>(args)...
							)] (MultiScopeGuard) {
							
								wrapped();
							
							},
							wait
						)
					);
					#pragma GCC diagnostic pop
				
				} catch (...) {
				
					callbacks_lock.Release();
					
					throw;
				
				}
				
				callbacks_lock.Release();
			
			}
			
	
	
	};


}

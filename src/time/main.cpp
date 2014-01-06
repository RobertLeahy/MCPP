#include <save/save.hpp>
#include <time/time.hpp>
#include <mod.hpp>
#include <serializer.hpp>
#include <server.hpp>
#include <singleton.hpp>
#include <utility>


using namespace MCPP;


namespace MCPP {


	Time::Callback::Callback (std::function<void (MultiScopeGuard)> callback, bool wait) noexcept : callback(std::move(callback)), wait(wait) {	}
	
	
	void Time::Callback::operator () (const MultiScopeGuard & sg) const {
	
		auto & pool=Server::Get().Pool();
		
		if (wait) pool.Enqueue(
			callback,
			sg
		);
		else pool.Enqueue(
			callback,
			MultiScopeGuard()
		);
	
	}


	static const String name("Tick Manager");
	static const Word priority=1;
	static const Word default_tick_length=50;
	static const Word default_too_long=60;
	static const bool default_offline_freeze=true;
	static const String tick_length_key("tick_length");
	static const String too_long_key("tick_length_threshold");
	static const String offline_freeze_key("offline_freeze");
	static const String save_key("time");
	static const String parse_error("Error parsing time: \"{0}\" at byte {1}");
	static const String took_too_long("Tick took {0}% too long (expected {1}ms, actual {2}ms)");
	static const Word day_length=24000;
	static const Word zero=6000;
	static const Word lunar_phases=8;
	
	
	void Time::too_long (UInt64 elapsed) {
	
		Server::Get().WriteLog(
			String::Format(
				took_too_long,
				((static_cast<Double>(elapsed)/static_cast<Double>(tick_length))*100)-100,
				tick_length,
				elapsed
			),
			Service::LogType::Warning
		);
	
	}
	
	
	bool Time::do_tick () const noexcept {
	
		if (!offline_freeze) return true;
		
		for (auto & client : Server::Get().Clients) if (client->GetState()==ProtocolState::Play) return true;
		
		return false;
	
	}
	
	
	void Time::tick () {
	
		Server::Get().PanicOnThrow([&] () mutable {
	
			//	End the previous tick
			auto elapsed=timer.ElapsedMilliseconds();
			++ticks;
			tick_time+=elapsed;
			//	Start timing the next tick
			timer=Timer::CreateAndStart();
			
			//	Determine whether we'll simulate the
			//	next tick
			auto simulate=do_tick();
			
			//	Was the previous tick "too long"?
			//
			//	Ignore if we're not simulating
			if (simulate && (elapsed>threshold)) too_long(elapsed);
			
			//	Prepare a multi scope guard, which
			//	prevents the next tick from occurring
			//	until all tasks that require the next
			//	tick to wait have completed
			MultiScopeGuard sg(
				[this] () mutable {
				
					//	How long has this tick been going on?
					auto elapsed=timer.ElapsedMilliseconds();
					executing+=elapsed;
					
					auto & pool=Server::Get().Pool();
					auto lambda=[this] () mutable {	tick();	};
					
					//	If this tick has already taken too long,
					//	execute the next tick at once
					if (elapsed>=tick_length) pool.Enqueue(std::move(lambda));
					else pool.Enqueue(
						tick_length-elapsed,
						std::move(lambda)
					);
				
				},
				std::function<void ()>(),
				[] () {
				
					try {
					
						Server::Get().Panic();
						
					} catch (...) {	}
				
				}
			);
			
			//	Do not continue if no one is online and
			//	we freeze time when no one is online
			if (!simulate) return;
			
			//	Increment time
			lock.Execute([&] () mutable {
			
				++age;
				++time;
			
			});
			
			//	Execute callbacks
			callbacks_lock.Execute([&] () mutable {
			
				for (auto & callback : callbacks) callback(sg);
				
				while (
					(scheduled_callbacks.Count()!=0) &&
					(scheduled_callbacks[0].Item<0>()<=age)
				) {
				
					auto t=std::move(scheduled_callbacks[0]);
					scheduled_callbacks.Delete(0);
					
					t.Item<1>()(sg);
				
				}
			
			});
			
		});
	
	}
	
	
	LunarPhase Time::get_lunar_phase () const noexcept {
	
		return static_cast<LunarPhase>(((time+zero)%(day_length*lunar_phases))/day_length);
	
	}
	
	
	UInt64 Time::get_time (bool total) const noexcept {
	
		return total ? time : ((time+zero)%day_length);
	
	}
	
	
	TimeOfDay Time::get_time_of_day () const noexcept {
	
		auto time=get_time(false);
	
		if (
			(time<4200) ||
			(time>=19800)
		) return TimeOfDay::Night;
		
		if (time<6000) return TimeOfDay::Dawn;
		
		if (time<18000) return TimeOfDay::Day;
		
		return TimeOfDay::Dusk;
	
	}


	static Singleton<Time> singleton;


	Time & Time::Get () noexcept {
	
		return singleton.Get();
	
	}


	static const String full_moon("Full");
	static const String waning_gibbous_moon("Waning Gibbous");
	static const String last_quarter_moon("Last Quarter");
	static const String waning_crescent_moon("Waning Crescent");
	static const String new_moon("New");
	static const String waxing_crescent_moon("Waxing Crescent");
	static const String first_quarter_moon("First Quarter");
	static const String waxing_gibbous_moon("Waxing Gibbous");
	
	
	const String & Time::GetLunarPhase (LunarPhase phase) noexcept {
	
		switch (phase) {
		
			case LunarPhase::Full:
			default:return full_moon;
			
			case LunarPhase::WaningGibbous:return waning_gibbous_moon;
			
			case LunarPhase::LastQuarter:return last_quarter_moon;
			
			case LunarPhase::WaningCrescent:return waning_crescent_moon;
			
			case LunarPhase::New:return new_moon;
			
			case LunarPhase::WaxingCrescent:return waxing_crescent_moon;
			
			case LunarPhase::FirstQuarter:return first_quarter_moon;
			
			case LunarPhase::WaxingGibbous:return waxing_gibbous_moon;
		
		}
	
	}
	
	
	static const String day("Day");
	static const String dusk("Dusk");
	static const String dawn("Dawn");
	static const String night("Night");
	
	
	const String & Time::GetTimeOfDay (TimeOfDay time_of_day) noexcept {
	
		switch (time_of_day) {
		
			case TimeOfDay::Day:
			default:return day;
			
			case TimeOfDay::Dusk:return dusk;
			
			case TimeOfDay::Night:return night;
			
			case TimeOfDay::Dawn:return dawn;
		
		}
	
	}


	Time::Time () noexcept : age(0), time(0) {
	
		ticks=0;
		tick_time=0;
		executing=0;
	
	}
	
	
	const String & Time::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word Time::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void Time::Install () {
	
		auto & server=Server::Get();
	
		//	Load settings from the backing
		//	store
		auto & data=server.Data();
		offline_freeze=data.GetSetting(
			offline_freeze_key,
			default_offline_freeze
		);
		tick_length=data.GetSetting(
			tick_length_key,
			default_tick_length
		);
		threshold=data.GetSetting(
			too_long_key,
			default_too_long
		);
		
		//	Attempt to load time/age of world
		
		auto buffer=ByteBuffer::Load(save_key);
		if (buffer.Count()!=0) {
		
			Word age;
			Word time;
			try {
			
				age=buffer.FromBytes<Word>();
				time=buffer.FromBytes<Word>();
			
			} catch (const ByteBufferError & e) {
			
				server.WriteLog(
					String::Format(
						parse_error,
						e.what(),
						e.Where()
					),
					Service::LogType::Error
				);
			
			}
			
			this->age=age;
			this->time=time;
		
		}
		
		//	Hook into the save system
		SaveManager::Get().Add([this] () {
		
			ByteBuffer buffer;
		
			lock.Execute([&] () {
			
				buffer.ToBytes(age);
				buffer.ToBytes(time);
			
			});
			
			buffer.Save(save_key);
		
		});
		
		//	Start the tick loop
		
		timer=Timer::CreateAndStart();
		
		server.Pool().Enqueue(
			tick_length,
			[this] () mutable {	tick();	}
		);
	
	}
	
	
	LunarPhase Time::GetLunarPhase () const noexcept {
	
		return lock.Execute([&] () {	return get_lunar_phase();	});
	
	}
	
	
	UInt64 Time::GetTime (bool total) const noexcept {
	
		return lock.Execute([&] () {	return get_time(total);	});
	
	}
	
	
	TimeOfDay Time::GetTimeOfDay () const noexcept {
	
		return lock.Execute([&] () {	return get_time_of_day();	});
	
	}
	
	
	UInt64 Time::GetAge () const noexcept {
	
		return lock.Execute([&] () {	return age;	});
	
	}
	
	
	TimeInfo Time::GetInfo () const noexcept {
	
		TimeInfo retr;
		
		lock.Execute([&] () {
		
			retr.Current=get_time(true);
			retr.SinceMidnight=get_time(false);
			retr.Age=age;
			retr.Phase=get_lunar_phase();
			retr.DayPart=get_time_of_day();
		
		});
		
		retr.Elapsed=ticks;
		retr.Total=tick_time;
		retr.Executing=executing;
		retr.Length=tick_length;
		
		return retr;
	
	}
	
	
	void Time::Add (UInt64 ticks) noexcept {
	
		lock.Execute([&] () mutable {	time+=ticks;	});
	
	}
	
	
	void Time::Subtract (UInt64 ticks) noexcept {
	
		lock.Execute([&] () mutable {	time-=ticks;	});
	
	}
	
	
	void Time::Set (UInt64 time, bool total) noexcept {
	
		//	Set the absolute time
		if (total) {
		
			lock.Execute([&] () mutable {	this->time=time;	});
			
			return;
		
		}
		
		//	Set the time since midnight
		
		//	Get the offset since midnight
		//	referred to by the given parameter
		time%=day_length;
		
		lock.Execute([&] () mutable {
		
			//	Determine the offset from midnight
			//	of the current time
			UInt64 offset=this->time%day_length;
			
			//	Adjust curret time so that the
			//	offset from midnight is the same
			if (offset<time) this->time+=time-offset;
			else this->time-=offset-time;
		
		});
	
	}


}


extern "C" {


	Module * Load () {
	
		return &(Time::Get());
	
	}
	
	
	void Unload () {
	
		singleton.Destroy();
	
	}


}

#include <time/time.hpp>
#include <packet.hpp>
#include <client.hpp>
#include <data_provider.hpp>
#include <thread_pool.hpp>
#include <cstddef>


namespace MCPP {


	class TimeSave {
	
	
		public:
		
		
			UInt64 Age;
			UInt64 Time;
	
	
	};
	
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Winvalid-offsetof"
	static_assert(
		offsetof(TimeSave,Time)==(offsetof(TimeSave,Age)+sizeof(UInt64)),
		"TimeSave layout incorrect"
	);
	#pragma GCC diagnostic pop


	static const String name("Time Module");
	static const Word priority=1;
	
	
	static const Word default_tick_length=50;
	static const Word day_length=24000;
	static const Word default_between_saves=5*60*20;	//	5 minutes
	static const bool default_offline_freeze=true;
	static const Word num_lunar_phases=8;
	
	
	static const String time_key("time");
	static const String tick_length_key("tick_length");
	static const String offline_freeze_key("offline_freeze");
	static const String between_saves_key("maintenance_interval");
	static const String tick_error("Error while ticking");
	static const Regex true_regex(
		"^\\s*(?:t(?:rue)?|y(?:es)?)\\s*$",
		RegexOptions().SetIgnoreCase()
	);
	static const Regex false_regex(
		"^\\s*(?:f(?:alse)?|n(?:o)?)\\s*$",
		RegexOptions().SetIgnoreCase()
	);
	
	
	static const String full_moon("Full");
	static const String waning_gibbous_moon("Waning Gibbous");
	static const String last_quarter_moon("Last Quarter");
	static const String waning_crescent_moon("Waning Crescent");
	static const String new_moon("New");
	static const String waxing_crescent_moon("Waxing Crescent");
	static const String first_quarter_moon("First Quarter");
	static const String waxing_gibbous_moon("Waxing Gibbous");
	
	
	const String & TimeModule::GetLunarPhase (LunarPhase phase) noexcept {
	
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
	
	
	const String & TimeModule::GetTimeOfDay (TimeOfDay time_of_day) noexcept {
	
		switch (time_of_day) {
		
			case TimeOfDay::Day:
			default:return day;
			
			case TimeOfDay::Dusk:return dusk;
			
			case TimeOfDay::Night:return night;
			
			case TimeOfDay::Dawn:return dawn;
		
		}
	
	}


	TimeModule::TimeModule () noexcept {
	
		age=0;
		time=0;
		since=0;
	
	}
	
	
	const String & TimeModule::Name () const noexcept {
	
		return name;
	
	}
	
	
	Word TimeModule::Priority () const noexcept {
	
		return priority;
	
	}
	
	
	void TimeModule::Install () {
	
		//	Retrieve settings from the backing
		//	store
		
		//	Time between ticks
		auto tick_length=RunningServer->Data().GetSetting(tick_length_key);
		if (
			tick_length.IsNull() ||
			!tick_length->ToInteger(&(this->tick_length)) ||
			(this->tick_length==0)
		) this->tick_length=default_tick_length;
		
		//	Time between saves
		auto between_saves=RunningServer->Data().GetSetting(between_saves_key);
		if (
			between_saves.IsNull() ||
			!between_saves->ToInteger(&(this->between_saves)) ||
			(this->between_saves==0)
		) this->between_saves=default_between_saves;
		else this->between_saves=this->between_saves/this->tick_length;
		
		//	Offline freeze
		auto offline_freeze=RunningServer->Data().GetSetting(offline_freeze_key);
		this->offline_freeze=(
			offline_freeze.IsNull()
				?	default_offline_freeze
				:	(
						true_regex.IsMatch(*offline_freeze)
							?	true
							:	(
									false_regex.IsMatch(*offline_freeze)
										?	false
										:	default_offline_freeze
								)
					)
		);
		
		//	Attempt to load age and time
		TimeSave save;
		Word len=sizeof(save);
		if (
			RunningServer->Data().GetBinary(
				time_key,
				&save,
				&len
			) &&
			(len==sizeof(save))
		) {
		
			//	Load "hit"
			
			age=save.Age;
			time=save.Time;
		
		}
	
		//	Start the tick loop
		RunningServer->OnInstall.Add(
			[this] (bool before) {
			
				if (!before) {
				
					RunningServer->Pool().Enqueue(
						this->tick_length,
						[this] () {	tick();	}
					);
					
				}
			
			}
		);
	
	}
	
	
	LunarPhase TimeModule::get_lunar_phase () const noexcept {
		
		//	Determine the phase of the
		//	lunar cycle we're in by
		//	first determining the number
		//	of ticks we are into this
		//	lunar cycle, and then using
		//	that to determine how many
		//	days we are into this lunar
		//	cycle
		return static_cast<LunarPhase>((time%(day_length*num_lunar_phases))/day_length);
	
	}
	
	
	UInt64 TimeModule::get_time (bool total) const noexcept {
		
		//	If we're getting the total,
		//	return exactly the current
		//	time
		if (total) return time;
		
		//	Otherwise return the number
		//	of ticks since midnight
		return time%day_length;
	
	}
	
	
	TimeOfDay TimeModule::get_time_of_day () const noexcept {
		
		if (
			(time<4200) ||
			(time>=19800)
		) return TimeOfDay::Night;
		
		if (time<6000) return TimeOfDay::Dawn;
		
		if (time<18000) return TimeOfDay::Day;
		
		return TimeOfDay::Dusk;
	
	}
	
	
	LunarPhase TimeModule::GetLunarPhase () const noexcept {
	
		return lock.Execute([&] () {	return get_lunar_phase();	});
	
	}
	
	
	UInt64 TimeModule::GetTime (bool total) const noexcept {
	
		return lock.Execute([&] () {	return get_time(total);	});
	
	}
	
	
	TimeOfDay TimeModule::GetTimeOfDay () const noexcept {
	
		return lock.Execute([&] () {	return get_time_of_day();	});
	
	}
	
	
	UInt64 TimeModule::GetAge () const noexcept {
	
		return lock.Execute([&] () {	return age;	});
	
	}
	
	
	Tuple<UInt64,UInt64,LunarPhase,TimeOfDay,UInt64> TimeModule::Get () const noexcept {
	
		return lock.Execute([&] () {
		
			return Tuple<UInt64,UInt64,LunarPhase,TimeOfDay,UInt64>(
				get_time(true),
				get_time(false),
				get_lunar_phase(),
				get_time_of_day(),
				age
			);
		
		});
	
	}
	
	
	void TimeModule::Add (UInt64 ticks) noexcept {
	
		lock.Acquire();
		time+=ticks;
		lock.Release();
	
	}
	
	
	void TimeModule::Subtract (UInt64 ticks) noexcept {
	
		lock.Acquire();
		//	If we'd cause an underflow, add enough
		//	days to the time not to
		if (ticks>time) time+=((ticks/day_length)+1)*day_length;
		time-=ticks;
		lock.Release();
	
	}
	
	
	void TimeModule::Set (UInt64 time, bool total) noexcept {
	
		if (total) {
		
			//	Set time exactly
		
			lock.Acquire();
			this->time=time;
			lock.Release();
		
		} else {
		
			//	Set elapsed time since
			//	midnight
		
			//	Determine the offset from
			//	midnight to which the parameter
			//	refers
			time%=day_length;
			
			lock.Acquire();
			
			//	Determine the offset from
			//	midnight of the current time
			UInt64 offset=this->time%day_length;
			
			//	Adjust current time so that
			//	the offset from midnight is
			//	the same
			if (offset<time) this->time+=time-offset;
			else this->time-=offset-time;
			
			lock.Release();
			
		}
	
	}
	
	
	void TimeModule::Save () const {
	
		lock.Acquire();
		UInt64 age=this->age;
		UInt64 time=this->time;
		lock.Release();
		
		save(age,time);
	
	}
	
	
	void TimeModule::save (UInt64 age, UInt64 time) const {
	
		TimeSave save{
			age,
			time
		};
		
		RunningServer->Data().SaveBinary(
			time_key,
			&save,
			sizeof(save)
		);
	
	}
	
	
	void TimeModule::tick () {
	
		try {
		
			if (!(
				offline_freeze &&
				(RunningServer->Clients.AuthenticatedCount()==0)
			)) {
			
				//	We're ticking
				
				//	Increment
				lock.Acquire();
				UInt64 age=++this->age;
				UInt64 time=++this->time;
				lock.Release();
				
				//	Send tick packet
				Packet packet;
				packet.SetType<PacketTypeMap<0x04>>();
				packet.Retrieve<UInt64>(0)=age;
				packet.Retrieve<UInt64>(1)=time;
				
				//	Send to all clients
				RunningServer->Clients.Scan([&] (SmartPointer<Client> & client) {
				
					if (client->GetState()==ClientState::Authenticated) client->Send(packet);
				
				});
				
			}
			
			//	It's been one more tick
			//	since we last saved the
			//	age and time to the backing
			//	store
			if ((++since)==between_saves) {
			
				//	We must save
				
				since=0;
				
				save(age,time);
			
			}
			
			//	Wait for the next tick
			RunningServer->Pool().Enqueue(
				tick_length,
				[this] () {	tick();	}
			);
		
		} catch (...) {
		
			try {
			
				RunningServer->WriteLog(
					tick_error,
					Service::LogType::Error
				);
			
			} catch (...) {	}
		
			RunningServer->Panic();
		
			throw;
		
		}
	
	}
	
	
	Nullable<TimeModule> Time;


}


extern "C" {


	Module * Load () {
	
		if (Time.IsNull()) Time.Construct();
		
		return &(*Time);
	
	}
	
	
	void Unload () {
	
		Time.Destroy();
	
	}


}

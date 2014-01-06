#include <rleahylib/rleahylib.hpp>
#include <chat/chat.hpp>
#include <info/info.hpp>
#include <time/time.hpp>
#include <mod.hpp>


using namespace MCPP;


static const String identifier("time");
static const String name("Time Information");
static const String help("Displays the current time- and tick-related information.");
static const Word priority=1;
static const String time_banner("TIME/TICKS");
static const String time_template("{0}:{1}");
static const String current_time("Time: {0} (in ticks since beginning: {1})");
static const String time_of_day("Time of Day: {0}");
static const String lunar_phase("Phase of the Moon: {0}");
static const String length("Tick Length: {0}ms");
static const String age("Age of the World: {0}");
static const String ticks("Elapsed Ticks: {0}");
static const String ticking("Ticking: {0}ms (average {1}ms)");
static const String executing("Executing: {0}ms (average {1}ms)");


class DisplayTime : public Module, public InformationProvider {


	private:
	
	
		template <typename T>
		static T avg (T num, Word count) noexcept {
		
			return (count==0) ? 0 : static_cast<T>(num/count);
		
		}
	
	
		static String zero_pad (String str, Word num) {
		
			while (str.Count()<num) str=String("0")+str;
			
			return str;
		
		}
	
	
		static String format_time (UInt64 time) {
		
			UInt64 hours=time/1000;
			UInt64 minutes=static_cast<UInt64>(
				(static_cast<Double>(time%1000)/1000)*60
			);
			
			return String::Format(
				time_template,
				zero_pad(hours,2),
				zero_pad(minutes,2)
			);
		
		}
	
	
	public:
	
	
		virtual const String & Name () const noexcept override {
		
			return name;
		
		}
		
		
		virtual Word Priority () const noexcept override {
		
			return priority;
		
		}
		
		
		virtual void Install () override {
		
			Information::Get().Add(this);
		
		}
		
		
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual void Execute (ChatMessage & message) const override {
		
			auto info=Time::Get().GetInfo();
			
			message	<<	ChatStyle::Bold
					<<	time_banner
					<<	ChatFormat::Pop
					<<	Newline
					<<	String::Format(
							current_time,
							format_time(info.SinceMidnight),
							info.Current
						)
					<<	Newline
					<<	String::Format(
							time_of_day,
							Time::GetTimeOfDay(info.DayPart)
						)
					<<	Newline
					<<	String::Format(
							lunar_phase,
							Time::GetLunarPhase(info.Phase)
						)
					<<	Newline
					<<	String::Format(
							length,
							info.Length
						)
					<<	Newline
					<<	String::Format(
							age,
							info.Age
						)
					<<	Newline
					<<	String::Format(
							ticks,
							info.Elapsed
						)
					<<	Newline
					<<	String::Format(
							ticking,
							info.Total,
							avg(info.Total,info.Elapsed)
						)
					<<	Newline
					<<	String::Format(
							executing,
							info.Executing,
							avg(info.Executing,info.Elapsed)
						);
		
		}


};


INSTALL_MODULE(DisplayTime)

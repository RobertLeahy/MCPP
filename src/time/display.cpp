#include <time/time.hpp>
#include <command/command.hpp>


static const String identifier("time");
static const String name("Display Time");
static const String summary("Displays the current time.");
static const String help(
	"Syntax: /time\n"
	"Displays the current time and other time-related information."
);
static const Word priority=1;


class DisplayTime : public Command, public Module {


	private:
	
	
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
				"{0}:{1} {2}",
				(hours==0) ? 12 : ((hours>12) ? (hours-12) : hours),
				zero_pad(minutes,2),
				(hours>12) ? "PM" : "AM"
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
		
			Commands::Get().Add(this);
		
		}
	
	
		virtual const String & Identifier () const noexcept override {
		
			return identifier;
		
		}
		
		
		virtual const String & Summary () const noexcept override {
		
			return summary;
		
		}
		
		
		virtual const String & Help () const noexcept override {
		
			return help;
		
		}
		
		
		virtual bool Execute (SmartPointer<Client>, const String &, ChatMessage & message) override {
		
			auto t=Time::Get().GetInfo();
			
			message	<< ChatStyle::Bold
					<< "The current time is: "
					<< format_time(t.Item<1>())
					<< Newline
					<< "It is: "
					<< Time::GetTimeOfDay(t.Item<3>())
					<< Newline
					<< "The moon is: "
					<< Time::GetLunarPhase(t.Item<2>());
		
			//	No syntax errors ever
			return true;
		
		}


};


static Nullable<DisplayTime> module;


extern "C" {


	Module * Load () {
	
		if (module.IsNull()) module.Construct();
		
		return &(*module);
	
	}
	
	
	void Unload () {
	
		module.Destroy();
	
	}


}

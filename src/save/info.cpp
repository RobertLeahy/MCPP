#include <info/info.hpp>
#include <save/save.hpp>
#include <mod.hpp>


using namespace MCPP;


static const String name("Save Manager Information");
static const Word priority=1;
static const String identifier("save");
static const String help("Displays information about the save system.");
static const String save_banner("SAVE SYSTEM:");
static const String paused("Paused: ");
static const String true_string("Yes");
static const String false_string("No");
static const String frequency("Save Frequency: {0}ms");
static const String saves("Saves: {0}");
static const String elapsed("Elapsed: {0}ns");
static const String average("Average: {0}ns");


template <typename T>
T avg (T total, Word count) noexcept {

	return (count==0) ? 0 : (total/count);

}


class SaveInfo : public Module, public InformationProvider {


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
		
			auto info=SaveManager::Get().GetInfo();
		
			message	<<	ChatStyle::Bold
					<<	save_banner
					<<	ChatFormat::Pop
					<<	Newline
					<<	paused
					<<	(info.Paused ? true_string : false_string)
					<<	Newline
					<<	String::Format(
							frequency,
							info.Frequency
						)
					<<	Newline
					<<	String::Format(
							saves,
							info.Count
						)
					<<	Newline
					<<	String::Format(
							elapsed,
							info.Elapsed
						)
					<<	Newline
					<<	String::Format(
							average,
							avg(
								info.Elapsed,
								info.Count
							)
						);
		
		}


};


INSTALL_MODULE(SaveInfo)

/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <mod.hpp>
#include <atomic>
#include <functional>


namespace MCPP {


	/**
	 *	Encapsulates statistics and information
	 *	about a SaveManager instance.
	 */
	class SaveManagerInfo {
	
	
		public:
		
		
			/**
			 *	The number of milliseconds between
			 *	automatic save operations.
			 */
			Word Frequency;
			/**
			 *	Whether the save loop is currently
			 *	paused or not.
			 */
			bool Paused;
			/**
			 *	The number of save operations that
			 *	have been performed.
			 */
			Word Count;
			/**
			 *	The total amount of time that has been
			 *	spent saving.
			 */
			UInt64 Elapsed;
	
	
	};


	/**
	 *	Provides a facility for other modules to
	 *	periodically save data, in addition to performing
	 *	a final save as the server is shutdown.
	 */
	class SaveManager : public Module {
	
	
		private:
		
		
			Word frequency;
			Vector<std::function<void ()>> callbacks;
			//	Held while a save operation is ongoing
			//	to prevent other save operations from
			//	executing at the same time
			Mutex lock;
			//	Whether the save loop is paused or not
			std::atomic<bool> paused;
			
			
			std::atomic<Word> count;
			std::atomic<UInt64> elapsed;
			
			
			static bool is_verbose ();
			void save ();
			void save_loop ();
			
			
		public:
		
		
			/**
			 *	Retrieves a reference to a valid instance
			 *	of this class.
			 *
			 *	\return
			 *		A reference to an instance of this class.
			 */
			static SaveManager & Get () noexcept;
			
			
			/**
			 *	\cond
			 */
			 
			 
			SaveManager () noexcept;
			 
			 
			virtual Word Priority () const noexcept override;
			virtual const String & Name () const noexcept override;
			virtual void Install () override;
			 
			 
			/**
			 *	\endcond
			 */
			 
			
			/** 
			 *	Adds a callback, which will be fired periodically
			 *	and before the server shuts down.
			 *
			 *	\param [in] callback
			 *		The callback to add.
			 */
			void Add (std::function<void ()> callback);
			
			
			/**
			 *	Immediately performs a save operation.
			 */
			void operator () ();
			
			
			/**
			 *	Pauses the save loop.
			 */
			void Pause () noexcept;
			/**
			 *	Resumes the save loop.
			 */
			void Resume () noexcept;
			
			
			/**
			 *	Retrieves information and statistics about
			 *	the save manager.
			 *
			 *	\return
			 *		A structure which contains information and
			 *		statistics about the save manager.
			 */
			SaveManagerInfo GetInfo () const noexcept;
	
	
	};


}

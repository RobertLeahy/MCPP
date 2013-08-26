/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	class DataProvider {
	
	
		protected:
		
		
			DataProvider () noexcept;
	
	
		public:
		
		
			static DataProvider * GetDataProvider ();
			static const String & GetLogType (Service::LogType type) noexcept;
		
		
			DataProvider (const DataProvider &) = delete;
			DataProvider (DataProvider &&) = delete;
			DataProvider & operator = (const DataProvider &) = delete;
			DataProvider & operator = (DataProvider &&) = delete;
		
		
			virtual ~DataProvider () noexcept;
			
			
			virtual void WriteLog (const String &, Service::LogType) = 0;
			virtual void WriteChatLog (const String &, const Vector<String> &, const String &, const Nullable<String> &) = 0;
			
			
			virtual bool GetBinary (const String &, void *, Word *) = 0;
			virtual void SaveBinary (const String &, const void *, Word) = 0;
			virtual void DeleteBinary (const String &) = 0;
			
			
			virtual Nullable<String> GetSetting (const String & setting) = 0;
			virtual void SetSetting (const String & setting, const Nullable<String> & value) = 0;
			virtual void DeleteSetting (const String & setting) = 0;
			
			
			virtual void InsertValue (const String &, const String &) = 0;
			virtual void DeleteValues (const String &, const String &) = 0;
			virtual void DeleteValues (const String &) = 0;
			virtual Vector<String> GetValues (const String &) = 0;
	
	
	};


}

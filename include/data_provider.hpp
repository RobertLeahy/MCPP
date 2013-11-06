/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>
#include <cstdlib>
#include <type_traits>
#include <utility>


namespace MCPP {


	/**
	 *	Specifies an interface to which providers
	 *	that wish to provide data for the MCPP server
	 *	shall conform.
	 *
	 *	Supports the storage of settings (unique keys
	 *	mapped to values), key/value pairs (non-unique
	 *	keys mapped to values), and binary objects (unique
	 *	keys mapped to raw binary data) in addition to
	 *	chat logging and system logging.
	 */
	class DataProvider {
	
	
		protected:
		
		
			DataProvider () noexcept;
	
	
		public:
		
		
			/**
			 *	Gets a concrete DataProvider.
			 *
			 *	\return
			 *		A pointer to an object which
			 *		derives from DataProvider.
			 */
			static DataProvider * GetDataProvider ();
			/**
			 *	Maps a Service::LogType to a descriptive
			 *	string.
			 *
			 *	This function is not compiled into the
			 *	MCPP library, it must be compiled into
			 *	the data provider binary.
			 *
			 *	Include \em data_provider.o in your
			 *	data provider compilation.
			 *
			 *	\param [in] type
			 *		The type of event which is being
			 *		logged.
			 *
			 *	\return
			 *		A reference to an immutable string
			 *		which is a human readable label for
			 *		\em type.
			 */
			static const String & GetLogType (Service::LogType type) noexcept;
		
		
			DataProvider (const DataProvider &) = delete;
			DataProvider (DataProvider &&) = delete;
			DataProvider & operator = (const DataProvider &) = delete;
			DataProvider & operator = (DataProvider &&) = delete;
		
		
			/**
			 *	Destroys an instance of a derived class.
			 */
			virtual ~DataProvider () noexcept;
			
			
			/**
			 *	When overriden in a derived class, retrieves
			 *	information about that concrete implementation.
			 *
			 *	The returned value is a tuple, whose members
			 *	are a string (which is the name of the concrete
			 *	implementation) and a collection of key/value
			 *	pairs realized as a vector of 2-tuples containing
			 *	strings.  These key/value pairs contain labels and
			 *	values for pieces of information about the
			 *	concrete implementation.
			 *
			 *	\return
			 *		Information about a concrete implementation of
			 *		this abstract class.
			 */
			virtual Tuple<String,Vector<Tuple<String,String>>> GetInfo () = 0;
			
			
			/**
			 *	When overriden in a derived class, writes to
			 *	the log.
			 *
			 *	\param [in] log
			 *		The message to log.
			 *	\param [in] type
			 *		The type of message being logged.
			 */
			virtual void WriteLog (const String & log, Service::LogType type) = 0;
			/**
			 *	When overriden in a derived class, writes to
			 *	the chat log.
			 *
			 *	\param [in] from
			 *		The person sending the message.
			 *	\param [in] to
			 *		A vector of strings representing the
			 *		recipients of the message.  If everyone
			 *		was a recipient of the message (i.e.\ it
			 *		was a broadcast), pass an empty collection.
			 *	\param [in] message
			 *		The body of the message.
			 *	\param [in] notes
			 *		Any notes about this message.
			 */
			virtual void WriteChatLog (const String & from, const Vector<String> & to, const String & message, const Nullable<String> & notes) = 0;
			
			
			/**
			 *	When overriden in a derived class, fetches
			 *	binary data from the binary store.
			 *
			 *	\param [in] key
			 *		The key whose associated binary data
			 *		shall be retrieved.
			 *
			 *	\return
			 *		A vector of bytes if there was binary
			 *		data associated with \em key, \em null
			 *		otherwise.
			 */
			virtual Nullable<Vector<Byte>> GetBinary (const String & key) = 0;
			/**
			 *	When overriden in a derived class, fetches
			 *	binary data from the binary store.
			 *
			 *	\param [in] key
			 *		The key whose associated binary data
			 *		shall be retrieved.
			 *	\param [out] ptr
			 *		A pointer to the region of memory where
			 *		the binary data shall be stored.
			 *	\param [in,out] len
			 *		A pointer to the size of the region of
			 *		memory pointed to by \em ptr, in bytes.
			 *		If this call returns \em true this shall
			 *		be updated to the actual length of the
			 *		data associated with \em key in the
			 *		backing store, with all data up to the
			 *		size of the memory pointed to by \em ptr
			 *		retrieved and stored in \em ptr.
			 *
			 *	\return
			 *		\em true if there was data associated with
			 *		\em key in the backing store, \em false
			 *		otherwise.
			 */
			virtual bool GetBinary (const String & key, void * ptr, Word * len) = 0;
			/**
			 *	When overriden in a derived class, saves
			 *	binary data to the binary store.
			 *
			 *	\param [in] key
			 *		The key to which binary data shall
			 *		be saved.
			 *	\param [in] ptr
			 *		A pointer to the region of memory
			 *		which shall be saved.
			 *	\param [in] len
			 *		The length of the memory pointed to
			 *		by \em ptr.
			 */
			virtual void SaveBinary (const String & key, const void * ptr, Word len) = 0;
			/**
			 *	When overriden in a derived class, deletes
			 *	binary data from the binary store.
			 *
			 *	\param [in] key
			 *		The key whose associated binary data
			 *		shall be deleted.
			 */
			virtual void DeleteBinary (const String & key) = 0;
			
			
			/**
			 *	When overriden in a derived class, retrieves
			 *	the value of a setting.
			 *
			 *	\param [in] setting
			 *		The name of the setting to retrieve.
			 *
			 *	\return
			 *		The value of \em setting.  Will be null
			 *		if the setting does not exist, or if the
			 *		backing store supports storing null values
			 *		and the value is null.
			 */
			virtual Nullable<String> GetSetting (const String & setting) = 0;
			/**
			 *	Retrieves the value of a setting converted
			 *	to an arbitrary type.
			 *
			 *	\tparam T
			 *		The type as which to retrieve the setting.
			 *
			 *	\param [in] setting
			 *		The name of the setting to retrieve.
			 *
			 *	\return
			 *		The value of \em setting, or \em default_value
			 *		if the value of \em setting does not exist, or
			 *		could not be converted to a \em T.
			 */
			template <typename T>
			T GetSetting (const String & setting, T default_value);
			/**
			 *	When overriden in a derived class, sets the
			 *	value of a setting.
			 *
			 *	\param [in] setting
			 *		The name of the setting whose value shall
			 *		be set.
			 *	\param [in] value
			 *		The value to set the setting to.
			 */
			virtual void SetSetting (const String & setting, const Nullable<String> & value) = 0;
			/**
			 *	When overriden in a derived class, deletes the
			 *	value of a setting.
			 *
			 *	\param [in] setting
			 *		The name of the setting whose value shall
			 *		be deleted.
			 */
			virtual void DeleteSetting (const String & setting) = 0;
			
			
			/**
			 *	When overriden in a derived class, inserts a
			 *	new pair into the key/value pair store.
			 *
			 *	\param [in] key
			 *		The key of the key/value pair to insert.
			 *	\param [in] value
			 *		The value of the key/value pair to insert.
			 */
			virtual void InsertValue (const String & key, const String & value) = 0;
			/**
			 *	When overriden in a derived class, deletes
			 *	pairs from the key/value store which have
			 *	a particular key and a particular value.
			 *
			 *	\param [in] key
			 *		The key which a key/value pair shall have
			 *		to qualify for deletion.
			 *	\param [in] value
			 *		The value which a key/value pair shall have
			 *		to qualify for deletion.
			 */
			virtual void DeleteValues (const String & key, const String & value) = 0;
			/**
			 *	When overriden in a derived class, deletes
			 *	all pairs from the key/value store which have
			 *	a particular key.
			 *
			 *	\param [in] key
			 *		The key which a key/value pair shall have
			 *		to qualify for deletion.
			 */
			virtual void DeleteValues (const String & key) = 0;
			/**
			 *	When overriden in a derived class, retrieves
			 *	all values associated with a given key from
			 *	the key/value store.
			 *
			 *	\param [in] key
			 *		The key whose associated values shall be
			 *		retrieved.
			 *
			 *	\return
			 *		A vector of strings containing the values
			 *		associated with \em key in the backing
			 *		store.
			 */
			virtual Vector<String> GetValues (const String & key) = 0;
	
	
	};
	 
	 
	template <typename T, typename=void>
	class SettingConverter {
	
	
		public:
		
		
			static T Get (DataProvider & dp, const String & setting, T default_value) {
			
				return default_value;
			
			}
	
	
	};
	
	
	/**
	 *	\cond
	 */
	 
	 
	template <>
	class SettingConverter<String,void> {
	
	
		public:
		
		
			static String Get (DataProvider & dp, const String & setting, String default_value) {
			
				auto val=dp.GetSetting(setting);
				
				return val.IsNull() ? default_value : std::move(*val);
			
			}
	
	
	};
	
	
	template <typename T>
	class SettingConverter<T,typename std::enable_if<std::is_integral<T>::value && !std::is_same<T,bool>::value>::type> {
	
	
		public:
		
		
			static T Get (DataProvider & dp, const String & setting, T default_value) {
			
				auto val=dp.GetSetting(setting);
				
				if (val.IsNull()) return default_value;
				
				T retr;
				if (val->ToInteger(&retr)) return retr;
				
				return default_value;
			
			}
	
	
	};
	
	
	template <typename T>
	class SettingConverter<T,typename std::enable_if<std::is_floating_point<T>::value>::type> {
	
	
		public:
		
		
			static T Get (DataProvider & dp, const String & setting, T default_value) {
			
				auto val=dp.GetSetting(setting);
				
				if (val.IsNull()) return default_value;
				
				auto c_str=val->ToCString();
				auto end_ptr=c_str.begin();
				T retr;
				if (std::is_same<T,Single>::value) retr=static_cast<T>(strtof(end_ptr,&end_ptr));
				else if (std::is_same<T,Double>::value) retr=static_cast<T>(strtod(end_ptr,&end_ptr));
				else retr=static_cast<T>(strtold(end_ptr,&end_ptr));
				
				if (end_ptr==c_str.begin()) return default_value;
				
				return retr;
			
			}
	
	
	};
	
	
	template <>
	class SettingConverter<bool,void> {
	
	
		public:
		
		
			static bool Get (DataProvider & dp, const String & setting, bool default_value) {
			
				auto val=dp.GetSetting(setting);
				
				if (val.IsNull()) return default_value;
				
				if (Regex(
					"^\\s*(?:t(?:rue)?|y(?:es)?)\\s*$",
					RegexOptions().SetIgnoreCase()
				).IsMatch(*val)) return true;
				
				if (Regex(
					"^\\s*(?:no?|f(?:alse)?)\\s*$",
					RegexOptions().SetIgnoreCase()
				).IsMatch(*val)) return false;
				
				return default_value;
			
			}
	
	
	};
	
	
	template <typename T>
	T DataProvider::GetSetting (const String & setting, T default_value) {
	
		return SettingConverter<typename std::decay<T>::type>::Get(
			*this,
			setting,
			std::forward<T>(default_value)
		);
	
	}
	
	
	/**
	 *	\endcond
	 */


}

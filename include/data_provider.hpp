/**
 *	\file
 */
 
 
#pragma once


#include <functional>
#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	The type of callback that shall be invoked
	 *	when the chunk loading process is complete.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	0.	X co-ordinate of the chunk.
	 *	1.	Y co-ordinate of the chunk.
	 *	2.	Z co-ordinate of the chunk.
	 *	3.	The dimension in which the chunk
	 *		resides.
	 *	4.	\em true if the chunk was loaded
	 *		successfully and the provided
	 *		buffer should be considered,
	 *		\em false otherwise.
	 *	5.	A buffer of bytes representing the
	 *		chunk (if it exists).
	 */
	typedef std::function<void (Int32, Int32, Int32, SByte, bool, Nullable<Vector<Byte>> &&)> ChunkLoad;
	/**
	 *	The type of callback that shall be invoked
	 *	when the chunk saving process begins.
	 *
	 *	Chunk saving will occur immediately after this
	 *	function returns \em true, invocation of
	 *	this function means that there is a thread
	 *	available and with all required locks to
	 *	write the chunk to the datastore.  This function
	 *	should ensure the pointers passed to
	 *	DataProvider::SaveChunk are still valid, and
	 *	that no other threads will attempt to modify
	 *	the chunk during the saving process, and then
	 *	return.
	 *
	 *	If this function returns \em false the chunk
	 *	saving process shall be aborted and the
	 *	provided pointers are guaranteed to have
	 *	never been dereferenced.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	0.	X co-ordinate of the chunk.
	 *	1.	Y co-ordinate of the chunk.
	 *	2.	Z co-ordinate of the chunk.
	 *	3.	The dimension in which the
	 *		chunk resides.
	 *
	 *	<B>Return Value:</B>
	 *
	 *	\em true if the data provider should save the
	 *	chunk to the backing store through the begin
	 *	and end iterators provided to the DataProvider::SaveChunk
	 *	call, \em false if the data provider should
	 *	abort at once.
	 *
	 *	If the operation is aborted the complete callback
	 *	is never invoked.
	 */
	typedef std::function<bool (Int32, Int32, Int32, SByte)> ChunkSaveBegin;
	/**
	 *	The type of callback that shall be invoked
	 *	when the chunk saving process is complete.
	 *
	 *	<B>Parameters:</B>
	 *
	 *	0.	X co-ordinate of the chunk.
	 *	1.	Y co-ordinate of the chunk.
	 *	2.	Z co-ordinate of the chunk.
	 *	3.	The dimension in which the
	 *		chunk resides.
	 *	4.	\em true if the chunk was loaded
	 *		successfully, \em false
	 *		otherwise.
	 */
	typedef std::function<void (Int32, Int32, Int32, SByte, bool)> ChunkSaveEnd;


	/**
	 *	The base class for all implementations
	 *	which supply the server with needed
	 *	data.
	 */
	class DataProvider {
	
	
		public:
		
		
			/**
			 *	Maps a Service::LogType to a descriptive
			 *	string.
			 *
			 *	This function is not compiled into the
			 *	MCPP binary, it must be compiled into
			 *	the data provider binary.
			 *
			 *	Include \em data_provider.cpp in your
			 *	data provider compilation.
			 */
			static String GetLogType (Service::LogType type);
		
		
			DataProvider () noexcept;
			DataProvider (const DataProvider &) = delete;
			DataProvider (DataProvider &&) = delete;
			DataProvider & operator = (const DataProvider &) = delete;
			DataProvider & operator = (DataProvider &&) = delete;
		
		
			/**
			 *	Gets a DataProvider to use for the
			 *	server's data needs.
			 *
			 *	The server shall be linked against
			 *	a certain library which shall provide
			 *	this function, which shall be called
			 *	to dynamically acquire a DataProvider.
			 *
			 *	\return
			 *		A pointer to a DataProvider object.
			 */
			static DataProvider * GetDataProvider ();
			
			
			/**
			 *	Virtual destructor.
			 *
			 *	This function is not compiled into the
			 *	MCPP binary, it must be compiled into
			 *	the data provider binary.
			 *
			 *	Include \em data_provider.cpp in your
			 *	data provider compilation.
			 */
			virtual ~DataProvider () noexcept;
			
			
			/**
			 *	When implemented in a base class, retrieves
			 *	information about this data provider.
			 *
			 *	\return
			 *		A tuple with two members:
			 *
			 *		1.	The name of the data provider.
			 *		2.	A list of tuples which act as
			 *			key/value pairs which map the
			 *			name of a piece of information
			 *			about the provider to the value
			 *			associated with it.
			 */
			virtual Tuple<String,Vector<Tuple<String,String>>> GetInfo () const = 0;
		
		
			/**
			 *	Removes a setting entirely from the
			 *	backing store.
			 *
			 *	\param [in] setting
			 *		A string representing the name
			 *		of the setting to delete.
			 */
			//virtual void DeleteSetting (const String & setting) = 0;
			/**
			 *	Sets a setting in the backing store.
			 *
			 *	\param [in] setting
			 *		A string representing the name
			 *		of the setting to set.
			 *	\param [in] value
			 *		A nullable string representing
			 *		the value to set the setting
			 *		to.
			 */
			//virtual void SetSetting (const String & setting, const Nullable<String> & value) = 0;
			/**
			 *	Attempts to get a setting from
			 *	whatever backing store is in use.
			 *
			 *	If the setting doesn't exist, or
			 *	is explicitly set to some value
			 *	analogous to \em null in the backing
			 *	store, a nulled Nullable<String>
			 *	is returned, otherwise the Nullable<String>
			 *	contains the String whose value corresponds
			 *	to the setting-in-question.
			 *
			 *	\param [in] setting
			 *		The key or name of the setting to
			 *		fetch.
			 *
			 *	\return
			 *		A nullable string which is nulled
			 *		if the setting-in-question is not
			 *		set, otherwise it is set to the value
			 *		of the setting.
			 */
			virtual Nullable<String> GetSetting (const String & setting) = 0;
			
			
			/**
			 *	Retrieves all the values associated with
			 *	the given key in the backing store's
			 *	key/value pair store.
			 *
			 *	\param [in] key
			 *		The key whose values are to be
			 *		retrieved.
			 *
			 *	\return
			 *		A vecter of nullable strings containing
			 *		the values associated with \em key in
			 *		an unspecified order.
			 */
			virtual Vector<Nullable<String>> GetValues (const String & key) = 0;
			/**
			 *	Deletes all key/value pairs with the given
			 *	key and the given value.
			 *
			 *	\param [in] key
			 *		The key that a pair must have to qualify
			 *		for deletion.
			 *	\param [in] value
			 *		The value that a pair must have to qualify
			 *		for deletion.
			 */
			virtual void DeleteValues (const String & key, const String & value) = 0;
			/**
			 *	Creates a new key/value pair in the backing store
			 *	with the given key and value.
			 *
			 *	\param [in] key
			 *		The key associated with the key/value pair
			 *		to be created.
			 *	\param [in] value
			 *		The value associated with the key/value pair
			 *		to be created.
			 */
			virtual void InsertValue (const String & key, const String & value) = 0;
			
			
			/**
			 *	Deletes all key/value pairs with the given
			 *	key and the given value.
			 *
			 *	\param [in] key
			 *		The key to delete.
			 *	\param [in] value
			 *		The value to delete.
			 */
			//virtual void DeletePairs (const String & key, const String & value) = 0;
			/**
			 *	Deletes all values associated with the
			 *	given key.
			 *
			 *	\param [in] key
			 *		The key to delete.
			 */
			//virtual void DeleteKey (const String & key) = 0;
			/**
			 *	Sets the value of a certain pair or pairs.
			 *
			 *	\param [in] key
			 *		The key.
			 *	\param [in] curr
			 *		The current value of the key/value pair
			 *		being targeted.
			 *	\param [in] value
			 *		The new value to replace \em curr with.
			 */
			//virtual void SetValues (const String & key, const String & curr, const String & value) = 0;
			
			
			/**
			 *	Starts the process of retrieving a chunk
			 *	from the backing store.
			 *
			 *	Whether this function returns immediately
			 *	and loads the chunk from the backing store
			 *	in the background, or loads the chunk
			 *	synchronously is implementation-defined,
			 *	and should not be relied upon.
			 *
			 *	\param [in] x
			 *		The x-coordinate of the chunk.
			 *		This is the displacement of the
			 *		target chunk from the origin in
			 *		chunks, and is unrelated to blocks.
			 *	\param [in] y
			 *		The y-coordinate of the chunk.
			 *		This is the displacement of the
			 *		target chunk from the origin in
			 *		chunks, and is unrelated to blocks.
			 *	\param [in] z
			 *		The z-coordinate of the chunk.
			 *		This is the displacement of the
			 *		target chunk from the origin in
			 *		chunks, and is unrelated to blocks.
			 *	\param [in] dimension
			 *		The dimension in which the target
			 *		chunk resides.
			 *	\param [in] callback
			 *		A function to be invoked once the
			 *		chunk has been loaded.
			 */
			//virtual void LoadChunk (Int32 x, Int32 y, Int32 z, SByte dimension, const ChunkLoad & callback) = 0;
			/**
			 *	Starts the process of saving a chunk to
			 *	the backing store.
			 *
			 *	Whether this function returns immediately
			 *	and saves the chunk in the background, or
			 *	saves the chunk synchronously is
			 *	implementation-defined, and should not be
			 *	relied upon.
			 *
			 *	\param [in] x
			 *		The x-coordinate of the chunk.
			 *		This is the displacement of the
			 *		target chunk from the origin in
			 *		chunks, and is unrelated to blocks.
			 *	\param [in] y
			 *		The y-coordinate of the chunk.
			 *		This is the displacement of the
			 *		target chunk from the origin in
			 *		chunks, and is unrelated to blocks.
			 *	\param [in] z
			 *		The z-coordinate of the chunk.
			 *		This is the displacement of the
			 *		target chunk from the origin in
			 *		chunks, and is unrelated to blocks.
			 *	\param [in] dimension
			 *		The dimension in which the target
			 *		chunk resides.
			 *	\param [in] begin
			 *		A begin iterator to the contiguous
			 *		buffer of bytes which represent the
			 *		chunk.
			 *	\param [in] end
			 *		An end iterator to the contiguous
			 *		buffer of bytes which represent the
			 *		chunk.
			 *	\param [in] callback_begin
			 *		A callback which shall be invoked
			 *		as the chunk saving process is about
			 *		to begin.
			 *	\param [in] callback_end
			 *		A callback which shall be invoked
			 *		after the chunk saving process is
			 *		complete.
			 */
			/*virtual void SaveChunk (
				Int32 x,
				Int32 y,
				Int32 z,
				SByte dimension,
				const Byte * begin,
				const Byte * end,
				const ChunkSaveBegin & callback_begin,
				const ChunkSaveEnd & callback_end
			) = 0;*/
			
			
			/**
			 *	Writes to the log.
			 *
			 *	\param [in] log
			 *		The text to log.
			 *	\param [in] type
			 *		The type of event to log.
			 */
			virtual void WriteLog (const String & log, Service::LogType type) = 0;
			/**
			 *	Writes to the chat log.
			 *
			 *	\param [in] from
			 *		The sender of the message.
			 *	\param [in] to
			 *		A list of the username's of
			 *		the recipients of the message.
			 *	\param [in] message
			 *		The text of the message.
			 *	\param [in] notes
			 *		Optional notes to associate with
			 *		the message.
			 */
			virtual void WriteChatLog (const String & from, const Vector<String> & to, const String & message, const Nullable<String> & notes) = 0;
	
	
	};


}

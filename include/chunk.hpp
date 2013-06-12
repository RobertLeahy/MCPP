/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	Represents a chunk of the Minecraft
	 *	world.
	 */
	class Chunk {
	
	
		private:
		
		
			//	Raw data
			Vector<Byte> data;
			//	Does the data buffer have
			//	skylight info?
			bool skylight;
			//	Does the data buffer have
			//	the extra 4 bits for extra
			//	block types?
			bool add;
			//	Lock on the chunk
			RWLock lock;
			
			
		public:
		
		
			Chunk () = delete;
			
			
			/**
			 *	Creates a new chunk by taking
			 *	ownership of a buffer of bytes.
			 *
			 *	If \em buffer is not of the correct
			 *	size an exception is thrown and
			 *	ownership is not taken.
			 *
			 *	\param [in] data
			 *		The buffer of bytes which
			 *		contains data about the chunk.
			 *	\param [in] skylight
			 *		Whether the buffer has skylight
			 *		info.
			 *	\param [in] add
			 *		Whether the uffer has the extra
			 *		4 bits for extra block types.
			 */
			Chunk (Vector<Byte> data, bool skylight, bool add) noexcept;
			
			
			/**
			 *	Retrieves the number of bytes in
			 *	this chunk.
			 *
			 *	\return
			 *		The number of bytes in this
			 *		chunk.
			 */
			Word Count () const noexcept;
			/**
			 *	Determines whether this chunk has
			 *	the extra nibbles for additional
			 *	block types.
			 *
			 *	\return
			 *		Whether this chunk contains
			 *		an add nibble array.
			 */
			bool Add () const noexcept;
			
			
			/**
			 *	Begins a read on this chunk.
			 */
			void Read () noexcept;
			/**
			 *	Ends a read on this chunk.
			 */
			void CompleteRead () noexcept;
			/**
			 *	Begins a write on this chunk.
			 */
			void Write () noexcept;
			/**
			 *	Ends a write on this chunk.
			 */
			void CompleteWrite () noexcept;
			
			
			/**
			 *	Gets a begin iterator to the buffer
			 *	of bytes underlying this chunk.
			 *
			 *	\return
			 *		An iterator pointing to the
			 *		first byte in this chunk.
			 */
			Byte * begin () noexcept;
			/**
			 *	Gets a begin iterator to the buffer
			 *	of bytes underlying this chunk.
			 *
			 *	\return
			 *		An iterator pointing to the
			 *		first byte in this chunk.
			 */
			const Byte * begin () const noexcept;
			/**
			 *	Gets an end iterator to the buffer
			 *	of bytes underlying this chunk.
			 *
			 *	\return
			 *		An iterator pointing to one
			 *		past the last byte in this
			 *		chunk.
			 */
			Byte * end () noexcept;
			/**
			 *	Gets an end iterator to the buffer
			 *	of bytes underlying this chunk.
			 *
			 *	\return
			 *		An iterator pointing to one
			 *		past the last byte in this
			 *		chunk.
			 */
			const Byte * end () const noexcept;
			
			
			/**
			 *	Gets the biome for a particular
			 *	XZ co-ordinate combination.
			 *
			 *	This function does not perform
			 *	bounds checking.
			 *
			 *	\param [in] x
			 *		The x-coordinate to fetch
			 *		the biome for.
			 *	\param [in] z
			 *		The z-coordinate to fetch
			 *		the biome for.
			 *
			 *	\return
			 *		The biome of the XZ co-ordinate
			 *		combination-in-question.
			 */
			Byte & Biome (Byte x, Byte z) noexcept;
			/**
			 *	Gets the biome for a particular
			 *	XZ co-ordinate combination.
			 *
			 *	This function does not perform
			 *	bounds checking.
			 *
			 *	\param [in] x
			 *		The x-coordinate to fetch
			 *		the biome for.
			 *	\param [in] z
			 *		The z-coordinate to fetch
			 *		the biome for.
			 *
			 *	\return
			 *		The biome of the XZ co-ordinate
			 *		combination-in-question.
			 */
			const Byte & Biome (Byte x, Byte z) const noexcept;
			
	
	
	};


}

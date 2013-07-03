/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	Represents a column of the
	 *	Minecraft world (16 vertically
	 *	stacked chunks).
	 */
	class Column {
	
	
		public:
		
		
			/**
			 *	The raw bytes which make up this
			 *	column.
			 */
			Byte Data [(16*16*16*3*16)+(16*16)];
			/**
			 *	\em true if this column has skylight
			 *	data, \em false otherwise.
			 */
			bool Skylight;
			/**
			 *	\em true if this column has add data
			 *	for high numbered block types, \em false
			 *	otherwise.
			 */
			bool Add;
	
	
	};


}

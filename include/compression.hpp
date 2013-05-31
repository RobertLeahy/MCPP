/**
 *	\file
 */
 
 
#pragma once


#include <rleahylib/rleahylib.hpp>


namespace MCPP {


	/**
	 *	Uses ZLib deflate() to compress
	 *	a buffer of bytes.
	 *
	 *	\param [in] begin
	 *		An iterator which points to the first
	 *		byte in the source buffer.
	 *	\param [in] end
	 *		An iterator which points to one past
	 *		the last byte in the source buffer.
	 *	\param [in] gzip
	 *		\em true if GZip should be used,
	 *		\em false for ZLib.  Defaults to
	 *		\em false.
	 *
	 *	\return
	 *		A buffer of bytes which contain
	 *		the compressed representation of
	 *		\em buffer.
	 */
	Vector<Byte> Deflate (const Byte * begin, const Byte * end, bool gzip=false);
	/**
	 *	Uses ZLib deflate() to compress a buffer
	 *	of bytes and places the result thereof
	 *	directly into another buffer of bytes.
	 *
	 *	The existing contents of the destination
	 *	buffer are guaranteed not to be modified.
	 *
	 *	In the case of an error the contents of
	 *	the destination buffer are guaranteed not
	 *	to be added to.
	 *
	 *	\param [in] begin
	 *		An iterator which points to the first
	 *		byte in the source buffer.
	 *	\param [in] end
	 *		An iterator which points to one past
	 *		the last byte in the source buffer.
	 *	\param [in,out] buffer
	 *		The buffer of bytes into which to
	 *		compress.
	 *	\param [in] gzip
	 *		\em true if GZip should be used,
	 *		\em false for ZLib.  Defaults to
	 *		\em false.
	 */
	void Deflate (const Byte * begin, const Byte * end, Vector<Byte> * buffer, bool gzip=false);
	/**
	 *	Gets the upper bound on the size of
	 *	deflating \em num bytes.
	 *
	 *	\param [in] num
	 *		A number of bytes.
	 *	\param [in] gzip
	 *		\em true if the bound for GZip
	 *		compression is to be obtained,
	 *		\em false for ZLib.  Defaults to
	 *		\em false.
	 *
	 *	\return
	 *		The maximum number of bytes
	 *		\em num bytes will deflate to.
	 */
	Word DeflateBound (Word num, bool gzip=false);
	
	
	/**
	 *	Uses ZLib inflate() to decompress
	 *	a buffer of bytes.
	 *
	 *	Whether ZLib or GZip was used to deflate
	 *	the buffer is irrelevant, the compression
	 *	will be automatically detected.
	 *
	 *	\param [in] begin
	 *		An iterator which points to the first
	 *		byte in the source buffer.
	 *	\param [in] end
	 *		An iterator which points to one past
	 *		the last byte in the source buffer.
	 *
	 *	\return
	 *		A buffer of bytes which contain the
	 *		decompressed representation of
	 *		\em buffer.
	 */
	Vector<Byte> Inflate (const Byte * begin, const Byte * end);
	/**
	 *	Uses ZLib inflate() to decompress a
	 *	buffer of bytes.
	 *
	 *	Whether ZLib or GZip was used to deflate
	 *	the buffer is irrelevant, the compression
	 *	will be automatically detected.
	 *
	 *	The existing contents of the destination
	 *	buffer are guaranteed not to be modified.
	 *
	 *	In the case of an error the contents of
	 *	the destination buffer are guaranteed not
	 *	to be added to.
	 *
	 *	\param [in] begin
	 *		An iterator which points to the first
	 *		byte in the source buffer.
	 *	\param [in] end
	 *		An iterator which points to one past
	 *		the last byte in the source buffer.
	 *	\param [in,out] buffer
	 *		The buffer of bytes into which to
	 *		decompress.
	 */
	void Inflate (const Byte * begin, const Byte * end, Vector<Byte> * buffer);


}

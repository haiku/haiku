//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file EmbeddedStream.cpp
*/

#include "EmbeddedStream.h"

#include <stdlib.h>
#include <string.h>

EmbeddedStream::EmbeddedStream(DataStream &stream, off_t offset, size_t size)
	: SimulatedStream(stream)
	, fOffset(offset)
	, fSize(size)
{
}

/*! \brief Returns the largest extent in the underlying data stream
	corresponding to the extent starting at byte position \a pos of
	byte length \a size in the output parameter \a extent.
	
	NOTE: If the position is at or beyond the end of the stream, the
	function will return B_OK, but the value of extent.size will be 0.
*/
status_t
EmbeddedStream::_GetExtent(off_t pos, size_t size, data_extent &extent)
{
	if (pos >= fSize) {
		// end of stream
		extent.offset = fOffset + fSize;
		extent.size = 0;
	} else {
		// valid position
		extent.offset = fOffset + pos;
		extent.size = fSize - pos;
	}
	return B_OK;
}	

/*! \brief Returns the current size of the stream.
*/
off_t
EmbeddedStream::_Size()
{
	return fSize;
}

//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file ExtentStream.cpp
*/

#include "ExtentStream.h"

#include <stdlib.h>
#include <string.h>

ExtentStream::ExtentStream(DataStream &stream,
                           const std::list<Udf::extent_address> &extentList,
                           uint32 blockSize)
	: SimulatedStream(stream)
	, fExtentList(extentList)
	, fBlockSize(blockSize)
	, fSize(0)
{
	for (std::list<Udf::extent_address>::const_iterator i = fExtentList.begin();
	       i != fExtentList.end();
	         i++)
	{
		fSize += i->length();
	}
}

/*! \brief Returns the largest extent in the underlying data stream
	corresponding to the extent starting at byte position \a pos of
	byte length \a size in the output parameter \a extent.
	
	NOTE: If the position is at or beyond the end of the stream, the
	function will return B_OK, but the value of extent.size will be 0.
*/
status_t
ExtentStream::_GetExtent(off_t pos, size_t size, data_extent &extent)
{
	status_t error = pos >= 0 ? B_OK : B_BAD_VALUE;
	if (!error) {
		off_t finalOffset = 0;
		off_t streamPos = 0;
		for (std::list<Udf::extent_address>::const_iterator i = fExtentList.begin();
		       i != fExtentList.end();
		         i++)
		{
			off_t offset = i->location() * fBlockSize;
			finalOffset = offset + i->length();
			if (streamPos <= pos && pos < streamPos+i->length()) {
				// Found it
				off_t difference = pos - streamPos;
				extent.offset = offset + difference;
				extent.size = i->length() - difference;
				if (extent.size > size)
					extent.size = size;
				return B_OK;
			} else {
				streamPos += i->length();
			}
		}
		// Didn't find it, so pos is past the end of the stream
		extent.offset = finalOffset;
		extent.size = 0;
	}
	return error;
}	

/*! \brief Returns the current size of the stream.
*/
off_t
ExtentStream::_Size()
{
	return fSize;
}

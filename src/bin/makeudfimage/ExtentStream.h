//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file ExtentStream.h
*/

#ifndef _EXTENT_STREAM_H
#define _EXTENT_STREAM_H

#include <list>

#include "SimulatedStream.h"
#include "UdfStructures.h"

/*! \brief SimulatedStream implementation that takes a list of
	block-aligned data extents.
*/
class ExtentStream : public SimulatedStream {
public:
	ExtentStream(DataStream &stream, const std::list<Udf::extent_address> &extentList, uint32 blockSize);

protected:
	virtual status_t _GetExtent(off_t pos, size_t size, data_extent &extent);
	virtual off_t _Size();

private:
	const std::list<Udf::extent_address> &fExtentList;
	const uint32 fBlockSize;
	off_t fSize;
};

#endif	// _EXTENT_STREAM_H

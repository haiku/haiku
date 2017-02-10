//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file EmbeddedStream.h
*/

#ifndef _EMBEDDED_STREAM_H
#define _EMBEDDED_STREAM_H

#include "SimulatedStream.h"

/*! \brief SimulatedStream implementation that takes a single, 
	not-neccessarily-block-aligned extent.
*/
class EmbeddedStream : public SimulatedStream {
public:
	EmbeddedStream(DataStream &stream, off_t offset, size_t size);

protected:
	virtual status_t _GetExtent(off_t pos, size_t size, data_extent &extent);
	virtual off_t _Size();

private:
	off_t fOffset;
	size_t fSize;
};

#endif	// _EMBEDDED_STREAM_H

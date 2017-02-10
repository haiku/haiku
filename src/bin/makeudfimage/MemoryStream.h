//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file MemoryStream.h
*/

#ifndef _MEMORY_STREAM_H
#define _MEMORY_STREAM_H

#include <DataIO.h>

#include "PositionIOStream.h"

/*! \brief 	DataStream implementation that writes directly to a chunk of memory.
*/
class MemoryStream : public PositionIOStream {
public:
	MemoryStream(void *buffer, size_t length);
	virtual status_t InitCheck() const;

private:
	BMemoryIO fMemory;
	status_t fInitStatus;
};

#endif	// _MEMORY_STREAM_H

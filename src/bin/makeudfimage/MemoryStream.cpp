//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file MemoryStream.cpp
*/

#include "MemoryStream.h"

#include <stdlib.h>
#include <string.h>

MemoryStream::MemoryStream(void *buffer, size_t length)
	: PositionIOStream(fMemory)
	, fMemory(buffer, length)
	, fInitStatus(buffer ? B_OK : B_NO_INIT)
{
}

status_t
MemoryStream::InitCheck() const
{
	status_t error = PositionIOStream::InitCheck();
	if (!error)
		error = fInitStatus;
	return error;
}


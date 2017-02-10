//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file FileStream.cpp
*/

#include "FileStream.h"

#include <stdlib.h>
#include <string.h>

FileStream::FileStream(const char *path, uint32 open_mode)
	: PositionIOStream(fFile)
	, fFile(path, open_mode)
{
}

status_t
FileStream::InitCheck() const
{
	status_t error = PositionIOStream::InitCheck();
	if (!error)
		error = fFile.InitCheck();
	return error;
}


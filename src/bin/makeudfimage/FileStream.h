//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file FileStream.h
*/

#ifndef _FILE_STREAM_H
#define _FILE_STREAM_H

#include <File.h>

#include "PositionIOStream.h"

/*! \brief 	DataStream implementation that writes directly to a file.
*/
class FileStream : public PositionIOStream {
public:
	FileStream(const char *path, uint32 open_mode);
	virtual status_t InitCheck() const;
	void Flush() { fFile.Sync(); }

private:
	BFile fFile;	               
};

#endif	// _FILE_STREAM_H

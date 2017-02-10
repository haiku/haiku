//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file PositionIOStream.h
*/

#ifndef _POSITION_IO_STREAM_H
#define _POSITION_IO_STREAM_H

#include <DataIO.h>

#include "DataStream.h"

/*! \brief 	DataStream implementation that writes to a BPositionIO.
*/
class PositionIOStream : public DataStream {
public:
	PositionIOStream(BPositionIO &stream);
	virtual status_t InitCheck() const { return B_OK; }

	static const size_t kBufferSize = 32 * 1024;

	virtual ssize_t Read(void *buffer, size_t size);
	virtual	ssize_t ReadAt(off_t pos, void *buffer, size_t size);

	virtual ssize_t Write(const void *buffer, size_t size);
	virtual ssize_t WriteAt(off_t pos, const void *buffer, size_t size);

	virtual ssize_t Write(BDataIO &data, size_t size);
	virtual ssize_t	WriteAt(off_t pos, BDataIO &data, size_t size);

	virtual ssize_t Zero(size_t size);
	virtual ssize_t	ZeroAt(off_t pos, size_t size);	

	virtual off_t Seek(off_t position, uint32 seek_mode);
	virtual off_t Position() const;

	virtual status_t SetSize(off_t size);
private:
	BPositionIO &fStream;	               
};

#endif	// _POSITION_IO_STREAM_H

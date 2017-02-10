//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file DataStream.h
*/

#ifndef _DATA_STREAM_H
#define _DATA_STREAM_H

#include <DataIO.h>

class DataStream : public BPositionIO {
public:
	virtual status_t InitCheck() const = 0;

	virtual ssize_t Read(void *buffer, size_t size) = 0;
	virtual	ssize_t ReadAt(off_t pos, void *buffer, size_t size) = 0;

	virtual ssize_t Write(const void *buffer, size_t size) = 0;
	virtual ssize_t WriteAt(off_t pos, const void *buffer, size_t size) = 0;

	virtual ssize_t Write(BDataIO &data, size_t size) = 0;
	virtual ssize_t	WriteAt(off_t pos, BDataIO &data, size_t size) = 0;

	virtual ssize_t Zero(size_t size) = 0;
	virtual ssize_t	ZeroAt(off_t pos, size_t size) = 0;	

	virtual off_t Seek(off_t position, uint32 seek_mode) = 0;
	virtual off_t Position() const = 0;

	virtual status_t SetSize(off_t size) = 0;
};

#endif	// _DATA_STREAM_H

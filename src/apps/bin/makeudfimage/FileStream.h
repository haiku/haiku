//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file FileStream.h
*/

#ifndef _OUTPUT_FILE_H
#define _OUTPUT_FILE_H

#include <File.h>

#include "DataStream.h"

/*! \brief 	DataStream implementation that writes directly to a file.

	Basically, this the filestream interface of BFile with a few
	new, handy output functions.
*/
class FileStream : public DataStream {
public:
	FileStream(const char *path, uint32 open_mode);
	status_t InitCheck() const;

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
	BFile fFile;	               
};

#endif	// _OUTPUT_FILE_H

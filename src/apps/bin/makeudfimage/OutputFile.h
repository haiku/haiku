//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file OutputFile.h

	BFile subclass with some handy new output methods. (declarations)
*/

#ifndef _OUTPUT_FILE_H
#define _OUTPUT_FILE_H

#include <File.h>

class OutputFile : public BFile {
public:
	OutputFile(const char *path, uint32 open_mode);

	static const size_t kDefaultBufferSize = 32 * 1024;

	virtual	ssize_t Write(const void *buffer, size_t size);
	virtual	ssize_t WriteAt(off_t pos, const void *buffer, size_t size);
	ssize_t Write(BDataIO &data, size_t size,
	              size_t bufferSize = kDefaultBufferSize);
	ssize_t	WriteAt(off_t pos, BDataIO &data, size_t size,
	                size_t bufferSize = kDefaultBufferSize);
	ssize_t Zero(size_t size, size_t bufferSize = kDefaultBufferSize);
	ssize_t	ZeroAt(off_t pos, size_t size,
	               size_t bufferSize = kDefaultBufferSize);	
};

#endif	// _OUTPUT_FILE_H

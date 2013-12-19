/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef PSD_DATA_ARRAY_H
#define PSD_DATA_ARRAY_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <String.h>
#include <DataIO.h>
#include <File.h>
#include <ByteOrder.h>

#define DATAARRAY_BLOCK_SIZE 1024

class BDataArray {
public:
					BDataArray(int32 blockSize = DATAARRAY_BLOCK_SIZE);
					~BDataArray();

	status_t		Append(int8 val);
	status_t		Append(uint8 val);
	status_t		Append(int16 val);
	status_t		Append(uint16 val);
	status_t		Append(int32 val);
	status_t		Append(uint32 val);
	status_t		Append(const char *str);
	status_t		Append(BString str);
	status_t		Append(uint8 *ptr, int32 len);
	
	status_t		Repeat(uint8 byte, int32 count);

	BDataArray&		operator<<(int8);
	BDataArray&		operator<<(uint8);
	BDataArray&		operator<<(int16);
	BDataArray&		operator<<(uint16);
	BDataArray&		operator<<(int32);
	BDataArray&		operator<<(uint32);
	BDataArray&		operator<<(const char*);

	uint8			*Buffer(void);
	int32			Length(void);

	ssize_t			WriteToStream(BPositionIO *stream);

private:
	inline status_t	_ReallocArrayFor(int32 size);

	uint8			*fData;
	int32			fDataSize;
	int32			fAllocatedDataSize;
	int32			fBlockSize;
};


#endif	/* PSD_DATA_ARRAY_H */

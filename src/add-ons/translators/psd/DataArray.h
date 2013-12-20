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

	BDataArray&		Append(int8 val);
	BDataArray&		Append(uint8 val);
	BDataArray&		Append(int16 val);
	BDataArray&		Append(uint16 val);
	BDataArray&		Append(int32 val);
	BDataArray&		Append(uint32 val);
	BDataArray&		Append(int64 val);
	BDataArray&		Append(uint64 val);
	BDataArray&		Append(const char *str);
	BDataArray&		Append(BString& str);
	BDataArray&		Append(BDataArray& str);
	BDataArray&		Append(uint8 *ptr, int32 len);
	
	BDataArray&		Repeat(uint8 byte, int32 count);

	BDataArray&		operator<<(int8);
	BDataArray&		operator<<(uint8);
	BDataArray&		operator<<(int16);
	BDataArray&		operator<<(uint16);
	BDataArray&		operator<<(int32);
	BDataArray&		operator<<(uint32);
	BDataArray&		operator<<(int64);
	BDataArray&		operator<<(uint64);
	BDataArray&		operator<<(const char*);
	BDataArray&		operator<<(BDataArray&);

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

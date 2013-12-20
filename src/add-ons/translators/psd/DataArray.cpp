/*
 * Copyright 2013, Gerasim Troeglazov, 3dEyes@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "DataArray.h"


BDataArray::BDataArray(int32 blockSize)
{
	fBlockSize = blockSize;
	fData = (uint8*)malloc(blockSize);
	fAllocatedDataSize = blockSize;
	fDataSize = 0;
}


BDataArray::~BDataArray()
{
	free(fData);
}


status_t
BDataArray::_ReallocArrayFor(int32 size)
{
	if (fDataSize + size > fAllocatedDataSize) {
		int32 blocks = ((fDataSize + size) / fBlockSize) + 1;
		uint8 *newData = (uint8*)realloc(fData, blocks * fBlockSize);
		if (newData != NULL) {
			fData = newData;
			fAllocatedDataSize = blocks * fBlockSize;
		}
	}
	return fData == NULL ? B_NO_MEMORY : B_OK;
}


uint8*
BDataArray::Buffer(void)
{
	return fData;
}


int32
BDataArray::Length(void)
{
	return fDataSize;
}


ssize_t
BDataArray::WriteToStream(BPositionIO *stream)
{
	return stream->Write(fData, fDataSize);
}


BDataArray&
BDataArray::Append(uint8 val)
{
	status_t status = _ReallocArrayFor(sizeof(val));
	if (status == B_OK) {
		fData[fDataSize] = val;
		fDataSize++;
	}
	return *this;
}


BDataArray&
BDataArray::Append(int8 val)
{
	return Append((uint8)val);
}


BDataArray&
BDataArray::Append(uint16 val)
{
	status_t status = _ReallocArrayFor(sizeof(val));
	if (status == B_OK) {
		val = B_HOST_TO_BENDIAN_INT16(val);
		memcpy(fData + fDataSize, &val, sizeof(val));
		fDataSize += sizeof(val);
	}
	return *this;	
}


BDataArray&
BDataArray::Append(int16 val)
{
	return Append((uint16)val);
}


BDataArray&
BDataArray::Append(uint32 val)
{
	status_t status = _ReallocArrayFor(sizeof(val));
	if (status == B_OK) {
		val = B_HOST_TO_BENDIAN_INT32(val);
		memcpy(fData + fDataSize, &val, sizeof(val));
		fDataSize += sizeof(val);
	}
	return *this;
}


BDataArray&
BDataArray::Append(int64 val)
{
	return Append((uint64)val);
}


BDataArray&
BDataArray::Append(uint64 val)
{
	status_t status = _ReallocArrayFor(sizeof(val));
	if (status == B_OK) {
		val = B_HOST_TO_BENDIAN_INT64(val);
		memcpy(fData + fDataSize, &val, sizeof(val));
		fDataSize += sizeof(val);
	}
	return *this;
}


BDataArray&
BDataArray::Append(int32 val)
{
	return Append((uint32)val);
}

BDataArray&
BDataArray::Append(const char *str)
{
	int32 len = strlen(str);
	status_t status = _ReallocArrayFor(len);
	if (status == B_OK) {
		memcpy(fData + fDataSize, str, len);
		fDataSize += len;
	}
	return *this;
}


BDataArray&
BDataArray::Append(BString& str)
{
	return Append(str.String());
}


BDataArray&
BDataArray::Append(BDataArray& array)
{
	return Append(array.Buffer(), array.Length());
}


BDataArray&
BDataArray::Append(uint8 *ptr, int32 len)
{
	status_t status = _ReallocArrayFor(len);
	if (status == B_OK) {
		memcpy(fData + fDataSize, ptr, len);
		fDataSize += len;
	}
	return *this;
}


BDataArray&
BDataArray::Repeat(uint8 byte, int32 count)
{
	status_t status = _ReallocArrayFor(count);
	if (status == B_OK) {
		memset(fData + fDataSize, byte, count);
		fDataSize += count;
	}
	return *this;
}


BDataArray&
BDataArray::operator<<(int8 val)
{
	Append(val);
	return *this;
}


BDataArray&
BDataArray::operator<<(uint8 val)
{
	Append(val);
	return *this;
}


BDataArray&
BDataArray::operator<<(int16 val)
{
	Append(val);
	return *this;
}


BDataArray&
BDataArray::operator<<(uint16 val)
{
	Append(val);
	return *this;
}


BDataArray&
BDataArray::operator<<(int32 val)
{
	Append(val);
	return *this;
}


BDataArray&
BDataArray::operator<<(uint32 val)
{
	Append(val);
	return *this;
}


BDataArray&
BDataArray::operator<<(int64 val)
{
	Append(val);
	return *this;
}


BDataArray&
BDataArray::operator<<(uint64 val)
{
	Append(val);
	return *this;
}


BDataArray&
BDataArray::operator<<(const char* str)
{
	Append(str);
	return *this;
}


BDataArray&
BDataArray::operator<<(BDataArray& array)
{
	Append(array);
	return *this;
}

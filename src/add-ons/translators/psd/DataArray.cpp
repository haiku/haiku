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


status_t
BDataArray::Append(uint8 val)
{
	status_t status = _ReallocArrayFor(sizeof(val));
	if (status != B_OK)
		return status;
	fData[fDataSize] = val;
	fDataSize++;
	return B_OK;
}


status_t
BDataArray::Append(int8 val)
{
	return Append((uint8)val);
}


status_t
BDataArray::Append(uint16 val)
{
	status_t status = _ReallocArrayFor(sizeof(val));
	if (status != B_OK)
		return status;
	val = B_HOST_TO_BENDIAN_INT16(val);
	memcpy(fData + fDataSize, &val, sizeof(val));
	fDataSize += sizeof(val);
	return B_OK;	
}


status_t
BDataArray::Append(int16 val)
{
	return Append((uint16)val);
}


status_t
BDataArray::Append(uint32 val)
{
	status_t status = _ReallocArrayFor(sizeof(val));
	if (status != B_OK)
		return status;
	val = B_HOST_TO_BENDIAN_INT32(val);
	memcpy(fData + fDataSize, &val, sizeof(val));
	fDataSize += sizeof(val);
	return B_OK;
}


status_t
BDataArray::Append(int32 val)
{
	return Append((uint32)val);
}


status_t
BDataArray::Append(const char *str)
{
	int32 len = strlen(str);
	status_t status = _ReallocArrayFor(len);
	if (status != B_OK)
		return status;
	memcpy(fData + fDataSize, str, len);
	fDataSize += len;
	return B_OK;	
}


status_t
BDataArray::Append(BString str)
{
	return Append(str.String());
}


status_t
BDataArray::Append(uint8 *ptr, int32 len)
{
	status_t status = _ReallocArrayFor(len);
	if (status != B_OK)
		return status;
	memcpy(fData + fDataSize, ptr, len);
	fDataSize += len;
	return B_OK;
}


status_t
BDataArray::Repeat(uint8 byte, int32 count)
{
	status_t status = _ReallocArrayFor(count);
	if (status != B_OK)
		return status;
	memset(fData + fDataSize, byte, count);
	fDataSize += count;
	return B_OK;	
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
BDataArray::operator<<(const char* str)
{
	Append(str);
	return *this;
}

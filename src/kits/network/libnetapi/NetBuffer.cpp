/******************************************************************************
/
/	File:			NetBuffer.cpp
/
/   Description:    The Network API.
/
/	Copyright 2002, OpenBeOS Project, All Rights Reserved.
/
******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <ByteOrder.h>
#include "NetBuffer.h"

BNetBuffer::BNetBuffer(size_t size)
	:	fStatus(B_OK),
		fData(NULL),
		fSize(0),
		fOffset(0)
{
	fStatus = Resize(size);
}

BNetBuffer::BNetBuffer(const BNetBuffer & buffer)
	:	fStatus(B_OK),
		fData(NULL),
		fSize(0),
		fOffset(0)
{
	fStatus = AppendData(buffer.Data(), buffer.Size());
}

BNetBuffer::BNetBuffer(BMessage *archive)
	:	fStatus(B_OK),
		fData(NULL),
		fSize(0),
		fOffset(0)
{
	// TODO
	fStatus = B_ERROR;
}

BNetBuffer::~BNetBuffer()
{
	if (fData != NULL)
		free(fData);
}

status_t BNetBuffer::Archive(BMessage *into, bool deep) const
{
	// TODO
	return B_ERROR;
}

BArchivable *BNetBuffer::Instantiate(BMessage *archive)
{
	// TODO
	return NULL;
}

BNetBuffer & BNetBuffer::operator = (const BNetBuffer & buffer)
{
	Resize(0);
	AppendData(buffer.Data(), buffer.Size());
	return *this;
}

status_t BNetBuffer::InitCheck() const
{
	return fStatus;
}

const unsigned char * BNetBuffer::Data() const
{
	return fData;
}

unsigned char * BNetBuffer::Data()
{
	return fData;
}

size_t BNetBuffer::Size() const
{
	return fSize;
}

size_t BNetBuffer::BytesRemaining() const
{
	return fSize - fOffset;
}

status_t BNetBuffer::AppendInt8(int8 value)
{
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendUInt8(uint8 value)
{
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendInt16(int16 value)
{
	value = B_HOST_TO_BENDIAN_INT16(value);
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendUInt16(uint16 value)
{
	value = B_HOST_TO_BENDIAN_INT16(value);
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendInt32(int32 value)
{
	value = B_HOST_TO_BENDIAN_INT32(value);
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendUInt32(uint32 value)
{
	value = B_HOST_TO_BENDIAN_INT32(value);
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendFloat(float value)
{
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendDouble(double value)
{
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendString(const char * value)
{
	return AppendData(value, strlen(value) + 1);
}

status_t BNetBuffer::AppendData(const void * data, size_t length)
{
	if (Resize(fSize + length) == B_OK) {
		memcpy(fData + fSize - length, data, length);
		return B_OK;
	}
	return B_ERROR;
}

status_t BNetBuffer::AppendInt64(int64 value)
{
	value = B_HOST_TO_BENDIAN_INT64(value);
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::AppendUInt64(uint64 value)
{
	value = B_HOST_TO_BENDIAN_INT64(value);
	return AppendData(&value, sizeof(value));
}

status_t BNetBuffer::RemoveInt8(int8 & value)
{
	return RemoveData(&value, sizeof(value));
}

status_t BNetBuffer::RemoveUInt8(uint8 & value)
{
	return RemoveData(&value, sizeof(value));
}

status_t BNetBuffer::RemoveInt16(int16 & value)
{
	status_t status = RemoveData(&value, sizeof(value));
	value = B_BENDIAN_TO_HOST_INT16(value);
	return status;
}

status_t BNetBuffer::RemoveUInt16(uint16 & value)
{
	status_t status = RemoveData(&value, sizeof(value));
	value = B_BENDIAN_TO_HOST_INT16(value);
	return status;
}

status_t BNetBuffer::RemoveInt32(int32 & value)
{
	status_t status = RemoveData(&value, sizeof(value));
	value = B_BENDIAN_TO_HOST_INT32(value);
	return status;
}

status_t BNetBuffer::RemoveUInt32(uint32 & value)
{
	status_t status = RemoveData(&value, sizeof(value));
	value = B_BENDIAN_TO_HOST_INT32(value);
	return status;
}

status_t BNetBuffer::RemoveFloat(float & value)
{
	return RemoveData(&value, sizeof(value));
}

status_t BNetBuffer::RemoveDouble(double & value)
{
	return RemoveData(&value, sizeof(value));
}

status_t BNetBuffer::RemoveString(char * value, size_t length)
{
	if (fOffset < fSize && length > 0) {
		while (--length > 0 && fOffset < fSize && fData[fOffset] != 0) {
			*value++ = fData[fOffset++];
		}
		*value = 0;
		return B_OK;
	}
	return B_ERROR;
}

status_t BNetBuffer::RemoveData(void * data, size_t length)
{
	if (fOffset + length <= fSize) {
		memcpy(data, fData + fOffset, length);
		fOffset += length;
		return B_OK;
	}
	return B_ERROR;	
}

status_t BNetBuffer::RemoveInt64(int64 & value)
{
	status_t status = RemoveData(&value, sizeof(value));
	value = B_BENDIAN_TO_HOST_INT64(value);
	return status;
}

status_t BNetBuffer::RemoveUInt64(uint64 & value)
{
	status_t status = RemoveData(&value, sizeof(value));
	value = B_BENDIAN_TO_HOST_INT64(value);
	return status;
}

status_t BNetBuffer::Resize(size_t size)
{
	unsigned char * data = (unsigned char *) realloc(fData, (size + 16) & ~15);
	if (data != NULL || size == 0) {
		fData = data;
		fSize = size;
		return B_OK;
	}
	return B_ERROR;
}

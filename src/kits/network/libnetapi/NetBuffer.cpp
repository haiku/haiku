/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *				Scott T. Mansfield, thephantom@mac.com
 *              Bruno Albuquerque, bga@bug-br.org.br
 */

#include <ByteOrder.h>
#include <Message.h>
#include <TypeConstants.h>

#include "DynamicBuffer.h"
#include "NetBuffer.h"

#include <new>
#include <string.h>

BNetBuffer::BNetBuffer(size_t size) :
	BArchivable(),
	fInit(B_NO_INIT)
{
	fImpl = new (std::nothrow) DynamicBuffer(size);
	if (fImpl != NULL)
		fInit = fImpl->InitCheck();
}


BNetBuffer::~BNetBuffer()
{
	delete fImpl;
}


BNetBuffer::BNetBuffer(const BNetBuffer& buffer) :
	BArchivable(),
	fInit(B_NO_INIT)
{
	fImpl = new (std::nothrow) DynamicBuffer(*buffer.GetImpl());
	if (fImpl != NULL)
		fInit = fImpl->InitCheck();
}


BNetBuffer::BNetBuffer(BMessage* archive) :
	BArchivable(),
	fInit(B_NO_INIT)
{
	const unsigned char* bufferPtr;
	ssize_t bufferSize;

	if (archive->FindData("buffer", B_RAW_TYPE, (const void**)&bufferPtr,
		&bufferSize) == B_OK) {
		fImpl = new (std::nothrow) DynamicBuffer(bufferSize);
		if (fImpl != NULL) {
			ssize_t result = fImpl->Write(bufferPtr, bufferSize);
			if (result >= 0)
				fInit = fImpl->InitCheck();
			else
				fInit = result;
		}
	}
}

BNetBuffer&
BNetBuffer::operator=(const BNetBuffer& buffer)
{
	if (&buffer != this) {
		delete fImpl;

		fImpl = new (std::nothrow) DynamicBuffer(*buffer.GetImpl());
		if (fImpl != NULL)
			fInit = fImpl->InitCheck();
	}
	return *this;
}


status_t
BNetBuffer::Archive(BMessage* into, bool deep) const
{
	if (fInit != B_OK)
		return B_NO_INIT;

	status_t result = into->AddData("buffer", B_RAW_TYPE, fImpl->Data(),
		fImpl->BytesRemaining());

	return result;
}


BArchivable*
BNetBuffer::Instantiate(BMessage* archive)
{
    if (!validate_instantiation(archive, "BNetBuffer"))
        return NULL;

    BNetBuffer* buffer = new (std::nothrow) BNetBuffer(archive);
    if (buffer == NULL)
        return NULL;

    if (buffer->InitCheck() != B_OK) {
        delete buffer;
        return NULL;
    }

    return buffer;
}


status_t
BNetBuffer::InitCheck()
{
	return fInit;
}


status_t
BNetBuffer::AppendInt8(int8 data)
{
	return AppendData((const void*)&data, sizeof(int8));
}


status_t
BNetBuffer::AppendUint8(uint8 data)
{
	return AppendData((const void*)&data, sizeof(int8));
}


status_t
BNetBuffer::AppendInt16(int16 data)
{
	int16 be_data = B_HOST_TO_BENDIAN_INT16(data);
	return AppendData((const void*)&be_data, sizeof(int16));
}


status_t
BNetBuffer::AppendUint16(uint16 data)
{
	uint16 be_data = B_HOST_TO_BENDIAN_INT16(data);
	return AppendData((const void*)&be_data, sizeof(uint16));
}


status_t
BNetBuffer::AppendInt32(int32 data)
{
	int32 be_data = B_HOST_TO_BENDIAN_INT32(data);
	return AppendData((const void*)&be_data, sizeof(int32));
}


status_t
BNetBuffer::AppendUint32(uint32 data)
{
	uint32 be_data = B_HOST_TO_BENDIAN_INT32(data);
	return AppendData((const void*)&be_data, sizeof(uint32));
}


status_t
BNetBuffer::AppendFloat(float data)
{
	return AppendData((const void*)&data, sizeof(float));
}


status_t
BNetBuffer::AppendDouble(double data)
{
	return AppendData((const void*)&data, sizeof(double));
}


status_t
BNetBuffer::AppendString(const char* data)
{
	return AppendData((const void*)data, strlen(data) + 1);
}


status_t
BNetBuffer::AppendData(const void* data, size_t size)
{
	if (fInit != B_OK)
		return B_NO_INIT;

	ssize_t bytesWritten = fImpl->Write(data, size);
	if (bytesWritten < 0)
		return (status_t)bytesWritten;
	return (size_t)bytesWritten == size ? B_OK : B_ERROR;
}


#define STACK_BUFFER_SIZE 2048

status_t
BNetBuffer::AppendMessage(const BMessage& data)
{
	char stackFlattenedData[STACK_BUFFER_SIZE];

	ssize_t dataSize = data.FlattenedSize();

	if (dataSize < 0)
		return dataSize;

	if (dataSize == 0)
		return B_ERROR;

	status_t result = B_OK;

	if (dataSize > STACK_BUFFER_SIZE) {
		char* flattenedData = new (std::nothrow) char[dataSize];
		if (flattenedData == NULL)
			return B_NO_MEMORY;

		if (data.Flatten(flattenedData, dataSize) == B_OK)
			result = AppendData((const void*)&flattenedData, dataSize);

		delete[] flattenedData;
	} else {
		if (data.Flatten(stackFlattenedData, dataSize) == B_OK)
			result = AppendData((const void*)&stackFlattenedData, dataSize);
	}

	return result;
}


status_t
BNetBuffer::AppendInt64(int64 data)
{
	int64 be_data = B_HOST_TO_BENDIAN_INT64(data);
	return AppendData((const void*)&be_data, sizeof(int64));
}


status_t
BNetBuffer::AppendUint64(uint64 data)
{
	uint64 be_data = B_HOST_TO_BENDIAN_INT64(data);
	return AppendData((const void*)&be_data, sizeof(uint64));
}


status_t
BNetBuffer::RemoveInt8(int8& data)
{
	return RemoveData((void*)&data, sizeof(int8));
}


status_t
BNetBuffer::RemoveUint8(uint8& data)
{
	return RemoveData((void*)&data, sizeof(uint8));
}


status_t
BNetBuffer::RemoveInt16(int16& data)
{
	int16 be_data;
	status_t result = RemoveData((void*)&be_data, sizeof(int16));
	if (result != B_OK)
		return result;

	data = B_BENDIAN_TO_HOST_INT16(be_data);

	return B_OK;
}


status_t
BNetBuffer::RemoveUint16(uint16& data)
{
	uint16 be_data;
	status_t result = RemoveData((void*)&be_data, sizeof(uint16));
	if (result != B_OK)
		return result;

	data = B_BENDIAN_TO_HOST_INT16(be_data);

	return B_OK;
}


status_t
BNetBuffer::RemoveInt32(int32& data)
{
	int32 be_data;
	status_t result = RemoveData((void*)&be_data, sizeof(int32));
	if (result != B_OK)
		return result;

	data = B_BENDIAN_TO_HOST_INT32(be_data);

	return B_OK;
}


status_t
BNetBuffer::RemoveUint32(uint32& data)
{
	uint32 be_data;
	status_t result = RemoveData((void*)&be_data, sizeof(uint32));
	if (result != B_OK)
		return result;

	data = B_BENDIAN_TO_HOST_INT32(be_data);

	return B_OK;
}


status_t
BNetBuffer::RemoveFloat(float& data)
{
	return RemoveData((void*)&data, sizeof(float));
}


status_t
BNetBuffer::RemoveDouble(double& data)
{
	return RemoveData((void*)&data, sizeof(double));
}


status_t
BNetBuffer::RemoveString(char* data, size_t size)
{
	// TODO(bga): Should we do anything specific to handle the terminating
	// NULL byte?
	return RemoveData((void*)data, size);
}


status_t
BNetBuffer::RemoveData(void* data, size_t size)
{
	if (fInit != B_OK)
		return B_NO_INIT;

	ssize_t bytesRead = fImpl->Read(data, size);
	if (bytesRead < 0)
		return (status_t)bytesRead;
	return (size_t)bytesRead == size ? B_OK : B_BUFFER_OVERFLOW;
}


status_t
BNetBuffer::RemoveMessage(BMessage& data)
{
	if (fInit != B_OK)
		return B_NO_INIT;

	unsigned char* bufferPtr = fImpl->Data();

	if (*(int32*)bufferPtr != B_MESSAGE_TYPE)
		return B_ERROR;

	bufferPtr += sizeof(int32);
	int32 dataSize = *(int32*)bufferPtr;

	char* flattenedData = new (std::nothrow) char[dataSize];
	if (flattenedData == NULL)
		return B_NO_MEMORY;

	status_t result = RemoveData(flattenedData, dataSize);
	if (result == B_OK)
		result = data.Unflatten(flattenedData);

	delete[] flattenedData;

	return result;
}


status_t
BNetBuffer::RemoveInt64(int64& data)
{
	int64 be_data;
	status_t result = RemoveData((void*)&be_data, sizeof(int64));
	if (result != B_OK)
		return result;

	data = B_BENDIAN_TO_HOST_INT64(be_data);

	return B_OK;
}


status_t
BNetBuffer::RemoveUint64(uint64& data)
{
	uint64 be_data;
	status_t result = RemoveData((void*)&be_data, sizeof(uint64));
	if (result != B_OK)
		return result;

	data = B_BENDIAN_TO_HOST_INT64(be_data);

	return B_OK;
}


unsigned char*
BNetBuffer::Data() const
{
	if (fInit != B_OK)
		return NULL;

	return fImpl->Data();
}


size_t
BNetBuffer::Size() const
{
	if (fInit != B_OK)
		return 0;

	return fImpl->Size();
}


size_t
BNetBuffer::BytesRemaining() const
{
	if (fInit != B_OK)
		return 0;

	return fImpl->BytesRemaining();
}


void
BNetBuffer::_ReservedBNetBufferFBCCruft1()
{
}


void
BNetBuffer::_ReservedBNetBufferFBCCruft2()
{
}


void
BNetBuffer::_ReservedBNetBufferFBCCruft3()
{
}


void
BNetBuffer::_ReservedBNetBufferFBCCruft4()
{
}


void
BNetBuffer::_ReservedBNetBufferFBCCruft5()
{
}


void
BNetBuffer::_ReservedBNetBufferFBCCruft6()
{
}

/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "XDR.h"

#include <stdlib.h>
#include <string.h>

#include <ByteOrder.h>


using namespace XDR;

Stream::Stream(void* buffer, uint32 size)
	:
	fBuffer(reinterpret_cast<uint32*>(buffer)),
	fSize(size),
	fPosition(0)
{
}


Stream::~Stream()
{
}


uint32
Stream::_PositionToSize() const
{
	return fPosition * sizeof(uint32);
}


uint32
Stream::_RealSize(uint32 size) const
{
	uint32 real_size = size;
	if (real_size % 4 != 0)
		real_size = ((real_size >> 2) + 1) << 2;
	return real_size;
}


ReadStream::ReadStream(void* buffer, uint32 size)
	:
	Stream(buffer, size),
	fEOF(false)
{
}


ReadStream::~ReadStream()
{
}


int32
ReadStream::GetInt()
{
	if (_PositionToSize() >= fSize) {
		fEOF = true;
		return 0;
	}

	return B_BENDIAN_TO_HOST_INT32(fBuffer[fPosition++]);
}


uint32
ReadStream::GetUInt()
{
	if (_PositionToSize() >= fSize) {
		fEOF = true;
		return 0;
	}

	return B_BENDIAN_TO_HOST_INT32(fBuffer[fPosition++]);
}


int64
ReadStream::GetHyper()
{
	if (_PositionToSize() + sizeof(int64) > fSize) {
		fEOF = true;
		return 0;
	}

	int64* ptr = reinterpret_cast<int64*>(fBuffer + fPosition);
	fPosition += 2;

	return B_BENDIAN_TO_HOST_INT64(*ptr);
}


uint64
ReadStream::GetUHyper()
{
	if (_PositionToSize() + sizeof(uint64) > fSize) {
		fEOF = true;
		return 0;
	}

	uint64* ptr = reinterpret_cast<uint64*>(fBuffer + fPosition);
	fPosition += 2;

	return B_BENDIAN_TO_HOST_INT64(*ptr);
}


const char*
ReadStream::GetString()
{
	if (_PositionToSize() >= fSize) {
		fEOF = true;
		return NULL;
	}

	uint32 size;
	const void* ptr = GetOpaque(&size);
	if (ptr == NULL)
		return NULL;

	char* str = reinterpret_cast<char*>(malloc(size + 1));
	if (str == NULL)
		return NULL;

	memcpy(str, ptr, size);
	str[size] = 0;

	return str;
}


const void*
ReadStream::GetOpaque(uint32* size)
{
	if (_PositionToSize() >= fSize) {
		fEOF = true;
		return NULL;
	}

	void* ptr = NULL;
	uint32 s = GetUInt();
	if (s != 0) {
		ptr = fBuffer + fPosition;
		if (_PositionToSize() + s <= fSize)
			fPosition += _RealSize(s) / sizeof(uint32);
		else {
			s = fSize - _PositionToSize();
			fPosition = fSize;
		}
	}

	if (size != NULL)
		*size = s;

	return ptr;
}


WriteStream::WriteStream()
	:
	Stream(malloc(kInitialSize), kInitialSize),
	fError(B_OK)
{
}


WriteStream::WriteStream(const WriteStream& x)
	:
	Stream(malloc(x.fSize), x.fSize),
	fError(x.fError)
{
	fPosition = x.fPosition;
	memcpy(fBuffer, x.fBuffer, fSize);
}


WriteStream::~WriteStream()
{
	free(fBuffer);
}


void
WriteStream::Clear()
{
	free(fBuffer);
	fSize = kInitialSize;
	fBuffer = reinterpret_cast<uint32*>(malloc(fSize));
	fError = B_OK;
	fPosition = 0;
}


status_t
WriteStream::InsertUInt(Stream::Position pos, uint32 x)
{
	if (pos * sizeof(uint32) >= fSize) {
		fError = B_BAD_VALUE;
		return B_BAD_VALUE;
	}

	fBuffer[pos] = B_HOST_TO_BENDIAN_INT32(x);
	return B_OK;
}


status_t
WriteStream::AddInt(int32 x)
{
	status_t err = _CheckResize(sizeof(int32));
	if (err != B_OK)
		return err;

	fBuffer[fPosition++] = B_HOST_TO_BENDIAN_INT32(x);
	return B_OK;
}


status_t
WriteStream::AddUInt(uint32 x)
{
	status_t err = _CheckResize(sizeof(uint32));
	if (err != B_OK)
		return err;

	fBuffer[fPosition++] = B_HOST_TO_BENDIAN_INT32(x);
	return B_OK;
}


status_t
WriteStream::AddHyper(int64 x)
{
	status_t err = _CheckResize(sizeof(int64));
	if (err != B_OK)
		return err;

	int64* ptr = reinterpret_cast<int64*>(fBuffer + fPosition);
	*ptr = B_HOST_TO_BENDIAN_INT64(x);
	fPosition += 2;
	return B_OK;
}


status_t
WriteStream::AddUHyper(uint64 x)
{
	status_t err = _CheckResize(sizeof(uint64));
	if (err != B_OK)
		return err;

	uint64* ptr = reinterpret_cast<uint64*>(fBuffer + fPosition);
	*ptr = B_HOST_TO_BENDIAN_INT64(x);
	fPosition += 2;
	return B_OK;
}


status_t
WriteStream::AddString(const char* str, uint32 maxlen)
{
	uint32 len = strlen(str);
	uint32 size = min_c(maxlen, len);

	return AddOpaque(str, size);
}


status_t
WriteStream::AddOpaque(const void *ptr, uint32 size)
{
	uint32 real_size = _RealSize(size);
	status_t err = _CheckResize(real_size + sizeof(uint32));
	if (err != B_OK)
		return err;

	AddUInt(size);
	memset(fBuffer + fPosition, 0, real_size);
	memcpy(fBuffer + fPosition, ptr, size);
	fPosition += real_size / sizeof(int32);

	return B_OK;
}


status_t
WriteStream::AddOpaque(const WriteStream& stream)
{
	return AddOpaque(stream.Buffer(), stream.Size());
}


status_t
WriteStream::Append(const WriteStream& stream)
{
	uint32 size = stream.Size();
	status_t err = _CheckResize(size);
	if (err != B_OK)
		return err;

	memcpy(fBuffer + fPosition, stream.Buffer(), size);
	fPosition += size / sizeof(int32);

	return B_OK;
}


status_t
WriteStream::_CheckResize(uint32 size)
{
	if (_PositionToSize() + size <= fSize)
		return B_OK;

	uint32 new_size = max_c(fSize * 2, fPosition * sizeof(uint32) + size);

	void* ptr = realloc(fBuffer, new_size);
	if (ptr == NULL) {
		fError = B_NO_MEMORY;
		return B_NO_MEMORY;
	}

	fBuffer = reinterpret_cast<uint32*>(ptr);
	fSize = new_size;

	return B_OK;
}


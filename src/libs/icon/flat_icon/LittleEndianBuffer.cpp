/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "LittleEndianBuffer.h"

#include <ByteOrder.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


_USING_ICON_NAMESPACE


#define CHUNK_SIZE 256


// constructor
LittleEndianBuffer::LittleEndianBuffer()
	: fBuffer((uint8*)malloc(CHUNK_SIZE)),
	  fHandle(fBuffer),
	  fBufferEnd(fBuffer + CHUNK_SIZE),
	  fSize(CHUNK_SIZE),
	  fOwnsBuffer(true)
{
}

// constructor
LittleEndianBuffer::LittleEndianBuffer(size_t size)
	: fBuffer((uint8*)malloc(size)),
	  fHandle(fBuffer),
	  fBufferEnd(fBuffer + size),
	  fSize(size),
	  fOwnsBuffer(true)
{
}

// constructor
LittleEndianBuffer::LittleEndianBuffer(uint8* buffer, size_t size)
	: fBuffer(buffer),
	  fHandle(fBuffer),
	  fBufferEnd(fBuffer + size),
	  fSize(size),
	  fOwnsBuffer(false)
{
}

// destructor
LittleEndianBuffer::~LittleEndianBuffer()
{
	if (fOwnsBuffer)
		free(fBuffer);
}

// Write 8
bool
LittleEndianBuffer::Write(uint8 value)
{
	if (fHandle == fBufferEnd)
		_SetSize(fSize + CHUNK_SIZE);

	if (!fBuffer)
		return false;

	*fHandle = value;
	fHandle++;

	return true;
}

// Write 16
bool
LittleEndianBuffer::Write(uint16 value)
{
	if ((fHandle + 1) >= fBufferEnd)
		_SetSize(fSize + CHUNK_SIZE);

	if (!fBuffer)
		return false;

	*(uint16*)fHandle = B_HOST_TO_LENDIAN_INT16(value);
	fHandle += 2;

	return true;
}

// Write 32
bool
LittleEndianBuffer::Write(uint32 value)
{
	if ((fHandle + 3) >= fBufferEnd)
		_SetSize(fSize + CHUNK_SIZE);

	if (!fBuffer)
		return false;

	*(uint32*)fHandle = B_HOST_TO_LENDIAN_INT32(value);
	fHandle += 4;

	return true;
}

// Write double
bool
LittleEndianBuffer::Write(float value)
{
	if ((fHandle + sizeof(float) - 1) >= fBufferEnd)
		_SetSize(fSize + CHUNK_SIZE);

	if (!fBuffer)
		return false;

	*(float*)fHandle = B_HOST_TO_LENDIAN_FLOAT(value);
	fHandle += sizeof(float);

	return true;
}

// Write double
bool
LittleEndianBuffer::Write(double value)
{
	if ((fHandle + sizeof(double) - 1) >= fBufferEnd)
		_SetSize(fSize + CHUNK_SIZE);

	if (!fBuffer)
		return false;

	*(double*)fHandle = B_HOST_TO_LENDIAN_DOUBLE(value);
	fHandle += sizeof(double);

	return true;
}

// Write LittleEndianBuffer
bool
LittleEndianBuffer::Write(const LittleEndianBuffer& other)
{
	return Write(other.Buffer(), other.SizeUsed());
}

// Write buffer
bool
LittleEndianBuffer::Write(const uint8* buffer, size_t bytes)
{
	if (bytes == 0)
		return true;

	// figure out needed size and suitable new size
	size_t neededSize = SizeUsed() + bytes;
	size_t newSize = fSize;
	while (newSize < neededSize)
		newSize += CHUNK_SIZE;

	// resize if necessary
	if (newSize > fSize)
		_SetSize(newSize);

	if (!fBuffer)
		return false;

	// paste buffer
	memcpy(fHandle, buffer, bytes);
	fHandle += bytes;

	return true;
}

// #pragma mark -

// Read 8
bool
LittleEndianBuffer::Read(uint8& value)
{
	if (fHandle >= fBufferEnd)
		return false;

	value = *fHandle++;

	return true;
}

// Read 16
bool
LittleEndianBuffer::Read(uint16& value)
{
	if ((fHandle + 1) >= fBufferEnd)
		return false;

	value = B_LENDIAN_TO_HOST_INT16(*(uint16*)fHandle);
	fHandle += 2;

	return true;
}

// Read 32
bool
LittleEndianBuffer::Read(uint32& value)
{
	if ((fHandle + 3) >= fBufferEnd)
		return false;

	value = B_LENDIAN_TO_HOST_INT32(*(uint32*)fHandle);
	fHandle += 4;

	return true;
}

// Read float
bool
LittleEndianBuffer::Read(float& value)
{
	if ((fHandle + sizeof(float) - 1) >= fBufferEnd)
		return false;

	value = B_LENDIAN_TO_HOST_FLOAT(*(float*)fHandle);
	fHandle += sizeof(float);

	return true;
}

// Read double
bool
LittleEndianBuffer::Read(double& value)
{
	if ((fHandle + sizeof(double) - 1) >= fBufferEnd)
		return false;

	value = B_LENDIAN_TO_HOST_DOUBLE(*(double*)fHandle);
	fHandle += sizeof(double);

	return true;
}

// Read LittleEndianBuffer
bool
LittleEndianBuffer::Read(LittleEndianBuffer& other, size_t bytes)
{
	if ((fHandle + bytes - 1) >= fBufferEnd)
		return false;

	if (other.Write(fHandle, bytes)) {
		// reset other handle to beginning of pasted data
		other.fHandle -= bytes;
		fHandle += bytes;
		return true;
	}

	return false;
}

// #pragma mark -

// Skip
void
LittleEndianBuffer::Skip(size_t bytes)
{
	// NOTE: is ment to be used while reading!!
	// when used while writing, the growing will not work reliably
	fHandle += bytes;
}

// Reset
void
LittleEndianBuffer::Reset()
{
	fHandle = fBuffer;
}

// #pragma mark -

// _SetSize
void
LittleEndianBuffer::_SetSize(size_t size)
{
	if (!fOwnsBuffer) {
		// prevent user error
		// (we are in read mode)
		fBuffer = NULL;
		return;
	}

	int32 pos = fHandle - fBuffer;
	fBuffer = (uint8*)realloc((void*)fBuffer, size);
	fHandle = fBuffer + pos;
	fBufferEnd = fBuffer + size;
	fSize = size;
}


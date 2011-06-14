/*
 * Copyright 2005-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */


#include <DataIO.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


BDataIO::BDataIO()
{
}


BDataIO::~BDataIO()
{
}


// Private or Reserved

BDataIO::BDataIO(const BDataIO &)
{
	// Copying not allowed
}


BDataIO &
BDataIO::operator=(const BDataIO &)
{
	// Copying not allowed
	return *this;
}


// FBC
void BDataIO::_ReservedDataIO1(){}
void BDataIO::_ReservedDataIO2(){}
void BDataIO::_ReservedDataIO3(){}
void BDataIO::_ReservedDataIO4(){}
void BDataIO::_ReservedDataIO5(){}
void BDataIO::_ReservedDataIO6(){}
void BDataIO::_ReservedDataIO7(){}
void BDataIO::_ReservedDataIO8(){}
void BDataIO::_ReservedDataIO9(){}
void BDataIO::_ReservedDataIO10(){}
void BDataIO::_ReservedDataIO11(){}
void BDataIO::_ReservedDataIO12(){}


//	#pragma mark -


BPositionIO::BPositionIO()
{
}


BPositionIO::~BPositionIO()
{
}


ssize_t
BPositionIO::Read(void *buffer, size_t size)
{
	off_t curPos = Position();
	ssize_t result = ReadAt(curPos, buffer, size);
	if (result > 0)
		Seek(result, SEEK_CUR);

	return result;
}


ssize_t
BPositionIO::Write(const void *buffer, size_t size)
{
	off_t curPos = Position();
	ssize_t result = WriteAt(curPos, buffer, size);
	if (result > 0)
		Seek(result, SEEK_CUR);

	return result;
}


status_t
BPositionIO::SetSize(off_t size)
{
	return B_ERROR;
}


status_t
BPositionIO::GetSize(off_t* size) const
{
	if (!size)
		return B_BAD_VALUE;

	off_t currentPos = Position();
	if (currentPos < 0)
		return (status_t)currentPos;

	*size = const_cast<BPositionIO*>(this)->Seek(0, SEEK_END);
	if (*size < 0)
		return (status_t)*size;

	off_t pos = const_cast<BPositionIO*>(this)->Seek(currentPos, SEEK_SET);

	if (pos != currentPos)
		return pos < 0 ? (status_t)pos : B_ERROR;

	return B_OK;
}


// FBC
extern "C" void _ReservedPositionIO1__11BPositionIO() {}
void BPositionIO::_ReservedPositionIO2(){}
void BPositionIO::_ReservedPositionIO3(){}
void BPositionIO::_ReservedPositionIO4(){}
void BPositionIO::_ReservedPositionIO5(){}
void BPositionIO::_ReservedPositionIO6(){}
void BPositionIO::_ReservedPositionIO7(){}
void BPositionIO::_ReservedPositionIO8(){}
void BPositionIO::_ReservedPositionIO9(){}
void BPositionIO::_ReservedPositionIO10(){}
void BPositionIO::_ReservedPositionIO11(){}
void BPositionIO::_ReservedPositionIO12(){}


//	#pragma mark -


BMemoryIO::BMemoryIO(void *buffer, size_t length)
	:
	fReadOnly(false),
	fBuffer(static_cast<char*>(buffer)),
	fLength(length),
	fBufferSize(length),
	fPosition(0)
{
}


BMemoryIO::BMemoryIO(const void *buffer, size_t length)
	:
	fReadOnly(true),
	fBuffer(const_cast<char*>(static_cast<const char*>(buffer))),
	fLength(length),
	fBufferSize(length),
	fPosition(0)
{
}


BMemoryIO::~BMemoryIO()
{
}


ssize_t
BMemoryIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (buffer == NULL || pos < 0)
		return B_BAD_VALUE;

	ssize_t sizeRead = 0;
	if (pos < (off_t)fLength) {
		sizeRead = min_c((off_t)size, (off_t)fLength - pos);
		memcpy(buffer, fBuffer + pos, sizeRead);
	}
	return sizeRead;
}


ssize_t
BMemoryIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (fReadOnly)
		return B_NOT_ALLOWED;

	if (buffer == NULL || pos < 0)
		return B_BAD_VALUE;

	ssize_t sizeWritten = 0;
	if (pos < (off_t)fBufferSize) {
		sizeWritten = min_c((off_t)size, (off_t)fBufferSize - pos);
		memcpy(fBuffer + pos, buffer, sizeWritten);
	}

	if (pos + sizeWritten > (off_t)fLength)
		fLength = pos + sizeWritten;

	return sizeWritten;
}


off_t
BMemoryIO::Seek(off_t position, uint32 seek_mode)
{
	switch (seek_mode) {
		case SEEK_SET:
			fPosition = position;
			break;
		case SEEK_CUR:
			fPosition += position;
			break;
		case SEEK_END:
			fPosition = fLength + position;
			break;
		default:
			break;
	}
	return fPosition;
}


off_t
BMemoryIO::Position() const
{
	return fPosition;
}


status_t
BMemoryIO::SetSize(off_t size)
{
	if (fReadOnly)
		return B_NOT_ALLOWED;

	if (size > (off_t)fBufferSize)
		return B_ERROR;

	fLength = size;
	return B_OK;
}


// Private or Reserved

BMemoryIO::BMemoryIO(const BMemoryIO &)
{
	//Copying not allowed
}


BMemoryIO &
BMemoryIO::operator=(const BMemoryIO &)
{
	//Copying not allowed
	return *this;
}


// FBC
void BMemoryIO::_ReservedMemoryIO1(){}
void BMemoryIO::_ReservedMemoryIO2(){}


//	#pragma mark -


BMallocIO::BMallocIO()
	:
	fBlockSize(256),
	fMallocSize(0),
	fLength(0),
	fData(NULL),
	fPosition(0)
{
}


BMallocIO::~BMallocIO()
{
	free(fData);
}


ssize_t
BMallocIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	ssize_t sizeRead = 0;
	if (pos < (off_t)fLength) {
		sizeRead = min_c((off_t)size, (off_t)fLength - pos);
		memcpy(buffer, fData + pos, sizeRead);
	}
	return sizeRead;
}


ssize_t
BMallocIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	size_t newSize = max_c(pos + (off_t)size, (off_t)fLength);
	status_t error = B_OK;

	if (newSize > fMallocSize)
		error = SetSize(newSize);

	if (error == B_OK) {
		memcpy(fData + pos, buffer, size);
		if (pos + size > fLength)
			fLength = pos + size;
	}
	return error != B_OK ? error : size;
}


off_t
BMallocIO::Seek(off_t position, uint32 seekMode)
{
	switch (seekMode) {
		case SEEK_SET:
			fPosition = position;
			break;
		case SEEK_END:
			fPosition = fLength + position;
			break;
		case SEEK_CUR:
			fPosition += position;
			break;
		default:
			break;
	}
	return fPosition;
}


off_t
BMallocIO::Position() const
{
	return fPosition;
}


status_t
BMallocIO::SetSize(off_t size)
{
	status_t error = B_OK;
	if (size == 0) {
		// size == 0, free the memory
		free(fData);
		fData = NULL;
		fMallocSize = 0;
	} else {
		// size != 0, see, if necessary to resize
		size_t newSize = (size + fBlockSize - 1) / fBlockSize * fBlockSize;
		if (size != (off_t)fMallocSize) {
			// we need to resize
			if (char *newData = static_cast<char*>(realloc(fData, newSize))) {
				// set the new area to 0
				if (newSize > fMallocSize)
					memset(newData + fMallocSize, 0, newSize - fMallocSize);
				fData = newData;
				fMallocSize = newSize;
			} else	// couldn't alloc the memory
				error = B_NO_MEMORY;
		}
	}

	if (error == B_OK)
		fLength = size;

	return error;
}


void
BMallocIO::SetBlockSize(size_t blockSize)
{
	if (blockSize == 0)
		blockSize = 1;
	if (blockSize != fBlockSize)
		fBlockSize = blockSize;
}


const void *
BMallocIO::Buffer() const
{
	return fData;
}


size_t
BMallocIO::BufferLength() const
{
	return fLength;
}


// Private or Reserved

BMallocIO::BMallocIO(const BMallocIO &)
{
	// copying not allowed...
}


BMallocIO &
BMallocIO::operator=(const BMallocIO &)
{
	// copying not allowed...
	return *this;
}


// FBC
void BMallocIO::_ReservedMallocIO1() {}
void BMallocIO::_ReservedMallocIO2() {}


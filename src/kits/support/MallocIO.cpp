// MallocIO.cpp
// Just here to be able to compile and test BResources.
// To be replaced by the OpenBeOS version to be provided by the IK Team.

#include <algobase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MallocIO.h"

// constructor
BMallocIO::BMallocIO()
		 : fBlockSize(256),
		   fMallocSize(0),
		   fLength(0),
		   fData(NULL),
		   fPosition(0)
{
}

// destructor
BMallocIO::~BMallocIO()
{
	if (fData)
		free(fData);
}

// ReadAt
ssize_t
BMallocIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	status_t error = (pos >= 0 && buffer ? B_OK : B_BAD_VALUE);
	ssize_t sizeRead = 0;
	if (error == B_OK && pos < fLength) {
		sizeRead = min((off_t)size, fLength - pos);
		memcpy(buffer, fData + pos, sizeRead);
	}
	return (error != B_OK ? error : sizeRead);
}

// WriteAt
ssize_t
BMallocIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	status_t error = (pos >= 0 && buffer ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		size_t newSize = max(pos + size, (off_t)fLength);
		error = _Resize(newSize);
	}
	if (error == B_OK)
		memcpy(fData + pos, buffer, size);
	return (error != B_OK ? error : size);
}

// Seek
off_t
BMallocIO::Seek(off_t position, uint32 seekMode)
{
	off_t result = B_BAD_VALUE;
	switch (seekMode) {
		case SEEK_SET:
			if (position >= 0)
				result = fPosition = position;
			break;
		case SEEK_END:
		{
			if (fLength + position >= 0)
				result = fPosition = fLength + position;
			break;
		}
		case SEEK_CUR:
			if (fPosition + position >= 0)
				result = fPosition += position;
			break;
		default:
			break;
	}
	return result;
}

// Position
off_t
BMallocIO::Position() const
{
	return fPosition;
}

// SetSize
status_t
BMallocIO::SetSize(off_t size)
{
	status_t error = (size >= 0 ? B_OK : B_BAD_VALUE );
	if (error == B_OK)
		error = _Resize(size);
	return error;
}

// SetBlockSize
void
BMallocIO::SetBlockSize(size_t blockSize)
{
	if (blockSize == 0)
		blockSize = 1;
	if (blockSize != fBlockSize) {
		fBlockSize = blockSize;
		_Resize(fLength);
	}
}

// Buffer
const void *
BMallocIO::Buffer() const
{
	return fData;
}

// BufferLength
size_t
BMallocIO::BufferLength() const
{
	return fLength;
}

// _Resize
status_t
BMallocIO::_Resize(off_t size)
{
	status_t error = (size >= 0 ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		if (size == 0) {
			// size == 0, free the memory
			free(fData);
			fData = NULL;
		} else {
			// size != 0, see, if necessary to resize
			size_t newSize = (size + fBlockSize - 1) / fBlockSize * fBlockSize;
			if (newSize != fMallocSize) {
				// we need to resize
				if (char *newData = (char*)realloc(fData, newSize)) {
					// set the new area to 0
					if (fMallocSize < newSize) {
						memset(newData + fMallocSize, 0,
							   newSize - fMallocSize);
					}
					fData = newData;
					fMallocSize = newSize;
				} else	// couldn't alloc the memory
					error = B_NO_MEMORY;
			}
		}
	}
	if (error == B_OK)
		fLength = size;
	return error;
}


// FBC
void BMallocIO::_ReservedMallocIO1() {}
void BMallocIO::_ReservedMallocIO2() {}

// copy constructor
BMallocIO::BMallocIO(const BMallocIO &)
{
	// copying not allowed...
}

// assignment operator
BMallocIO &
BMallocIO::operator=(const BMallocIO &)
{
	// copying not allowed...
	return *this;
}


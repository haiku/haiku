//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		DataIO.h
//	Author(s):		The Storage Kit team
//	Description:	Pure virtual BDataIO and BPositioIO classes provide
//					the protocol for Read()/Write()/Seek().
//
//					BMallocIO and BMemoryIO classes implement the protocol,
//					as does BFile in the Storage Kit.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <algobase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <DataIO.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


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


/*
 * $Log $
 *
 * $Id  $
 *
 */


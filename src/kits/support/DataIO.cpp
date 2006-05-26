//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
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
//	File Name:		DataIO.cpp
//	Author(s):		Stefano Ceccherini (burton666@libero.it)
//					The Storage Team
//	Description:	Pure virtual BDataIO and BPositioIO classes provide
//					the protocol for Read()/Write()/Seek().
//
//					BMallocIO and BMemoryIO classes implement the protocol,
//					as does BFile in the Storage Kit.
//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <DataIO.h>


// *** BDataIO ***

// Construction
BDataIO::BDataIO()
{
}


// Destruction
BDataIO::~BDataIO()
{
}


// Private or Reserved
BDataIO::BDataIO(const BDataIO &)
{
	//Copying not allowed
}


BDataIO &
BDataIO::operator=(const BDataIO &)
{
	//Copying not allowed
	return *this;
}


// FBC
void BDataIO::_ReservedDataIO1(){}
void BDataIO::_ReservedDataIO2(){}
void BDataIO::_ReservedDataIO3(){}
void BDataIO::_ReservedDataIO4(){}

#if !_PR3_COMPATIBLE_
void BDataIO::_ReservedDataIO5(){}
void BDataIO::_ReservedDataIO6(){}
void BDataIO::_ReservedDataIO7(){}
void BDataIO::_ReservedDataIO8(){}
void BDataIO::_ReservedDataIO9(){}
void BDataIO::_ReservedDataIO10(){}
void BDataIO::_ReservedDataIO11(){}
void BDataIO::_ReservedDataIO12(){}
#endif


// *** BPositionIO ***

// Construction
BPositionIO::BPositionIO()
{
}


// Destruction
BPositionIO::~BPositionIO()
{
}


// Read
ssize_t
BPositionIO::Read(void *buffer, size_t size)
{
	off_t curPos = Position();
	ssize_t result = ReadAt(curPos, buffer, size);
	if (result > 0)
		Seek(result, SEEK_CUR);
	
	return result;
}


// Write
ssize_t
BPositionIO::Write(const void *buffer, size_t size)
{
	off_t curPos = Position();
	ssize_t result = WriteAt(curPos, buffer, size);
	if (result > 0)
		Seek(result, SEEK_CUR);
	
	return result;
}


// SetSize
status_t
BPositionIO::SetSize(off_t size)
{
	return B_ERROR;
}

// GetSize
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

#if !_PR3_COMPATIBLE_
void BPositionIO::_ReservedPositionIO5(){}
void BPositionIO::_ReservedPositionIO6(){}
void BPositionIO::_ReservedPositionIO7(){}
void BPositionIO::_ReservedPositionIO8(){}
void BPositionIO::_ReservedPositionIO9(){}
void BPositionIO::_ReservedPositionIO10(){}
void BPositionIO::_ReservedPositionIO11(){}
void BPositionIO::_ReservedPositionIO12(){}
#endif


// *** BMemoryIO ***

// Construction
BMemoryIO::BMemoryIO(void *p, size_t len)
		:fReadOnly(false),
		fBuf(static_cast<char*>(p)),
		fLen(len),
		fPhys(len),
		fPos(0)
		
{
}


BMemoryIO::BMemoryIO(const void *p, size_t len)
		:fReadOnly(true),
		fBuf(const_cast<char*>(static_cast<const char*>(p))),
		fLen(len),
		fPhys(len),
		fPos(0)
{
}


// Destruction
BMemoryIO::~BMemoryIO()
{
}


// ReadAt
ssize_t
BMemoryIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (buffer == NULL || pos < 0)
		return B_BAD_VALUE;
		
	ssize_t sizeRead = 0;
	if (pos < fLen) {
		sizeRead = min_c(static_cast<off_t>(size), fLen - pos);
		memcpy(buffer, fBuf + pos, sizeRead);
	}
	return sizeRead;
}


// WriteAt
ssize_t
BMemoryIO::WriteAt(off_t pos, const void *buffer, size_t size)
{	
	if (fReadOnly)
		return B_NOT_ALLOWED;
	
	if (buffer == NULL || pos < 0)
		return B_BAD_VALUE;
		
	ssize_t sizeWritten = 0;	
	if (pos < fPhys) {
		sizeWritten = min_c(static_cast<off_t>(size), fPhys - pos);
		memcpy(fBuf + pos, buffer, sizeWritten);
	}
	
	if (pos + sizeWritten > fLen)
		fLen = pos + sizeWritten;
						
	return sizeWritten;
}


// Seek
off_t
BMemoryIO::Seek(off_t position, uint32 seek_mode)
{
	switch (seek_mode) {
		case SEEK_SET:
			fPos = position;
			break;
		case SEEK_CUR:
			fPos += position;
			break;		
		case SEEK_END:
			fPos = fLen + position;
			break;
		default:
			break;
	}	
	return fPos;
}


// Position
off_t
BMemoryIO::Position() const
{
	return fPos;
}


// SetSize
status_t
BMemoryIO::SetSize(off_t size)
{
	if (fReadOnly)
		return B_NOT_ALLOWED;

	if (size > fPhys)
		return B_ERROR;
	
	fLen = size;
	
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


// *** BMallocIO ***

// Construction
BMallocIO::BMallocIO()
		 : fBlockSize(256),
		   fMallocSize(0),
		   fLength(0),
		   fData(NULL),
		   fPosition(0)
{
}


// Destruction
BMallocIO::~BMallocIO()
{
	free(fData);
}


// ReadAt
ssize_t
BMallocIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;
	
	ssize_t sizeRead = 0;
	if (pos < fLength) {
		sizeRead = min_c(static_cast<off_t>(size), fLength - pos);
		memcpy(buffer, fData + pos, sizeRead);
	}
	return sizeRead;
}


// WriteAt
ssize_t
BMallocIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;
		
	size_t newSize = max_c(pos + size, static_cast<off_t>(fLength));
	
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


// Seek
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
	status_t error = B_OK;
	if (size == 0) {
		// size == 0, free the memory
		free(fData);
		fData = NULL;
		fMallocSize = 0;
	} else {
		// size != 0, see, if necessary to resize
		size_t newSize = (size + fBlockSize - 1) / fBlockSize * fBlockSize;
		if (size != fMallocSize) {
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


// SetBlockSize
void
BMallocIO::SetBlockSize(size_t blockSize)
{
	if (blockSize == 0)
		blockSize = 1;
	if (blockSize != fBlockSize)
		fBlockSize = blockSize;
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


/*
 * $Log $
 *
 * $Id  $
 *
 */

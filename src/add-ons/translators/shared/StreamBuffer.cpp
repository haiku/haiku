/*
 * Copyright 2003-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors :
 *		Michael Wilber
 *		Jérôme Duval
 */

#include <stdio.h>
#include <string.h>
#include "StreamBuffer.h"

#ifndef min
#define min(x,y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef max
#define max(x,y) (((x) > (y)) ? (x) : (y))
#endif

// ---------------------------------------------------------------
// Constructor
//
// Initializes the StreamBuffer to read from pstream, buffering
// nbuffersize bytes of data at a time. Note that if nbuffersize
// is smaller than MIN_BUFFER_SIZE, MIN_BUFFER_SIZE is used
// as the buffer size.
//
// Preconditions:
//
// Parameters: pstream,	the stream to be buffered
//
//             nbuffersize,	number of bytes to be read from
//			                pstream at a time
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
StreamBuffer::StreamBuffer(BPositionIO *pstream, size_t nbuffersize, bool toRead)
{
	fStream = pstream;
	fBuffer = NULL;
	fBufferSize = 0;
	fLen = 0;
	fPos = 0;
	fToRead = toRead;
	
	if (!pstream)
		return;

	fBufferSize = max(nbuffersize, MIN_BUFFER_SIZE);
	fBuffer = new uint8[fBufferSize];
}

// ---------------------------------------------------------------
// Destructor
//
// Destroys data allocated for this object
//
// Preconditions:
//
// Parameters: 
//
// Postconditions:
//
// Returns:
// ---------------------------------------------------------------
StreamBuffer::~StreamBuffer()
{
	if (!fToRead && fLen > 0)
		fStream->Write(fBuffer, fLen);
	delete[] fBuffer;
	fBuffer = NULL;
}

// ---------------------------------------------------------------
// InitCheck
//
// Determines whether the constructor failed or not
//
// Preconditions: 
//
// Parameters:	
//
// Postconditions:
//
// Returns: B_OK if object has been initialized successfully,
// B_ERROR if not
// ---------------------------------------------------------------
status_t
StreamBuffer::InitCheck()
{
	if (fStream && fBuffer)
		return B_OK;
	else
		return B_ERROR;
}

// ---------------------------------------------------------------
// Read
//
// Copies up to nbytes of data from the stream into pinto
//
// Preconditions: ReadStream() must be called once before this
// function is called (the constructor does this)
//
// Parameters:	pinto,	the buffer to be copied to
//
//				nbytes,	the maximum number of bytes to copy
//
// Postconditions:
//
// Returns: the number of bytes successfully read or an
// error code returned by BPositionIO::Read()
// ---------------------------------------------------------------
ssize_t
StreamBuffer::Read(void *_pinto, size_t nbytes)
{
	if (_pinto == NULL)
		return B_BAD_VALUE;
	if (nbytes == 0)
		return 0;

	ssize_t result = B_ERROR;
	uint8 *pinto = (uint8 *)_pinto;
		
	size_t totalRead = min(nbytes, fLen - fPos);
	memcpy(pinto, fBuffer + fPos, totalRead);
	fPos += totalRead;
	pinto += totalRead;
	nbytes -= totalRead;
	
	while (nbytes > 0) {
		result = _ReadStream();
		if (result <= 0)
			return result;
		if (result > 0) {
			size_t left = min(nbytes, fLen - fPos);
			memcpy(pinto, fBuffer + fPos, left);
			fPos += left;
			pinto += left;
			nbytes -= left;
			totalRead += left;
		}
	}
	
	return totalRead;
}


// ---------------------------------------------------------------
// Write
//
// Copies up to nbytes of data from pinto into the stream
//
// Parameters:	pinto,	the buffer to be copied from
//				nbytes,	the maximum number of bytes to copy
//
// Returns: the number of bytes successfully read or an
// error code returned by BPositionIO::Read()
// ---------------------------------------------------------------
void
StreamBuffer::Write(void *pinto, size_t nbytes)
{
	if (nbytes < fBufferSize - fLen) {
		memcpy(fBuffer + fLen, pinto, nbytes);
		fLen += nbytes;
	} else {
		if (fLen > 0) {
			fStream->Write(fBuffer, fLen);
			fLen = 0;
		}
		fStream->Write(pinto, nbytes);
	}
}


// ---------------------------------------------------------------
// Seek
//
// Seeks the stream to the given position. If the seek operation fails, 
// the read buffer will be reset.
//
// Preconditions: fBuffer must be allocated and fBufferSize
// must be valid
//
// Parameters:	
//
// Postconditions:
//
// Returns: the new position
// ---------------------------------------------------------------
off_t
StreamBuffer::Seek(off_t position, uint32 seekMode)
{
	// just seek in the current buffer if the new position is in it
	if (seekMode == SEEK_CUR) {
		if (fToRead
			&& (fPos + position < fLen)
			&& (fPos + position >= 0)) {
			fPos += position;
			return Position();
		} else if (!fToRead
			&& (fLen + position < fBufferSize)
			&& (fLen + position >= 0)) {
			fLen += position;
			return Position();
		}
	} 
	
	// flush if something to write
	if (!fToRead
		&& fLen > 0) {
			fStream->Write(fBuffer, fLen);	
	}
	
	fLen = 0;
	fPos = 0;
		
	return fStream->Seek(position, seekMode);
}


// ---------------------------------------------------------------
// Position
//
// Returns the current position in the stream.
//
// Preconditions: fBuffer must be allocated and fBufferSize
// must be valid
//
// Parameters:	
//
// Postconditions:
//
// Returns: the position
// ---------------------------------------------------------------
off_t
StreamBuffer::Position()
{
	off_t position = fStream->Position();
	if (fToRead)
		position -= (fLen - fPos);
	else
		position += fLen;
	return position;
}


// ---------------------------------------------------------------
// _ReadStream
//
// Fills the stream buffer with data read in from the stream
//
// Preconditions: fBuffer must be allocated and fBufferSize
// must be valid
//
// Parameters:	
//
// Postconditions:
//
// Returns: the number of bytes successfully read or an
// error code returned by BPositionIO::Read()
// ---------------------------------------------------------------
ssize_t
StreamBuffer::_ReadStream()
{
	ssize_t len = fStream->Read(fBuffer, fBufferSize);
	if (len < 0)
		return len;
	fLen = len;
	fPos = 0;
	return fLen;
}

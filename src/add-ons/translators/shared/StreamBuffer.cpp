/*****************************************************************************/
// StreamBuffer
// Written by Michael Wilber, OBOS Translation Kit Team
//
// StreamBuffer.cpp
//
// This class is for buffering data from a BPositionIO object in order to
// improve performance for cases when small amounts of data are frequently
// read from a BPositionIO object.
//
//
// Copyright (c) 2003 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

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
StreamBuffer::StreamBuffer(BPositionIO *pstream, size_t nbuffersize,
	bool binitialread)
{
	fpStream = pstream;
	fpBuffer = NULL;
	fnBufferSize = 0;
	fnLen = 0;
	fnPos = 0;
	
	if (!pstream)
		return;

	fnBufferSize = max(nbuffersize, MIN_BUFFER_SIZE);
	fpBuffer = new uint8[fnBufferSize];
	if (fpBuffer && binitialread)
		ReadStream();
			// Fill the buffer with data so that
			// object is prepared for first call to
			// Read()
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
	fnBufferSize = 0;
	fnLen = 0;
	fnPos = 0;
	fpStream = NULL;
	
	delete[] fpBuffer;
	fpBuffer = NULL;
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
	if (fpStream && fpBuffer)
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
StreamBuffer::Read(uint8 *pinto, size_t nbytes)
{
	ssize_t result = B_ERROR;
	size_t rd1 = 0, rd2 = 0;
	
	rd1 = min(nbytes, fnLen - fnPos);
	memcpy(pinto, fpBuffer + fnPos, rd1);
	fnPos += rd1;
	
	if (rd1 < nbytes) {
		pinto += rd1;
		result = ReadStream();
		if (result > 0) {
			rd2 = min(nbytes - rd1, fnLen);
			memcpy(pinto, fpBuffer, rd2);
			fnPos += rd2;
		} else
			// return error code or zero
			return result;
	}
	
	return rd1 + rd2;
}

// ---------------------------------------------------------------
// Seek
//
// Seeks the stream to the given position and refreshes the
// read buffer.  If the seek operation fails, the read buffer
// will be reset.
//
// Preconditions: fpBuffer must be allocated and fnBufferSize
// must be valid
//
// Parameters:	
//
// Postconditions:
//
// Returns: true if the seek was successful,
// false if the seek operation failed
// ---------------------------------------------------------------
bool
StreamBuffer::Seek(off_t position)
{
	fnLen = 0;
	fnPos = 0;
	
	if (fpStream->Seek(position, SEEK_SET) == position) {
		ReadStream();
		return true;
	}
	
	return false;
}

// ---------------------------------------------------------------
// ReadStream
//
// Fills the stream buffer with data read in from the stream
//
// Preconditions: fpBuffer must be allocated and fnBufferSize
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
StreamBuffer::ReadStream()
{
	ssize_t rd;
	rd = fpStream->Read(fpBuffer, fnBufferSize);
	if (rd >= 0) {
		fnLen = rd;
		fnPos = 0;
	}
	return rd;
}

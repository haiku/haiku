/*****************************************************************************/
// StreamBuffer
// StreamBuffer.cpp
//
// The description goes here.
//
//
// Copyright (c) 2002 OpenBeOS Project
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

#include "StreamBuffer.h"

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

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
StreamBuffer::StreamBuffer(BPositionIO *pstream, size_t nbuffersize)
{
	m_pStream = pstream;
	m_pBuffer = NULL;
	m_nBufferSize = 0;
	m_nLen = 0;
	m_nPos = 0;
	
	if (!pstream)
		return;

	m_nBufferSize = max(nbuffersize, MIN_BUFFER_SIZE);
	m_pBuffer = new uint8[m_nBufferSize];
	if (m_pBuffer)
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
	m_nBufferSize = 0;
	m_nLen = 0;
	m_nPos = 0;
	m_pStream = NULL;
	
	delete[] m_pBuffer;
	m_pBuffer = NULL;
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
status_t StreamBuffer::InitCheck()
{
	if (m_pStream && m_pBuffer)
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
ssize_t StreamBuffer::Read(uint8 *pinto, size_t nbytes)
{
	ssize_t result = B_ERROR;
	size_t rd1 = 0, rd2 = 0;
	
	rd1 = min(nbytes, m_nLen - m_nPos);
	memcpy(pinto, m_pBuffer + m_nPos, rd1);
	m_nPos += rd1;
	
	if (rd1 < nbytes) {
		pinto += rd1;
		result = ReadStream();
		if (result > 0) {
			rd2 = min(nbytes - rd1, m_nLen);
			memcpy(pinto, m_pBuffer, rd2);
			m_nPos += rd2;
		} else
			// return error code or zero
			return result;
	}
	
	return rd1 + rd2;
}

// ---------------------------------------------------------------
// ReadStream
//
// Fills the stream buffer with data read in from the stream
//
// Preconditions: m_pBuffer must be allocated and m_nBufferSize
// must be valid
//
// Parameters:	
//
// Postconditions:
//
// Returns: the number of bytes successfully read or an
// error code returned by BPositionIO::Read()
// ---------------------------------------------------------------
ssize_t StreamBuffer::ReadStream()
{
	ssize_t rd;
	rd = m_pStream->Read(m_pBuffer, m_nBufferSize);
	if (rd >= 0) {
		m_nLen = rd;
		m_nPos = 0;
	}
	return rd;
}



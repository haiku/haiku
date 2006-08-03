/*****************************************************************************/
// StreamBuffer
// Written by Michael Wilber, Haiku Translation Kit Team
//
// StreamBuffer.h
//
// This class is for buffering data from a BPositionIO object in order to
// improve performance for cases when small amounts of data are frequently
// read from a BPositionIO object.
//
//
// Copyright (c) 2003 Haiku, Inc.
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

#ifndef STREAM_BUFFER_H
#define STREAM_BUFFER_H

#include <DataIO.h>

#define MIN_BUFFER_SIZE 512

class StreamBuffer {
public:
	StreamBuffer(BPositionIO *pstream, size_t nbuffersize, bool binitialread);
	~StreamBuffer();
	
	status_t InitCheck();
		// Determines whether the constructor failed or not
	
	ssize_t Read(uint8 *pinto, size_t nbytes);
		// copy nbytes from the stream into pinto
		
	bool Seek(off_t position);
		// seek the stream to the given position
	
private:
	ssize_t ReadStream();
		// Load the stream buffer from the stream

	BPositionIO *fpStream;
		// stream object this object is buffering
	uint8 *fpBuffer;
		// buffered data from fpStream
	size_t fnBufferSize;
		// number of bytes of memory allocated for fpBuffer
	size_t fnLen;
		// number of bytes of actual data in fpBuffer
	size_t fnPos;
		// current position in the buffer
};

#endif

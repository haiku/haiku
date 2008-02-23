/*
 * Copyright 2003-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 * 
 * Authors :
 *		Michael Wilber
 *		Jérôme Duval
 */

#ifndef STREAM_BUFFER_H
#define STREAM_BUFFER_H

#include <DataIO.h>

#define MIN_BUFFER_SIZE 512

class StreamBuffer {
public:
	StreamBuffer(BPositionIO *stream, size_t bufferSize, bool toRead = true);
	~StreamBuffer();
	
	status_t InitCheck();
		// Determines whether the constructor failed or not
	
	ssize_t Read(void *buffer, size_t size);
		// copy nbytes from the stream into pinto
		
	void Write(void *buffer, size_t size);
		// copy nbytes from the stream into pinto
	
	off_t Seek(off_t position, uint32 seekMode);
		// seek the stream to the given position
	
	off_t Position();
		// return the actual position
private:
	ssize_t _ReadStream();
		// Load the stream buffer from the stream

	BPositionIO *fStream;
		// stream object this object is buffering
	uint8 *fBuffer;
		// buffered data from fpStream
	size_t fBufferSize;
		// number of bytes of memory allocated for fpBuffer
	size_t fLen;
		// number of bytes of actual data in fpBuffer
	size_t fPos;
		// current position in the buffer
	bool fToRead;
		// whether the stream is to be read.
};

#endif

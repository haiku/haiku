/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Bruno Albuquerque, bga@bug-br.org.br
 */

#ifndef _DYNAMIC_BUFFER_H
#define _DYNAMIC_BUFFER_H

#include <SupportDefs.h>


class DynamicBuffer {
public:
	DynamicBuffer(size_t initialSize = 0);
	~DynamicBuffer();

	DynamicBuffer(const DynamicBuffer& buffer);
	
	// InitCheck() should be called to guarantee the object initialization
	// didn't fail. If it does not return B_OK, the initialization failed.
	status_t InitCheck() const;
	
	// Insert data at the end of the buffer. The buffer will be increased to
	// accomodate the data if needed.
	status_t Insert(const void* data, size_t size);
	
	// Remove data from the start of the buffer. Move the buffer start
	// pointer to point to the data following it.
	status_t Remove(void* data, size_t size);

	// Return a pointer to the underlying buffer. Note this will not
	// necessarily be a pointer to the start of the allocated memory as the
	// initial data may be positioned at an offset inside the buffer. In other
	// words, this returns a pointer to the start of data inside the buffer.
	unsigned char* Data() const;
	
	// Returns the actual buffer size. This is the amount of memory allocated
	// for the buffer.
	size_t Size() const;
	
	// Number of bytes of data still present in the buffer that can be
	// extracted through calls to Remove().
	size_t BytesRemaining() const;

	// Dump some buffer statistics to stdout.
	void PrintToStream();

private:
	status_t _GrowToFit(size_t size, bool exact = false);
	
	unsigned char* fBuffer;
	size_t fBufferSize;
	size_t fDataStart;
	size_t fDataEnd;
	
	status_t fInit;
};

#endif  // _DYNAMIC_BUFFER_H

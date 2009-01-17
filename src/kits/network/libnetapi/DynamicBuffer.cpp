/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *              Bruno Albuquerque, bga@bug-br.org.br
 */

#include "DynamicBuffer.h"

#include <Errors.h>
#include <SupportDefs.h>


DynamicBuffer::DynamicBuffer(size_t _initialSize) :
	fBuffer(NULL),
	fBufferSize(0),
	fDataStart(0),
	fDataEnd(0),
	fInit(B_NO_INIT)
{
	if (_initialSize > 0) {
		fBuffer = new unsigned char[_initialSize];
		if (fBuffer != NULL) {
			fBufferSize = _initialSize;
			fInit = B_OK;
		}
	}
}


DynamicBuffer::~DynamicBuffer()
{
	delete[] fBuffer;
	fBufferSize = 0;
	fDataStart = 0;
	fDataEnd = 0;
}


status_t
DynamicBuffer::InitCheck() const
{
	return fInit;
}


status_t
DynamicBuffer::Insert(const void* _data, size_t _size)
{
	if (fInit != B_OK)
		return fInit;

	status_t result = _GrowToFit(_size);
	if (result != B_OK)
		return result;
	
	memcpy(fBuffer + fDataEnd, _data, _size);
	fDataEnd += _size;
	
	return B_OK;
}


status_t
DynamicBuffer::Remove(void* _data, size_t _size)
{
	if (fInit != B_OK)
		return fInit;

	if (fDataStart + _size > fDataEnd)
		return B_BUFFER_OVERFLOW;
			
	memcpy(_data, fBuffer + fDataStart, _size);
	fDataStart += _size;
	
	if (fDataStart == fDataEnd)
		fDataStart = fDataEnd = 0;
	
	return B_OK;
}


unsigned char*
DynamicBuffer::Data() const
{
	return fBuffer + fDataStart;
}


size_t
DynamicBuffer::Size() const
{
	return fBufferSize;
}


size_t
DynamicBuffer::BytesRemaining() const
{
	return fDataEnd - fDataStart;
}


void
DynamicBuffer::PrintToStream()
{
	printf("Current buffer size : %ld\n", fBufferSize);
	printf("Data start position : %ld\n", fDataStart);
	printf("Data end position   : %ld\n", fDataEnd);
	printf("Bytes wasted        : %ld\n", fDataStart);
	printf("Bytes available     : %ld\n", fBufferSize - fDataEnd);	
}


status_t
DynamicBuffer::_GrowToFit(size_t _size)
{
	if (_size <= fBufferSize - fDataEnd)
		return B_OK;
	
	size_t newSize = (fBufferSize + _size) * 2;

	unsigned char* newBuffer = new unsigned char[newSize];
	if (newBuffer == NULL)
		return B_NO_MEMORY;

	if (fDataStart != fDataEnd) {
		memcpy(newBuffer, fBuffer + fDataStart, fDataEnd - fDataStart); 
	}
	
	delete[] fBuffer;
	fBuffer = newBuffer;
	fDataEnd -= fDataStart;
	fDataStart = 0;
	fBufferSize = newSize;

	return B_OK;
}

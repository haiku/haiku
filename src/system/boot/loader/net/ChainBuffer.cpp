/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/net/ChainBuffer.h>

#include <stdlib.h>
#include <string.h>

#include <util/kernel_cpp.h>

// constructor
ChainBuffer::ChainBuffer(void *data, uint32 size, ChainBuffer *next,
	bool freeData)
{
	_Init(data, size, next,
		CHAIN_BUFFER_ON_STACK | (freeData ? CHAIN_BUFFER_FREE_DATA : 0));
}

// destructor
ChainBuffer::~ChainBuffer()
{
	_Destroy();
}

// DetachNext
ChainBuffer *
ChainBuffer::DetachNext()
{
	if (!fNext)
		return NULL;

	ChainBuffer *next = fNext;

	fNext = NULL;
	next->fFlags |= CHAIN_BUFFER_HEAD;
	fTotalSize = fSize;

	return next;
}

// Append
void
ChainBuffer::Append(ChainBuffer *next)
{
	if (!next)
		return;

	if (fNext)
		fNext->Append(next);
	else
		fNext = next;

	fTotalSize = fSize + fNext->fTotalSize;
}

// Flatten
void
ChainBuffer::Flatten(void *_buffer) const
{
	if (uint8 *buffer = (uint8*)_buffer) {
		if (fData && fSize > 0) {
			memcpy(buffer, fData, fSize);
			buffer += fSize;
		}

		if (fNext)
			fNext->Flatten(buffer);
	}
}

// _Init
void
ChainBuffer::_Init(void *data, uint32 size, ChainBuffer *next, uint32 flags)
{
	fFlags = flags | CHAIN_BUFFER_HEAD;
	fSize = size;
	fTotalSize = fSize;
	fData = data;
	fNext = NULL;
	Append(next);
}

// _Destroy
void
ChainBuffer::_Destroy()
{
	ChainBuffer *next = fNext;
	fNext = NULL;
	if ((fFlags & CHAIN_BUFFER_FREE_DATA) && fData) {
		free(fData);
		fData = NULL;
	}

	if (!(fFlags & CHAIN_BUFFER_EMBEDDED_DATA))
		fSize = 0;
	fTotalSize = fSize;

	if (next) {
		if (next->fFlags & CHAIN_BUFFER_ON_STACK)
			next->_Destroy();
		else
			delete next;
	}
}

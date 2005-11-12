#include "RAMLinkMsgReader.h"
#include <stdlib.h>
#include <string.h>

RAMLinkMsgReader::RAMLinkMsgReader(int8 *buffer)
	: BPrivate::LinkReceiver(-1)
{
	SetBuffer(buffer);
}


RAMLinkMsgReader::RAMLinkMsgReader(void)
	: BPrivate::LinkReceiver(-1)
{
	SetBuffer(NULL);
}


RAMLinkMsgReader::~RAMLinkMsgReader(void)
{
	// Note that this class does NOT take responsibility for the buffer it reads from.
}


void
RAMLinkMsgReader::SetBuffer(int8 *buffer)
{
	if (!buffer) {
		fBuffer = NULL;
		fAttachStart = NULL;
		fPosition = NULL;
		fAttachSize = 0;
		fCode = B_ERROR;
		return;
	}

	fBuffer = buffer;
	fPosition = fBuffer + 4;

	fCode = *((int32*)fBuffer);
	fAttachSize = *((size_t *)fPosition);
	fPosition += sizeof(size_t);
	fAttachStart = fPosition;
}


int8 *
RAMLinkMsgReader::GetBuffer(void)
{
	return fBuffer;
}


size_t
RAMLinkMsgReader::GetBufferSize(void)
{
	return fAttachSize;
}


status_t
RAMLinkMsgReader::Read(void *data, ssize_t size)
{
	if (!fBuffer || fAttachSize == 0)
		return B_NO_INIT;

	if (size < 1)
		return B_BAD_VALUE;

	if (fPosition + size > fAttachStart + fAttachSize) {
		// read past end of buffer
		return B_BAD_VALUE;
	}

	memcpy(data, fPosition, size);
	fPosition += size;
	return B_OK;
}

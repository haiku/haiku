// MallocBuffer.h

#include <malloc.h>

#include "MallocBuffer.h"

// TODO: maybe this class could be more flexible by taking
// a color_space argument in the constructor
// the hardcoded width * 4 (because that's how it's used now anyways)
// could be avoided, but I'm in a hurry... :-)

// constructor
MallocBuffer::MallocBuffer(uint32 width,
						   uint32 height)
	: fBuffer(NULL),
	  fWidth(width),
	  fHeight(height)
{
	if (fWidth > 0 && fHeight > 0) {
		fBuffer = malloc((fWidth * 4) * fHeight);
	}
}

// destructor
MallocBuffer::~MallocBuffer()
{
	if (fBuffer)
		free(fBuffer);
}

// InitCheck
status_t
MallocBuffer::InitCheck() const
{
	return fBuffer ? B_OK : B_NO_MEMORY;
}

// ColorSpace
color_space
MallocBuffer::ColorSpace() const
{
	return B_RGBA32;
}

// Bits
void*
MallocBuffer::Bits() const
{
	if (InitCheck() >= B_OK)
		return fBuffer;
	return NULL;
}

// BytesPerRow
uint32
MallocBuffer::BytesPerRow() const
{
	if (InitCheck() >= B_OK)
		return fWidth * 4;
	return 0;
}

// Width
uint32
MallocBuffer::Width() const
{
	if (InitCheck() >= B_OK)
		return fWidth;
	return 0;
}

// Height
uint32
MallocBuffer::Height() const
{
	if (InitCheck() >= B_OK)
		return fHeight;
	return 0;
}


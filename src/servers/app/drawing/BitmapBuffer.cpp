// BitmapBuffer.h

#include "ServerBitmap.h"

#include "BitmapBuffer.h"

// TODO: It should be more or less guaranteed that this object
// is not used if InitCheck() returns an error, so the checks
// in all thos functions should probably be removed...

// constructor
BitmapBuffer::BitmapBuffer(ServerBitmap* bitmap)
	: fBitmap(bitmap)
{
}

// destructor
BitmapBuffer::~BitmapBuffer()
{
	// We don't own the ServerBitmap
}

// InitCheck
status_t
BitmapBuffer::InitCheck() const
{
	status_t ret = B_NO_INIT;
	if (fBitmap)
		ret = fBitmap->IsValid() ? B_OK : B_ERROR;
	return ret;
}

// ColorSpace
color_space
BitmapBuffer::ColorSpace() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->ColorSpace();
	return B_NO_COLOR_SPACE;
}

// Bits
void*
BitmapBuffer::Bits() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Bits();
	return NULL;
}

// BytesPerRow
uint32
BitmapBuffer::BytesPerRow() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->BytesPerRow();
	return 0;
}

// Width
uint32
BitmapBuffer::Width() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Width();
	return 0;
}

// Height
uint32
BitmapBuffer::Height() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Height();
	return 0;
}


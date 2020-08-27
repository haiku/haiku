// BBitmapBuffer.h

#include <Bitmap.h>

#include "BBitmapBuffer.h"

// constructor
BBitmapBuffer::BBitmapBuffer(BBitmap* bitmap)
	: fBitmap(bitmap)
{
}

// destructor
BBitmapBuffer::~BBitmapBuffer()
{
}

// InitCheck
status_t
BBitmapBuffer::InitCheck() const
{
	status_t ret = B_NO_INIT;
	if (fBitmap.Get() != NULL)
		ret = fBitmap->InitCheck();
	return ret;
}

// ColorSpace
color_space
BBitmapBuffer::ColorSpace() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->ColorSpace();
	return B_NO_COLOR_SPACE;
}

// Bits
void*
BBitmapBuffer::Bits() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Bits();
	return NULL;
}

// BytesPerRow
uint32
BBitmapBuffer::BytesPerRow() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->BytesPerRow();
	return 0;
}

// Width
uint32
BBitmapBuffer::Width() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Bounds().IntegerWidth() + 1;
	return 0;
}

// Height
uint32
BBitmapBuffer::Height() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Bounds().IntegerHeight() + 1;
	return 0;
}


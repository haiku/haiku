// BitmapBuffer.h

#include <Bitmap.h>

#include "BitmapBuffer.h"

// constructor
BitmapBuffer::BitmapBuffer(BBitmap* bitmap)
	: fBitmap(bitmap)
{
}

// destructor
BitmapBuffer::~BitmapBuffer()
{
	delete fBitmap;
}

// InitCheck
status_t
BitmapBuffer::InitCheck() const
{
	status_t ret = B_NO_INIT;
	if (fBitmap)
		ret = fBitmap->InitCheck();
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
		return fBitmap->Bounds().IntegerWidth() + 1;
	return 0;
}

// Height
uint32
BitmapBuffer::Height() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Bounds().IntegerHeight() + 1;
	return 0;
}


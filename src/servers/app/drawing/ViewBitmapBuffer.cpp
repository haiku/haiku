// ViewBitmapBuffer.h

#include <Bitmap.h>

#include "ViewBitmapBuffer.h"

// constructor
ViewBitmapBuffer::ViewBitmapBuffer(BBitmap* bitmap)
	: fBitmap(bitmap)
{
}

// destructor
ViewBitmapBuffer::~ViewBitmapBuffer()
{
	delete fBitmap;
}

// InitCheck
status_t
ViewBitmapBuffer::InitCheck() const
{
	status_t ret = B_NO_INIT;
	if (fBitmap)
		ret = fBitmap->InitCheck();
	return ret;
}

// ColorSpace
color_space
ViewBitmapBuffer::ColorSpace() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->ColorSpace();
	return B_NO_COLOR_SPACE;
}

// Bits
void*
ViewBitmapBuffer::Bits() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Bits();
	return NULL;
}

// BytesPerRow
uint32
ViewBitmapBuffer::BytesPerRow() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->BytesPerRow();
	return 0;
}

// Width
uint32
ViewBitmapBuffer::Width() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Bounds().IntegerWidth() + 1;
	return 0;
}

// Height
uint32
ViewBitmapBuffer::Height() const
{
	if (InitCheck() >= B_OK)
		return fBitmap->Bounds().IntegerHeight() + 1;
	return 0;
}


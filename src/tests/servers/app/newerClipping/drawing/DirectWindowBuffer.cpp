#include <DirectWindow.h>

#include "DirectWindowBuffer.h"

// constructor
DirectWindowBuffer::DirectWindowBuffer()
	: fBits(NULL),
	  fWidth(0),
	  fHeight(0),
	  fBytesPerRow(0),
	  fFormat(B_NO_COLOR_SPACE),
	  fWindowClipping()
{
}

// destructor
DirectWindowBuffer::~DirectWindowBuffer()
{
}

// InitCheck
status_t
DirectWindowBuffer::InitCheck() const
{
	if (fBits)
		return B_OK;
	
	return B_NO_INIT;
}

// ColorSpace
color_space
DirectWindowBuffer::ColorSpace() const
{
	return fFormat;
}

// Bits
void*
DirectWindowBuffer::Bits() const
{
	return fBits;
}

// BytesPerRow
uint32
DirectWindowBuffer::BytesPerRow() const
{
	return fBytesPerRow;
}

// Width
uint32
DirectWindowBuffer::Width() const
{
	return fWidth;
}

// Height
uint32
DirectWindowBuffer::Height() const
{
	return fHeight;
}

// Set
void
DirectWindowBuffer::SetTo(direct_buffer_info* info)
{
	fWindowClipping.MakeEmpty();

	if (info) {
		int32 xOffset = info->window_bounds.left;
		int32 yOffset = info->window_bounds.top;
		// Get clipping information
		for (int32 i = 0; i < info->clip_list_count; i++) {
			fWindowClipping.Include(info->clip_list[i]);
		}
		fBytesPerRow = info->bytes_per_row;
		fBits = (void*)info->bits;
		fFormat = info->pixel_format;
	//	fBounds = info->window_bounds;
	//	fDirty = true;
		fWidth = info->window_bounds.right - info->window_bounds.left + 1;
		fHeight = info->window_bounds.bottom - info->window_bounds.top + 1;
	} else {
		fBits = NULL;
		fWidth = 0;
		fHeight = 0;
		fBytesPerRow = 0;
		fFormat = B_NO_COLOR_SPACE;
	}
}


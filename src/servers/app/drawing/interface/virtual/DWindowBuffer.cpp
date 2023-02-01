#include <stdio.h>

#include <Accelerant.h>
#include <DirectWindow.h>

#include "DWindowBuffer.h"

// constructor
DWindowBuffer::DWindowBuffer()
	: fBits(NULL),
	  fWidth(0),
	  fHeight(0),
	  fBytesPerRow(0),
	  fFormat(B_NO_COLOR_SPACE),
	  fWindowClipping()
{
}

// destructor
DWindowBuffer::~DWindowBuffer()
{
}

// InitCheck
status_t
DWindowBuffer::InitCheck() const
{
	if (fBits)
		return B_OK;
	
	return B_NO_INIT;
}

// ColorSpace
color_space
DWindowBuffer::ColorSpace() const
{
	return fFormat;
}

// Bits
void*
DWindowBuffer::Bits() const
{
	return (void*)fBits;
}

// BytesPerRow
uint32
DWindowBuffer::BytesPerRow() const
{
	return fBytesPerRow;
}

// Width
uint32
DWindowBuffer::Width() const
{
	return fWidth;
}

// Height
uint32
DWindowBuffer::Height() const
{
	return fHeight;
}

// Set
void
DWindowBuffer::SetTo(direct_buffer_info* info)
{
	fWindowClipping.MakeEmpty();

	if (info) {
		int32 xOffset = info->window_bounds.left;
		int32 yOffset = info->window_bounds.top;
		// Get clipping information
		for (uint32 i = 0; i < info->clip_list_count; i++) {
			fWindowClipping.Include(info->clip_list[i]);
		}
		fWindowClipping.OffsetBy(xOffset, yOffset);

		fBytesPerRow = info->bytes_per_row;
		fBits = (uint8*)info->bits;
		fFormat = info->pixel_format;
		fWidth = info->window_bounds.right - info->window_bounds.left + 1;
		fHeight = info->window_bounds.bottom - info->window_bounds.top + 1;
		// offset bits to left top corner of window
		fBits += xOffset * 4 + yOffset * fBytesPerRow;
	} else {
		fBits = NULL;
		fWidth = 0;
		fHeight = 0;
		fBytesPerRow = 0;
		fFormat = B_NO_COLOR_SPACE;
	}
}

// SetTo
void
DWindowBuffer::SetTo(frame_buffer_config* config,
					 uint32 x, uint32 y,
					 uint32 width, uint32 height,
					 color_space format)
{
	fBits = (uint8*)config->frame_buffer;
	fBytesPerRow = config->bytes_per_row;
	fBits += x * 4 + y * fBytesPerRow;
	fWidth = width;
	fHeight = height;
	fFormat = format;
}

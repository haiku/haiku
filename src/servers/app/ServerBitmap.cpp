/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ServerBitmap.h"
#include "ClientMemoryAllocator.h"
#include "ColorConversion.h"
#include "HWInterface.h"
#include "Overlay.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using std::nothrow;

/*!
	A word about memory housekeeping and why it's implemented this way:

	The reason why this looks so complicated is to optimize the most common
	path (bitmap creation from the application), and don't cause any further
	memory allocations for maintaining memory in that case.
	If a bitmap was allocated this way, both, the fAllocator and fAllocationCookie
	members are used.

	For overlays, the allocator only allocates a small piece of client memory
	for use with the overlay_client_data structure - the actual buffer will be
	placed in the graphics frame buffer and is allocated by the graphics driver.

	If the memory was allocated on the app_server heap, neither fAllocator, nor
	fAllocationCookie are used, and the buffer is just freed in that case when
	the bitmap is destructed. This method is mainly used for cursors.
*/


/*!
	\brief Constructor called by the BitmapManager (only).
	\param rect Size of the bitmap.
	\param space Color space of the bitmap
	\param flags Various bitmap flags to tweak the bitmap as defined in Bitmap.h
	\param bytesperline Number of bytes in each row. -1 implies the default value. Any
	value less than the the default will less than the default will be overridden, but any value
	greater than the default will result in the number of bytes specified.
	\param screen Screen assigned to the bitmap.
*/
ServerBitmap::ServerBitmap(BRect rect, color_space space,
						   int32 flags, int32 bytesPerRow,
						   screen_id screen)
	:
	fAllocator(NULL),
	fAllocationCookie(NULL),
	fOverlay(NULL),
	fBuffer(NULL),
	fReferenceCount(1),
	// WARNING: '1' is added to the width and height.
	// Same is done in FBBitmap subclass, so if you
	// modify here make sure to do the same under
	// FBBitmap::SetSize(...)
	fWidth(rect.IntegerWidth() + 1),
	fHeight(rect.IntegerHeight() + 1),
	fBytesPerRow(0),
	fSpace(space),
	fFlags(flags),
	fBitsPerPixel(0)
	// fToken is initialized (if used) by the BitmapManager
{
	_HandleSpace(space, bytesPerRow);
}


//! Copy constructor does not copy the buffer.
ServerBitmap::ServerBitmap(const ServerBitmap* bmp)
	:
	fAllocator(NULL),
	fAllocationCookie(NULL),
	fOverlay(NULL),
	fBuffer(NULL),
	fReferenceCount(1)
{
	if (bmp) {
		fWidth			= bmp->fWidth;
		fHeight			= bmp->fHeight;
		fBytesPerRow	= bmp->fBytesPerRow;
		fSpace			= bmp->fSpace;
		fFlags			= bmp->fFlags;
		fBitsPerPixel	= bmp->fBitsPerPixel;
	} else {
		fWidth			= 0;
		fHeight			= 0;
		fBytesPerRow	= 0;
		fSpace			= B_NO_COLOR_SPACE;
		fFlags			= 0;
		fBitsPerPixel	= 0;
	}
}


ServerBitmap::~ServerBitmap()
{
	if (fAllocator != NULL)
		fAllocator->Free(AllocationCookie());
	else
		free(fBuffer);

	delete fOverlay;
		// deleting the overlay will also free the overlay buffer
}


void
ServerBitmap::Acquire()
{
	atomic_add(&fReferenceCount, 1);
}


bool
ServerBitmap::_Release()
{
	if (atomic_add(&fReferenceCount, -1) == 1)
		return true;

	return false;
}


/*! 
	\brief Internal function used by subclasses
	
	Subclasses should call this so the buffer can automagically
	be allocated on the heap.
*/
void
ServerBitmap::_AllocateBuffer(void)
{
	uint32 length = BitsLength();
	if (length > 0) {
		delete[] fBuffer;
		fBuffer = new(nothrow) uint8[length];
	}
}


/*!
	\brief Internal function used to translate color space values to appropriate internal
	values. 
	\param space Color space for the bitmap.
	\param bytesPerRow Number of bytes per row to be used as an override.
*/
void
ServerBitmap::_HandleSpace(color_space space, int32 bytesPerRow)
{
	// calculate the minimum bytes per row
	// set fBitsPerPixel
	int32 minBPR = 0;
	switch(space) {
		// 32-bit
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_UVL32:
		case B_UVLA32:
		case B_LAB32:
		case B_LABA32:
		case B_HSI32:
		case B_HSIA32:
		case B_HSV32:
		case B_HSVA32:
		case B_HLS32:
		case B_HLSA32:
		case B_CMY32:
		case B_CMYA32:
		case B_CMYK32:
			minBPR = fWidth * 4;
			fBitsPerPixel = 32;
			break;

		// 24-bit
		case B_RGB24_BIG:
		case B_RGB24:
		case B_LAB24:
		case B_UVL24:
		case B_HSI24:
		case B_HSV24:
		case B_HLS24:
		case B_CMY24:
		// TODO: These last two are calculated
		// (width + 3) / 4 * 12
		// in Bitmap.cpp, I don't understand why though.
		case B_YCbCr444:
		case B_YUV444:
			minBPR = fWidth * 3;
			fBitsPerPixel = 24;
			break;

		// 16-bit
		case B_YUV9:
		case B_YUV12:
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGB15_BIG:
		case B_RGBA15_BIG:
			minBPR = fWidth * 2;
			fBitsPerPixel = 16;
			break;

		case B_YCbCr422:
		case B_YUV422:
			minBPR = (fWidth + 3) / 4 * 8;
				// TODO: huh? why not simply fWidth * 2 ?!?
			fBitsPerPixel = 16;
			break;

		// 8-bit
		case B_CMAP8:
		case B_GRAY8:
			minBPR = fWidth;
			fBitsPerPixel = 8;
			break;

		// 1-bit
		case B_GRAY1:
			minBPR = (fWidth + 7) / 8;
			fBitsPerPixel = 1;
			break;

		// TODO: ??? get a clue what these mean
		case B_YCbCr411:
		case B_YUV411:
		case B_YUV420:
		case B_YCbCr420:
			minBPR = (fWidth + 3) / 4 * 6;
			fBitsPerPixel = 0;
			break;

		case B_NO_COLOR_SPACE:
		default:
			fBitsPerPixel = 0;
			break;
	}
	if (minBPR > 0 || bytesPerRow > 0) {
		// add the padding or use the provided bytesPerRow if sufficient
		if (bytesPerRow >= minBPR) {
			fBytesPerRow = bytesPerRow;
		} else {
			fBytesPerRow = ((minBPR + 3) / 4) * 4;
		}
	}
}


status_t
ServerBitmap::ImportBits(const void *bits, int32 bitsLength, int32 bytesPerRow,
	color_space colorSpace)
{
	if (!bits || bitsLength < 0 || bytesPerRow <= 0)
		return B_BAD_VALUE;

	return BPrivate::ConvertBits(bits, fBuffer, bitsLength, BitsLength(),
		bytesPerRow, fBytesPerRow, colorSpace, fSpace, fWidth, fHeight);
}


status_t
ServerBitmap::ImportBits(const void *bits, int32 bitsLength, int32 bytesPerRow,
	color_space colorSpace, BPoint from, BPoint to, int32 width, int32 height)
{
	if (!bits || bitsLength < 0 || bytesPerRow <= 0 || width < 0 || height < 0)
		return B_BAD_VALUE;

	return BPrivate::ConvertBits(bits, fBuffer, bitsLength, BitsLength(),
		bytesPerRow, fBytesPerRow, colorSpace, fSpace, from, to, width,
		height);
}


area_id
ServerBitmap::Area() const
{
	if (fAllocator != NULL)
		return fAllocator->Area(AllocationCookie());

	return B_ERROR;
}


uint32
ServerBitmap::AreaOffset() const
{
	if (fAllocator != NULL)
		return fAllocator->AreaOffset(AllocationCookie());

	return 0;
}


void
ServerBitmap::SetOverlay(::Overlay* cookie)
{
	fOverlay = cookie;
}


::Overlay*
ServerBitmap::Overlay() const
{
	return fOverlay;
}


void
ServerBitmap::PrintToStream()
{
	printf("Bitmap@%p: (%ld:%ld), space %ld, bpr %ld, buffer %p\n",
		this, fWidth, fHeight, (int32)fSpace, fBytesPerRow, fBuffer);
}


//	#pragma mark -


UtilityBitmap::UtilityBitmap(BRect rect, color_space space,
							 int32 flags, int32 bytesperline,
							 screen_id screen)
	: ServerBitmap(rect, space, flags, bytesperline, screen)
{
	_AllocateBuffer();
}


UtilityBitmap::UtilityBitmap(const ServerBitmap* bitmap)
	: ServerBitmap(bitmap)
{
	_AllocateBuffer();

	if (bitmap->Bits())
		memcpy(Bits(), bitmap->Bits(), bitmap->BitsLength());
}


UtilityBitmap::UtilityBitmap(const uint8* alreadyPaddedData,
							 uint32 width, uint32 height,
							 color_space format)
	: ServerBitmap(BRect(0, 0, width - 1, height - 1), format, 0)
{
	_AllocateBuffer();
	if (Bits())
		memcpy(Bits(), alreadyPaddedData, BitsLength());
}


UtilityBitmap::~UtilityBitmap()
{
}

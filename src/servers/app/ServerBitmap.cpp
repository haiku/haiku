/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */


#include "ServerBitmap.h"

#include <new>
#include <stdio.h>
#include <string.h>

using std::nothrow;


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
	: fInitialized(false),
	  fArea(B_ERROR),
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
	  // TODO: what about fToken and fOffset ?!?
{
	_HandleSpace(space, bytesPerRow);
}


//! Copy constructor does not copy the buffer.
ServerBitmap::ServerBitmap(const ServerBitmap* bmp)
	: fInitialized(false),
	  fArea(B_ERROR),
	  fBuffer(NULL),
	  fReferenceCount(1)
	  // TODO: what about fToken and fOffset ?!?
{
	if (bmp) {
		fInitialized	= bmp->fInitialized;
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


/*!
	\brief Empty. Defined for subclasses.
*/
ServerBitmap::~ServerBitmap()
{
	// TODO: Maybe it would be wiser to free the buffer here,
	// instead of do that in every subclass ?
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
		fInitialized = fBuffer != NULL;
	}
}


/*!
	\brief Internal function used by subclasses
	
	Subclasses should call this to free the internal buffer.
*/
void
ServerBitmap::_FreeBuffer()
{
	delete[] fBuffer;
	fBuffer = NULL;
	fInitialized = false;
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


UtilityBitmap::UtilityBitmap(const ServerBitmap* bmp)
	: ServerBitmap(bmp)
{
	_AllocateBuffer();
	if (bmp->Bits())
		memcpy(Bits(), bmp->Bits(), bmp->BitsLength());
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
	_FreeBuffer();
}

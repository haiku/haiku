/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "ServerBitmap.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BitmapManager.h"
#include "ClientMemoryAllocator.h"
#include "ColorConversion.h"
#include "HWInterface.h"
#include "InterfacePrivate.h"
#include "Overlay.h"
#include "ServerApp.h"


using std::nothrow;
using namespace BPrivate;


/*!	A word about memory housekeeping and why it's implemented this way:

	The reason why this looks so complicated is to optimize the most common
	path (bitmap creation from the application), and don't cause any further
	memory allocations for maintaining memory in that case.
	If a bitmap was allocated this way, both, the fAllocator and
	fAllocationCookie members are used.

	For overlays, the allocator only allocates a small piece of client memory
	for use with the overlay_client_data structure - the actual buffer will be
	placed in the graphics frame buffer and is allocated by the graphics driver.

	If the memory was allocated on the app_server heap, neither fAllocator, nor
	fAllocationCookie are used, and the buffer is just freed in that case when
	the bitmap is destructed. This method is mainly used for cursors.
*/


/*!	\brief Constructor called by the BitmapManager (only).
	\param rect Size of the bitmap.
	\param space Color space of the bitmap
	\param flags Various bitmap flags to tweak the bitmap as defined in Bitmap.h
	\param bytesperline Number of bytes in each row. -1 implies the default
		value. Any value less than the the default will less than the default
		will be overridden, but any value greater than the default will result
		in the number of bytes specified.
	\param screen Screen assigned to the bitmap.
*/
ServerBitmap::ServerBitmap(BRect rect, color_space space, uint32 flags,
		int32 bytesPerRow, screen_id screen)
	:
	fMemory(NULL),
	fOverlay(NULL),
	fBuffer(NULL),
	// WARNING: '1' is added to the width and height.
	// Same is done in FBBitmap subclass, so if you
	// modify here make sure to do the same under
	// FBBitmap::SetSize(...)
	fWidth(rect.IntegerWidth() + 1),
	fHeight(rect.IntegerHeight() + 1),
	fBytesPerRow(0),
	fSpace(space),
	fFlags(flags),
	fOwner(NULL)
	// fToken is initialized (if used) by the BitmapManager
{
	int32 minBytesPerRow = get_bytes_per_row(space, fWidth);

	fBytesPerRow = max_c(bytesPerRow, minBytesPerRow);
}


//! Copy constructor does not copy the buffer.
ServerBitmap::ServerBitmap(const ServerBitmap* bitmap)
	:
	fMemory(NULL),
	fOverlay(NULL),
	fBuffer(NULL),
	fOwner(NULL)
{
	if (bitmap) {
		fWidth = bitmap->fWidth;
		fHeight = bitmap->fHeight;
		fBytesPerRow = bitmap->fBytesPerRow;
		fSpace = bitmap->fSpace;
		fFlags = bitmap->fFlags;
	} else {
		fWidth = 0;
		fHeight = 0;
		fBytesPerRow = 0;
		fSpace = B_NO_COLOR_SPACE;
		fFlags = 0;
	}
}


ServerBitmap::~ServerBitmap()
{
	if (fMemory != NULL) {
		if (fMemory != &fClientMemory)
			delete fMemory;
	} else
		delete[] fBuffer;

	delete fOverlay;
		// deleting the overlay will also free the overlay buffer
}


/*!	\brief Internal function used by subclasses

	Subclasses should call this so the buffer can automagically
	be allocated on the heap.
*/
void
ServerBitmap::AllocateBuffer()
{
	uint32 length = BitsLength();
	if (length > 0) {
		delete[] fBuffer;
		fBuffer = new(std::nothrow) uint8[length];
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
	if (fMemory != NULL)
		return fMemory->Area();

	return B_ERROR;
}


uint32
ServerBitmap::AreaOffset() const
{
	if (fMemory != NULL)
		return fMemory->AreaOffset();

	return 0;
}


void
ServerBitmap::SetOverlay(::Overlay* overlay)
{
	fOverlay = overlay;
}


::Overlay*
ServerBitmap::Overlay() const
{
	return fOverlay;
}


void
ServerBitmap::SetOwner(ServerApp* owner)
{
	fOwner = owner;
}


ServerApp*
ServerBitmap::Owner() const
{
	return fOwner;
}


void
ServerBitmap::PrintToStream()
{
	printf("Bitmap@%p: (%" B_PRId32 ":%" B_PRId32 "), space %" B_PRId32 ", "
		"bpr %" B_PRId32 ", buffer %p\n", this, fWidth, fHeight, (int32)fSpace,
		fBytesPerRow, fBuffer);
}


//	#pragma mark -


UtilityBitmap::UtilityBitmap(BRect rect, color_space space, uint32 flags,
		int32 bytesPerRow, screen_id screen)
	:
	ServerBitmap(rect, space, flags, bytesPerRow, screen)
{
	AllocateBuffer();
}


UtilityBitmap::UtilityBitmap(const ServerBitmap* bitmap)
	:
	ServerBitmap(bitmap)
{
	AllocateBuffer();

	if (bitmap->Bits())
		memcpy(Bits(), bitmap->Bits(), bitmap->BitsLength());
}


UtilityBitmap::UtilityBitmap(const uint8* alreadyPaddedData, uint32 width,
		uint32 height, color_space format)
	:
	ServerBitmap(BRect(0, 0, width - 1, height - 1), format, 0)
{
	AllocateBuffer();
	if (Bits())
		memcpy(Bits(), alreadyPaddedData, BitsLength());
}


UtilityBitmap::~UtilityBitmap()
{
}

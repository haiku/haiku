/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!
	Although descended from ServerBitmaps, ServerCursors are not handled by
	the BitmapManager - they are allocated like any other object. Unlike BeOS 
	R5, cursors can be any size or color space, and this class accomodates and
	expands the R5 API.
*/

#include "CursorManager.h"
#include "ServerCursor.h"

#include <ByteOrder.h>

#include <new>
#include <stdio.h>


using std::nothrow;

/*!
	\brief Constructor
	\param r Size of the cursor
	\param cspace Color space of the cursor
	\param flags ServerBitmap flags. See Bitmap.h.
	\param hotspot Hotspot of the cursor
	\param bytesperline Bytes per row for the cursor. See ServerBitmap::ServerBitmap()
	
*/
ServerCursor::ServerCursor(BRect r, color_space format,
						   int32 flags, BPoint hotspot,
						   int32 bytesPerRow,
						   screen_id screen)
	: ServerBitmap(r, format, flags, bytesPerRow, screen),
	  fHotSpot(hotspot),
	  fOwningTeam(-1),
	  fReferenceCount(1),
	  fCursorData(NULL),
	  fManager(NULL),
	  fPendingViewCursor(0)
{
	fHotSpot.ConstrainTo(Bounds());
	_AllocateBuffer();
}


/*!
	\brief Constructor
	\param data Pointer to 68-byte cursor data array. See BeBook entry for BCursor for details
*/
ServerCursor::ServerCursor(const uint8* data)
	: ServerBitmap(BRect(0, 0, 15, 15), B_RGBA32, 0),
	  fHotSpot(0, 0),
	  fOwningTeam(-1),
	  fReferenceCount(1),
	  fCursorData(NULL),
	  fManager(NULL),
	  fPendingViewCursor(0)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported) in R2

	// Now that we have all the setup, we're going to map (for now) the cursor
	// to RGBA32 (little endian). Eventually, there will be support for 16 and
	// 8-bit depths
	// NOTE: review this once we have working PPC graphics cards (big endian).
	if (data) {	
		_AllocateBuffer();
		uint8* buffer = Bits();
		if (!buffer)
			return;

		fInitialized = true;

		uint16* cursorBits = (uint16*)(data + 4);
		uint16* transparencyBits = (uint16*)(data + 36);
		fHotSpot.Set(data[3], data[2]);

		// for each row in the cursor data
		for (int32 j = 0; j < 16; j++) {
			uint32* bits = (uint32*)(buffer + (j * BytesPerRow()));

			// On intel, our bytes end up swapped, so we must swap them back
			uint16 cursorLine = __swap_int16(cursorBits[j]);
			uint16 transparencyLine = __swap_int16(transparencyBits[j]);

			uint16 mask = 1 << 15;

			// for each column in each row of cursor data
			for (int32 i = 0; i < 16; i++, mask >>= 1) {
				// Get the values and dump them to the bitmap
				if (cursorLine & mask)
					bits[i] = 0xff000000; // black
				else if (transparencyLine & mask)
					bits[i] = 0xffffffff; // white
				else
					bits[i] = 0x00000000; // transparent
			}
		}

		// remember cursor data for later
		fCursorData = new (nothrow) uint8[68];
		if (fCursorData)
			memcpy(fCursorData, data, 68);

	} else {
		fWidth = 0;
		fHeight = 0;
		fBytesPerRow = 0;
		fSpace = B_NO_COLOR_SPACE;
	}
}


/*!
	\brief Constructor
	\param data Pointer to bitmap data in memory,
	the padding bytes should be contained when format less than 32 bpp.
*/
ServerCursor::ServerCursor(const uint8* alreadyPaddedData,
						   uint32 width, uint32 height,
						   color_space format)
	: ServerBitmap(BRect(0, 0, width - 1, height - 1), format, 0),
	  fHotSpot(0, 0),
	  fOwningTeam(-1),
	  fReferenceCount(1),
	  fCursorData(NULL),
	  fManager(NULL),
	  fPendingViewCursor(0)
{	
	_AllocateBuffer();
	if (Bits())
		memcpy(Bits(), alreadyPaddedData, BitsLength());
}


/*!
	\brief Copy constructor
	\param cursor cursor to copy
*/
ServerCursor::ServerCursor(const ServerCursor* cursor)
	: ServerBitmap(cursor),
	  fHotSpot(0, 0),
	  fOwningTeam(-1),
	  fReferenceCount(1),
	  fCursorData(NULL),
	  fManager(NULL),
	  fPendingViewCursor(0)
{
	// TODO: Hm. I don't move this into the if clause,
	// because it might break code elsewhere.
	_AllocateBuffer();

	if (cursor) {	
		fInitialized = true;
		if (Bits() && cursor->Bits())
			memcpy(Bits(), cursor->Bits(), BitsLength());
		fHotSpot = cursor->fHotSpot;
		if (cursor->fCursorData) {
			fCursorData = new (nothrow) uint8[68];
			if (fCursorData)
				memcpy(fCursorData, cursor->fCursorData, 68);
		}
	}
}


//!	Frees the heap space allocated for the cursor's image data
ServerCursor::~ServerCursor()
{
}


/*!
	\brief Sets the cursor's hotspot
	\param pt New location of hotspot, constrained to the cursor's boundaries.
*/
void
ServerCursor::SetHotSpot(BPoint hotSpot)
{
	fHotSpot = hotSpot;
	fHotSpot.ConstrainTo(Bounds());
}


bool
ServerCursor::Release()
{
	if (atomic_add(&fReferenceCount, -1) == 1) {
		if (fPendingViewCursor > 0) {
			// There is a SetViewCursor() waiting to be carried out
			return false;
		}

		if (fManager) {
			fManager->RemoveCursor(this);
		}
		delete this;

		return true;
	}
	return false;
}


void
ServerCursor::SetPendingViewCursor(bool pending)
{
	atomic_add(&fPendingViewCursor, pending ? 1 : -1);
}


void
ServerCursor::AttachedToManager(CursorManager* manager)
{
	fManager = manager;
}


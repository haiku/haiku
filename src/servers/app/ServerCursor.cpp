/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	Although descended from ServerBitmaps, ServerCursors are not handled by
	the BitmapManager, but the CursorManager instead. Until they have been
	attached to a CursorManager, you can delete cursors like any other object.

	Unlike BeOS, cursors can be any size or color space, and this class
	accomodates and expands the BeOS API.
*/


#include "CursorManager.h"
#include "ServerCursor.h"

#include <ByteOrder.h>

#include <new>
#include <stdio.h>


using std::nothrow;


/*!	\brief Constructor

	\param r Size of the cursor
	\param cspace Color space of the cursor
	\param flags ServerBitmap flags. See Bitmap.h.
	\param hotspot Hotspot of the cursor
	\param bytesperline Bytes per row for the cursor. See
		ServerBitmap::ServerBitmap()

*/
ServerCursor::ServerCursor(BRect r, color_space format, int32 flags,
		BPoint hotspot, int32 bytesPerRow, screen_id screen)
	:
	ServerBitmap(r, format, flags, bytesPerRow, screen),
	fHotSpot(hotspot),
	fOwningTeam(-1),
	fCursorData(NULL),
	fManager(NULL)
{
	fHotSpot.ConstrainTo(Bounds());
	AllocateBuffer();
}


/*!	\brief Constructor
	\param data Pointer to 68-byte cursor data array. See BeBook entry for
		BCursor for details
*/
ServerCursor::ServerCursor(const uint8* data)
	:
	ServerBitmap(BRect(0, 0, 15, 15), B_RGBA32, 0),
	fHotSpot(0, 0),
	fOwningTeam(-1),
	fCursorData(NULL),
	fManager(NULL)
{
	// 68-byte array used in BeOS for holding cursors.
	// This API has serious problems and should be deprecated (but supported)
	// in R2

	// Now that we have all the setup, we're going to map (for now) the cursor
	// to RGBA32 (little endian). Eventually, there will be support for 16 and
	// 8-bit depths
	// NOTE: review this once we have working PPC graphics cards (big endian).
	if (data) {
		AllocateBuffer();
		uint8* buffer = Bits();
		if (!buffer)
			return;

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


/*!	\brief Constructor
	\param data Pointer to bitmap data in memory,
	the padding bytes should be contained when format less than 32 bpp.
*/
ServerCursor::ServerCursor(const uint8* alreadyPaddedData, uint32 width,
		uint32 height, color_space format)
	:
	ServerBitmap(BRect(0, 0, width - 1, height - 1), format, 0),
	fHotSpot(0, 0),
	fOwningTeam(-1),
	fCursorData(NULL),
	fManager(NULL)
{
	AllocateBuffer();
	if (Bits())
		memcpy(Bits(), alreadyPaddedData, BitsLength());
}


/*!	\brief Copy constructor
	\param cursor cursor to copy
*/
ServerCursor::ServerCursor(const ServerCursor* cursor)
	:
	ServerBitmap(cursor),
	fHotSpot(0, 0),
	fOwningTeam(-1),
	fCursorData(NULL),
	fManager(NULL)
{
	// TODO: Hm. I don't move this into the if clause,
	// because it might break code elsewhere.
	AllocateBuffer();

	if (cursor) {
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
	delete[] fCursorData;
}


/*!	\brief Sets the cursor's hotspot
	\param pt New location of hotspot, constrained to the cursor's boundaries.
*/
void
ServerCursor::SetHotSpot(BPoint hotSpot)
{
	fHotSpot = hotSpot;
	fHotSpot.ConstrainTo(Bounds());
}


void
ServerCursor::AttachedToManager(CursorManager* manager)
{
	fManager = manager;
}


void
ServerCursor::LastReferenceReleased()
{
	if (fManager == NULL || fManager->RemoveCursor(this))
		delete this;
}


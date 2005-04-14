//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ServerCursor.cpp
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//					Stephan Aßmus <superstippi@gmx.de>
//	Description:	Glorified ServerBitmap used for cursor work.
//  
//------------------------------------------------------------------------------
#include "ServerCursor.h"
#include <stdio.h>
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
	  fOwningTeam(-1)
{
	fHotSpot.ConstrainTo(Bounds());
	_AllocateBuffer();
}

/*!
	\brief Constructor
	\param data Pointer to 68-byte cursor data array. See BeBook entry for BCursor for details
*/
ServerCursor::ServerCursor(const int8* data)
	: ServerBitmap(BRect(0, 0, 15, 15), B_RGBA32, 0),
	  fHotSpot(0, 0),
	  fOwningTeam(-1)
{
	// 68-byte array used in R5 for holding cursors.
	// This API has serious problems and should be deprecated(but supported) in R2

	// Now that we have all the setup, we're going to map (for now) the cursor
	// to RGBA32. Eventually, there will be support for 16 and 8-bit depths
	if (data) {	
		_AllocateBuffer();
		uint8* buffer = Bits();
		if (!buffer)
			return;

		fInitialized = true;
		uint32 black = 0xFF000000;
		uint32 white = 0xFFFFFFFF;

		uint16* cursorpos = (uint16*)(data + 4);
		uint16* maskpos = (uint16*)(data + 36);
		fHotSpot.Set(data[3], data[2]);
				
		uint32* bmppos;
		uint16 cursorflip;
		uint16 maskflip;
		uint16 cursorval;
		uint16 maskval;
		uint16 powval;
	
		// for each row in the cursor data
		for (int32 j = 0; j < 16; j++) {
			bmppos = (uint32*)(buffer + (j * BytesPerRow()));
	
			// On intel, our bytes end up swapped, so we must swap them back
			cursorflip = (cursorpos[j] & 0xFF) << 8;
			cursorflip |= (cursorpos[j] & 0xFF00) >> 8;
			
			maskflip = (maskpos[j] & 0xFF) << 8;
			maskflip |= (maskpos[j] & 0xFF00) >> 8;
	
			// for each column in each row of cursor data
			for (int32 i = 0; i < 16; i++) {
				// Get the values and dump them to the bitmap
				powval = 1 << (15 - i);
				cursorval = cursorflip & powval;
				maskval = maskflip & powval;
				bmppos[i] = ((cursorval != 0) ? black : white) &
							((maskval > 0) ? 0xFFFFFFFF : 0x00FFFFFF);
			}
		}
	} else {
		fWidth = 0;
		fHeight = 0;
		fBytesPerRow = 0;
		fSpace = B_NO_COLOR_SPACE;
	}
}

/*!
	\brief Constructor
	\param data Pointer to bitmap data in memory, the padding bytes should be contained when format less than 32 bpp.
*/
ServerCursor::ServerCursor(const uint8* alreadyPaddedData,
						   uint32 width, uint32 height,
						   color_space format)
	: ServerBitmap(BRect(0, 0, width - 1, height - 1), format, 0),
	  fHotSpot(0, 0),
	  fOwningTeam(-1)
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
	  fOwningTeam(-1)
{
	// TODO: Hm. I don't move this into the if clause,
	// because it might break code elsewhere.
	_AllocateBuffer();

	if (cursor) {	
		fInitialized = true;
		if (Bits() && cursor->Bits())
			memcpy(Bits(), cursor->Bits(), BitsLength());
		fHotSpot = cursor->fHotSpot;
	}
}

//!	Frees the heap space allocated for the cursor's image data
ServerCursor::~ServerCursor()
{
	_FreeBuffer();
}

/*!
	\brief Sets the cursor's hotspot
	\param pt New location of hotspot, constrained to the cursor's boundaries.
*/
void
ServerCursor::SetHotSpot(BPoint pt)
{
	fHotSpot = pt;
	fHotSpot.ConstrainTo(Bounds());
}

// SetAppSignature
void
ServerCursor::SetAppSignature(const char* signature)
{
	fAppSignature.SetTo((signature) ? signature : "");
}
